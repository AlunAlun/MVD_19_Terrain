#include "DebugSystem.h"
#include "extern.h"
#include "Parsers.h"
#include "shaders_default.h"

DebugSystem::~DebugSystem() {
	delete grid_shader_;
	delete icon_shader_;
}

void DebugSystem::init(GraphicsSystem* gs) {
	graphics_system_ = gs;
}

void DebugSystem::lateInit() {
	//init booleans
	draw_grid_ = false;
	draw_icons_ = false;
	draw_frustra_ = false;
	draw_colliders_ = false;

	//compile debug shaders from strings in header file
	grid_shader_ = new Shader();
	grid_shader_->compileFromStrings(g_shader_line_vertex, g_shader_line_fragment);
	icon_shader_ = new Shader();
	icon_shader_->compileFromStrings(g_shader_icon_vertex, g_shader_icon_fragment);

	//create geometries
	createGrid_();
	createIcon_();
	createCube_();
	createRay_();

	//create texture for light icon
	icon_light_texture_ = Parsers::parseTexture("data/assets/icon_light.tga");
	icon_camera_texture_ = Parsers::parseTexture("data/assets/icon_camera.tga");

	//picking collider
	ent_picking_ray_ = ECS.createEntity("picking_ray");
	Collider& picking_ray = ECS.createComponentForEntity<Collider>(ent_picking_ray_);
	picking_ray.collider_type = ColliderTypeRay;
	picking_ray.direction = lm::vec3(0, 0, -1);
	picking_ray.max_distance = 0.001f;

	setActive(true);
}

//draws debug information or not
void DebugSystem::setActive(bool a) {
	draw_grid_ = a;
	draw_icons_ = a;
	draw_frustra_ = a;
	draw_colliders_ = a;
}

//called once per frame
void DebugSystem::update(float dt) {

	//get the camera view projection matrix
	lm::mat4 vp = ECS.getComponentInArray<Camera>(ECS.main_camera).view_projection;

	//line drawing first
	if (draw_grid_ || draw_frustra_ || draw_colliders_) {

		//use line shader to draw all lines and boxes
		glUseProgram(grid_shader_->program);
		GLint u_mvp = glGetUniformLocation(grid_shader_->program, "u_mvp");
		GLint u_color = glGetUniformLocation(grid_shader_->program, "u_color");
		GLint u_color_mod = glGetUniformLocation(grid_shader_->program, "u_color_mod");
		GLint u_size_scale = glGetUniformLocation(grid_shader_->program, "u_size_scale");
		GLint u_center_mod = glGetUniformLocation(grid_shader_->program, "u_center_mod");



		if (draw_grid_) {
			//set uniforms and draw grid
			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, vp.m);
			glUniform3fv(u_color, 4, grid_colors);
			glUniform3f(u_size_scale, 1.0, 1.0, 1.0);
			glUniform3f(u_center_mod, 0.0, 0.0, 0.0);
			glUniform1i(u_color_mod, 0);
			glBindVertexArray(grid_vao_); //GRID
			glDrawElements(GL_LINES, grid_num_indices, GL_UNSIGNED_INT, 0);
		}

		if (draw_frustra_) {
			//draw frustra for all cameras
			auto& cameras = ECS.getAllComponents<Camera>();
            int counter = 0;
			for (auto& cc : cameras) {
                //don't draw current camera frustum
                if (counter == ECS.main_camera) continue;
                counter++;
                
				lm::mat4 cam_iv = cc.view_matrix;
				cam_iv.inverse();
				lm::mat4 cam_ip = cc.projection_matrix;
				cam_ip.inverse();
				lm::mat4 cam_ivp = cc.view_projection;
				cam_ivp.inverse();
				lm::mat4 mvp = vp * cam_ivp;

				//set uniforms and draw cube
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
				glUniform1i(u_color_mod, 1); //set color to index 1 (red)
				glBindVertexArray(cube_vao_); //CUBE
				glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
			}

			//now for lights
			auto& lights = ECS.getAllComponents<Light>();
			for (auto& ll : lights) {
				if (!ll.cast_shadow) continue;
				lm::mat4 light_ivp = ll.view_projection;
				light_ivp.inverse();
				lm::mat4 mvp = vp * light_ivp;

				//set uniforms and draw cube
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
				glUniform1i(u_color_mod, 1); //set color to index 1 (red)
				glBindVertexArray(cube_vao_); //CUBE
				glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
			}
		}

		if (draw_colliders_) {
			//draw all colliders
			auto& colliders = ECS.getAllComponents<Collider>();
			for (auto& cc : colliders) {
				//get transform for collider
				Transform& tc = ECS.getComponentFromEntity<Transform>(cc.owner);
				//get the colliders local model matrix in order to draw correctly
				lm::mat4 collider_matrix = tc.getGlobalMatrix(ECS.getAllComponents<Transform>());

				if (cc.collider_type == ColliderTypeBox) {

					//now move by the box by its offset
					collider_matrix.translateLocal(cc.local_center.x, cc.local_center.y, cc.local_center.z);
					//convert -1 -> +1 geometry to size of collider box
					collider_matrix.scaleLocal(cc.local_halfwidth.x, cc.local_halfwidth.y, cc.local_halfwidth.z);
					//set mvp
					lm::mat4 mvp = vp * collider_matrix;

					//set uniforms and draw
					glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
					glUniform1i(u_color_mod, 2); //set color to index 2 (green)
					glBindVertexArray(cube_vao_); //CUBE
					glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
				}

				if (cc.collider_type == ColliderTypeRay) {
					//ray natively goes from (0,0,0 to 0,0,1) (see definition in createRay_())
					//we need to rotate this vector so that it matches the direction specified by the component
					//to do this we first need to find angle and axis between the two vectors
					lm::vec3 buffer_vec(0, 0, 1);
					lm::vec3 dir_norm = cc.direction;
					dir_norm.normalize();
					float rotation_angle = acos(dir_norm.dot(buffer_vec));
					//if angle is PI, vector is opposite to buffer vec
					//so rotation axis can be anything (we set it to 0,1,0 vector
					lm::vec3 rotation_axis = lm::vec3(0, 1, 0);
					//otherwise, we calculate rotation axis with cross product
					if (rotation_angle < 3.14159f) {
						rotation_axis = dir_norm.cross(buffer_vec).normalize();
					}
					//now we rotate the buffer vector to
					if (rotation_angle > 0.00001f) {
						//only rotate if we have to
						collider_matrix.rotateLocal(rotation_angle, rotation_axis);
					}
					//apply distance scale
					collider_matrix.scaleLocal(cc.max_distance, cc.max_distance, cc.max_distance);
					//apply center offset
					collider_matrix.translateLocal(cc.local_center.x, cc.local_center.y, cc.local_center.z);

					//set uniforms
					lm::mat4 mvp = vp * collider_matrix;
					glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
					//set color to index 2 (green)
					glUniform1i(u_color_mod, 3);

					//bind the cube vao
					glBindVertexArray(collider_ray_vao_);
					glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, 0);
				}
			}
		}
	}

	if (draw_icons_) {
		//switch to icon shader
		glUseProgram(icon_shader_->program);

		//get uniforms
		GLint u_mvp = glGetUniformLocation(icon_shader_->program, "u_mvp");
		GLint u_icon = glGetUniformLocation(icon_shader_->program, "u_icon");
		glUniform1i(u_icon, 0);


		//for each light - bind light texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, icon_light_texture_);

		auto& lights = ECS.getAllComponents<Light>();
		for (auto& curr_light : lights) {
			Transform& curr_light_transform = ECS.getComponentFromEntity<Transform>(curr_light.owner);

			lm::mat4 mvp_matrix = vp * curr_light_transform.getGlobalMatrix(ECS.getAllComponents<Transform>());;
			//BILLBOARDS
			//the mvp for the light contains rotation information. We want it to look at the camera always.
			//So we zero out first three columns of matrix, which contain the rotation information
			//this is an extremely simple billboard
			lm::mat4 bill_matrix;
			for (int i = 12; i < 16; i++) bill_matrix.m[i] = mvp_matrix.m[i];

			//send this new matrix as the MVP
			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, bill_matrix.m);
			glBindVertexArray(icon_vao_);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}

		//bind camera texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, icon_camera_texture_);

		//for each camera, exactly the same but with camera texture
		auto& cameras = ECS.getAllComponents<Camera>();
		for (auto& curr_camera : cameras) {
			Transform& curr_cam_transform = ECS.getComponentFromEntity<Transform>(curr_camera.owner);
			lm::mat4 mvp_matrix = vp * curr_cam_transform.getGlobalMatrix(ECS.getAllComponents<Transform>());

			// billboard as above
			lm::mat4 bill_matrix;
			for (int i = 12; i < 16; i++) bill_matrix.m[i] = mvp_matrix.m[i];
			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, bill_matrix.m);
			glBindVertexArray(icon_vao_);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		}
	}
	glBindVertexArray(0);

	//imGUI
	updateimGUI_(dt);
}

// recursive function to render a transform node in imGUI
void DebugSystem::imGuiRenderTransformNode(TransformNode& trans) {
	auto& ent = ECS.entities[trans.entity_owner];
	if (ImGui::TreeNode(ent.name.c_str())) {
		Transform& transform = ECS.getComponentFromEntity<Transform>(ent.name);
		if (ECS.getComponentID<Light>(trans.entity_owner) != -1) {
			graphics_system_->needUpdateLights = true;
		}
		lm::vec3 pos = transform.position();
		float pos_array[3] = { pos.x, pos.y, pos.z };
		ImGui::DragFloat3("Position", pos_array);
		transform.position(pos_array[0], pos_array[1], pos_array[2]);

		for (auto& child : trans.children) {

			imGuiRenderTransformNode(child);
		}
		ImGui::TreePop();
	}
}


//called at the end of DebugSystem::update()
void DebugSystem::updateimGUI_(float dt) {

    //show_imGUI_ is a bool toggled directly from Game, using Alt-0
	if (show_imGUI_)
	{
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		//Demo window
		//ImGui::ShowDemoWindow();

		//get input
		ImGuiIO &io = ImGui::GetIO();

		//if imGUI wants the mouse, don't fire picking ray
        //this disables firing picking ray when pointer is
        //over imGUI window
		if (io.WantCaptureMouse)
			can_fire_picking_ray_ = false;
		else
			can_fire_picking_ray_ = true;

		//open window
		ImGui::SetNextWindowSize(ImVec2(400, 200));
		ImGui::SetNextWindowBgAlpha(1.0);
		ImGui::Begin("Scene", &show_imGUI_);

		//Tell imGUI to display variables of the camera
		//get camera and its transform
		Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);
		Transform& cam_transform = ECS.getComponentFromEntity<Transform>(cam.owner);

		//Create an unfoldable tree node called 'Camera'
		if (ImGui::TreeNode("Camera")) {
			//create temporary arrays with position and direction data
			float cam_pos_array[3] = { cam.position.x, cam.position.y, cam.position.z };
			float cam_dir_array[3] = { cam.forward.x, cam.forward.y, cam.forward.z };

			//create imGUI components that allow us to change the values when click-dragging
			ImGui::DragFloat3("Position", cam_pos_array);
			ImGui::DragFloat3("Direction", cam_dir_array);

			//use values of temporary arrays to set real values (in case user changes)
			cam.position = lm::vec3(cam_pos_array[0], cam_pos_array[1], cam_pos_array[2]);
			cam_transform.position(cam.position);
			cam.forward = lm::vec3(cam_dir_array[0], cam_dir_array[1], cam_dir_array[2]).normalize();
			ImGui::TreePop();
		}

		//create a tree of TransformNodes objects (defined in DebugSystem.h)
        //which represents the current scene graph
        
        // 1) create a temporary array with ALL transforms
		std::vector<TransformNode> transform_nodes;
		auto& all_transforms = ECS.getAllComponents<Transform>();
		for (size_t i = 0; i < all_transforms.size(); i++) {
			TransformNode tn;
			tn.trans_id = (int)i;
			tn.entity_owner = all_transforms[i].owner;
			tn.ent_name = ECS.entities[tn.entity_owner].name;
			if (all_transforms[i].parent == -1)
				tn.isTop = true;
			transform_nodes.push_back(tn);
		}
        
		// 2) traverse array to assign children to their parents
		for (size_t i = 0; i < transform_nodes.size(); i++) {
			int parent = all_transforms[i].parent;
			if (parent != -1) {
				transform_nodes[parent].children.push_back(transform_nodes[i]);
			}
		}
        
		// 3) create a new array with only top level nodes of transform tree
		std::vector<TransformNode> transform_topnodes;
		for (size_t i = 0; i < transform_nodes.size(); i++) {
			if (transform_nodes[i].isTop)
				transform_topnodes.push_back(transform_nodes[i]);
		}

        //create 2 imGUI columns, first contains transform tree
        //second contains selected item from picking
		ImGui::Columns(2, "columns");

		//draw all the nodes
		for (auto& trans : transform_topnodes) {
            //this is a recursive function (defined above)
            //which draws a transform node (and its children)
            //using imGUI
			imGuiRenderTransformNode(trans);
		}

        //*** PICKING*** //
        //general approach: Debug System has a member variable which is an entity
        //with Ray Collider (ent_picking_ray_). When user clicks on the screen, this
        //ray is fired into the scene.
        //if it collides with a box collider, we read that collision here and render
        //imGUI with the details of the collider
        
        //look at DebugSystem::setPickingRay_() to see how picking ray is constructed
        
        //next column for picking
		ImGui::NextColumn();
    
		//get the pick ray first
		Collider& pick_ray_collider = ECS.getComponentFromEntity<Collider>(ent_picking_ray_);

		//is it colliding? if so, get pitcked, entity, and transform, and render imGUI text
		int picked_entity = -1;
		if (pick_ray_collider.colliding) {
			//get the other collider and entity
			Collider& picked_collider = ECS.getComponentInArray<Collider>(pick_ray_collider.other);
			picked_entity = picked_collider.owner;
			Transform& picked_transform = ECS.getComponentFromEntity<Transform>(picked_entity);
			ImGui::Text("Selected entity:");
			ImGui::TextColored(ImVec4(1, 1, 0, 1), ECS.entities[picked_collider.owner].name.c_str());
		}


		ImGui::End();

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}

//this function takes a mouse screen point and fires a ray into the world
//using the inverse viewprojection matrux
void DebugSystem::setPickingRay(int mouse_x, int mouse_y, int screen_width, int screen_height) {
	
    //if we are not in debug mode (alt-0) do nothing!
	if (!can_fire_picking_ray_) return;

	//convert mouse_x and mouse_y to NDC
	float ndc_x = (((float)mouse_x / (float)screen_width) * 2) - 1;
	float ndc_y = (((float)(screen_height - mouse_y) / (float)screen_height) * 2) - 1;

    //start point for ray is point on near plane of ndc
	lm::vec4 mouse_near_plane(ndc_x, ndc_y, -1.0, 1.0);
	
    //get view projection
    Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);
	lm::mat4 inv_vp = cam.view_projection;
	inv_vp.inverse();

    //get ray start point in world coordinates
    //don't forget this is in HOMOGENOUS coords :)
	lm::vec4 mouse_world = inv_vp * mouse_near_plane;
    //so we must normalize the vector
	mouse_world.normalize();
	lm::vec3 mouse_world_3(mouse_world.x, mouse_world.y, mouse_world.z);

    //set the picking ray
    //the actual collision detection will be done next frame in the CollisionSystem
	Transform& pick_ray_transform = ECS.getComponentFromEntity<Transform>(ent_picking_ray_);
	Collider& pick_ray_collider = ECS.getComponentFromEntity<Collider>(ent_picking_ray_);
	pick_ray_transform.position(cam.position);
	pick_ray_collider.direction = (mouse_world_3 - cam.position).normalize();
	pick_ray_collider.max_distance = 1000000;
}

///////////////////////////////////////////////
// **** Functions to create geometry ********//
///////////////////////////////////////////////

//creates a simple quad to store a light texture
void DebugSystem::createIcon_() {
	float is = 0.5f;
	GLfloat icon_vertices[12]{ -is, -is, 0, is, -is, 0, is, is, 0, -is, is, 0 };
	GLfloat icon_uvs[8]{ 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
	GLuint icon_indices[6]{ 0, 1, 2, 0, 2, 3 };
	glGenVertexArrays(1, &icon_vao_);
	glBindVertexArray(icon_vao_);
	GLuint vbo;
	//positions
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(icon_vertices), icon_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//uvs
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(icon_uvs), icon_uvs, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(icon_indices), icon_indices, GL_STATIC_DRAW);
	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void DebugSystem::createRay_() {
	//4th component is color
	GLfloat icon_vertices[8]{ 0, 0, 0, 0,
		0, 0, 1, 0 };
	GLuint icon_indices[2]{ 0, 1 };
	glGenVertexArrays(1, &collider_ray_vao_);
	glBindVertexArray(collider_ray_vao_);
	GLuint vbo;
	//positions
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(icon_vertices), icon_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(icon_indices), icon_indices, GL_STATIC_DRAW);
	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void DebugSystem::createCube_() {

	//4th component is color!
	const GLfloat quad_vertex_buffer_data[] = {
		-1.0f,  -1.0f,  -1.0f,  0.0f,  //near bottom left
		1.0f,   -1.0f,  -1.0f,  0.0f,   //near bottom right
		1.0f,   1.0f,   -1.0f,  0.0f,    //near top right
		-1.0f,  1.0f,   -1.0f,  0.0f,   //near top left
		-1.0f,  -1.0f,  1.0f,   0.0f,   //far bottom left
		1.0f,   -1.0f,  1.0f,   0.0f,    //far bottom right
		1.0f,   1.0f,   1.0f,   0.0f,     //far top right
		-1.0f,  1.0f,   1.0f,   0.0f,    //far top left
	};

	const GLuint quad_index_buffer_data[] = {
		0,1, 1,2, 2,3, 3,0, //top
		4,5, 5,6, 6,7, 7,4, // bottom
		4,0, 7,3, //left
		5,1, 6,2, //right
	};

	glGenVertexArrays(1, &cube_vao_);
	glBindVertexArray(cube_vao_);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_buffer_data), quad_vertex_buffer_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_index_buffer_data), quad_index_buffer_data, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//creates the debug grid for our scene
void DebugSystem::createGrid_() {

	std::vector<float> grid_vertices;
	const float size = 100.0f; //outer width and height
	const int div = 100; // how many divisions
	const int halfdiv = div / 2;
	const float step = size / div; // gap between divisions
	const float half = size / 2; // middle of grid

	float p; //temporary variable to store position
	for (int i = 0; i <= div; i++) {

		//lines along z-axis, need to vary x position
		p = -half + (i*step);
		//one end of line
		grid_vertices.push_back(p);
		grid_vertices.push_back(0);
		grid_vertices.push_back(half);
		if (i == halfdiv) grid_vertices.push_back(1); //color
		else grid_vertices.push_back(0);

		//other end of line
		grid_vertices.push_back(p);
		grid_vertices.push_back(0);
		grid_vertices.push_back(-half);
		if (i == halfdiv) grid_vertices.push_back(1); //color
		else grid_vertices.push_back(0);

		//lines along x-axis, need to vary z positions
		p = half - (i * step);
		//one end of line
		grid_vertices.push_back(-half);
		grid_vertices.push_back(0);
		grid_vertices.push_back(p);
		if (i == halfdiv) grid_vertices.push_back(3); //color
		else grid_vertices.push_back(0);

		//other end of line
		grid_vertices.push_back(half);
		grid_vertices.push_back(0);
		grid_vertices.push_back(p);
		if (i == halfdiv) grid_vertices.push_back(3); //color
		else grid_vertices.push_back(0);
	}

	//indices
	const int num_indices = (div + 1) * 4;
	GLuint grid_line_indices[num_indices];
	for (int i = 0; i < num_indices; i++)
		grid_line_indices[i] = i;

	grid_num_indices = num_indices;

	//gl buffers
	glGenVertexArrays(1, &grid_vao_);
	glBindVertexArray(grid_vao_);
	GLuint vbo;
	//positions
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, grid_vertices.size() * sizeof(float), &(grid_vertices[0]), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	//indices
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(grid_line_indices), grid_line_indices, GL_STATIC_DRAW);

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

