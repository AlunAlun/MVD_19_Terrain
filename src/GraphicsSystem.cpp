//
//  Copyright 2018 Alun Evans. All rights reserved.
//
#include "GraphicsSystem.h"
#include "Parsers.h"
#include "extern.h"
#include <algorithm>

//destructor
GraphicsSystem::~GraphicsSystem() {
	//delete shader pointers
	for (auto shader_pair : shaders_) {
		if (shader_pair.second)
			delete shader_pair.second;
	}
}

//set initial state of graphics system
void GraphicsSystem::init(int window_width, int window_height, std::string assets_folder) {

	screen_background_color = lm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    updateMainViewport(window_width, window_height);
    
    //enable culling and depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); //for cubemap optimization
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    //enable seamless cubemap sampling
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    
	//set assets folder
    assets_folder_ = assets_folder;

	//generate light ubo
	glGenBuffers(1, &light_ubo_);


	//screen space geometry
	Geometry ss_geom;
	ss_geom.createPlaneGeometry();
	geometries_.push_back(ss_geom);
	screen_space_geom_ = (int)(geometries_.size() - 1);
    
    sphere_volume_geom_ = createGeometryFromFile("data/assets/sphere.obj");
    
    cone_volume_geom_ = createGeometryFromFile("data/assets/cone.obj");

    //screen space texture shader
    screen_space_shader_ = new Shader("data/shaders/screen.vert", "data/shaders/screen.frag");
    
	//screen space depth shader
	screen_depth_shader_ = new Shader("data/shaders/screen.vert", "data/shaders/screen_depth.frag");

	//shadow map shader
	depth_shader_ = new Shader("data/shaders/depth.vert", "data/shaders/depth.frag");

    //gbuffer stuff
    gbuffer_shader_ = new Shader("data/shaders/gbuffer.vert", "data/shaders/gbuffer.frag");
    deferred_shader_ = new Shader("data/shaders/deferred.vert", "data/shaders/deferred.frag");
    deferred_volume_shader_ = new Shader("data/shaders/deferred_volume.vert", "data/shaders/deferred_volume.frag");
    gbuffer_.initGbuffer(window_width, window_height);
    
	
}

//called after loading everything
void GraphicsSystem::lateInit() {
	// sort meshes initially
    sortMeshes_();

	//create shadow buffers depending on number of lights
	for (size_t i = 0; i < ECS.getAllComponents<Light>().size(); i++) {
		shadow_frame_[i].initDepth(2048, 2048);
	}

}

void GraphicsSystem::update(float dt) {
    
	updateAllCameras_();

	if (needUpdateLights)
		updateLights_();
    
	/* SHADOW PASS FOR ALL LIGHTS */
	glCullFace(GL_FRONT);
	useShader(depth_shader_);
	const auto& lights = ECS.getAllComponents<Light>();
	for (size_t i = 0; i < lights.size(); i++) {
		shadow_frame_[i].bindAndClear();
		auto& mesh_components = ECS.getAllComponents<Mesh>();
		for (auto &curr_comp : mesh_components) {
			renderDepth_(curr_comp, lights[i]);
		}
	}
	glCullFace(GL_BACK);

    /* GBUFFER PASS */
    gbuffer_.bindAndClear(screen_background_color);
    useShader(gbuffer_shader_);
    for (auto &mesh : ECS.getAllComponents<Mesh>()) {
        if (mesh.render_mode != RenderModeDeferred)
            continue;
        checkMaterial_(mesh);
        renderMeshComponent_(mesh);
    }
    
	/* SCREEN BUFFER */
	bindAndClearScreen_();
    resetShaderAndMaterial_();
    
    /* GBUFFER OR LIGHT VOLUMES */
    //renderGbuffer();
    renderLightVolumes();
    
    /* FORWARD RENDERING */
    for (auto &mesh : ECS.getAllComponents<Mesh>()) {
        if (mesh.render_mode != RenderModeForward)
            continue;
        checkShaderAndMaterial_(mesh);
        renderMeshComponent_(mesh);
    }
    
    /* ENVIRONMENT */
    renderEnvironment_();
    
	/* VIEW FRAMES */
    //previewTextureViewport(gbuffer_.color_textures[2]);
    
}

void GraphicsSystem::previewTextureViewport(GLuint texture_id) {
    glDisable(GL_DEPTH_TEST);
    useShader(screen_space_shader_);
    glViewport(0, 0, GLsizei(viewport_width_/4), GLsizei(viewport_height_/4));
    screen_space_shader_->setTexture(U_SCREEN_TEXTURE, texture_id, 0);
    geometries_[screen_space_geom_].render();
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, GLsizei(viewport_width_), GLsizei(viewport_height_));
}

void GraphicsSystem::renderLightVolumes() {
    
    //activate shader
    useShader(deferred_volume_shader_);
    
    //set uniforms common for all light passes
    auto lights = ECS.getAllComponents<Light>();
    for (size_t i = 0; i < lights.size(); i++) {
        //this static cast assumes shadowmap enums are consecutive
        UniformID new_enum = static_cast<UniformID>((int)U_SHADOW_MAP0 + (int)i);
        shader_->setTexture(new_enum, shadow_frame_[i].color_textures[0], (int)i);
    }
    shader_->setUniformBlock(U_LIGHTS_UBO, LIGHTS_BINDING_POINT);
    shader_->setTexture(U_TEX_POSITION, gbuffer_.color_textures[0], 8);
    shader_->setTexture(U_TEX_NORMAL, gbuffer_.color_textures[1], 9);
    shader_->setTexture(U_TEX_ALBEDO, gbuffer_.color_textures[2], 10);
    shader_->setUniform(U_CAM_POS, ECS.getComponentInArray<Camera>(ECS.main_camera).position);
    
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    //render directional 
    for (size_t i = 0; i < lights.size(); i++) {
        if (lights[i].type == 0) {
            //set light id
            shader_->setUniform(U_LIGHT_ID,(int)i);
            //mvp is simple identity pass through
            lm::mat4 mvp;
            shader_->setUniform(U_MVP, mvp);
            //draw
            geometries_[screen_space_geom_].render();
        }
    }
    
    for (size_t i = 0; i < lights.size(); i++) {
        if (lights[i].type == 2) {
            //set light id
            shader_->setUniform(U_LIGHT_ID,(int)i);
            
            //set u_mvp
            lm::vec3 light_pos = ECS.getComponentFromEntity<Transform>(lights[i].owner).position();
            lm::mat4 model;
            
            float cone_width_scale =
                lights[i].radius * tanf(DEG2RAD*lights[i].spot_outer*2);
            
            
            model.scale(cone_width_scale, lights[i].radius, cone_width_scale);
           
            lm::vec3 minus = lights[i].forward.normalize();
            
            float angle = minus.dot(lm::vec3(0,1,0));
            lm::vec3 axis = lm::vec3(0,1,0).cross(minus);
            
            lm::mat4 rotate_matrix;
            rotate_matrix.makeRotationMatrix(angle*4, axis);
            model = rotate_matrix * model;
            model.translate(light_pos);
            
            lm::mat4 view_projection = ECS.getComponentInArray<Camera>(ECS.main_camera).view_projection;
            lm::mat4 mvp = view_projection * model;
            shader_->setUniform(U_MVP, mvp);
            //draw
            glCullFace(GL_FRONT);
            geometries_[cone_volume_geom_].render();
            glCullFace(GL_BACK);
        }
    }
    
    //now render point lights
    for (size_t i = 0; i < lights.size(); i++) {
        if (lights[i].type != 1)
            continue;
        //set light id
        shader_->setUniform(U_LIGHT_ID,(int)i);
        
        //set u_mvp
        lm::vec3 light_pos = ECS.getComponentFromEntity<Transform>(lights[i].owner).position();
        lm::mat4 model;
        model.scale(lights[i].radius, lights[i].radius, lights[i].radius);
        model.translate(light_pos);
        lm::mat4 view_projection = ECS.getComponentInArray<Camera>(ECS.main_camera).view_projection;
        lm::mat4 mvp = view_projection * model;
        shader_->setUniform(U_MVP, mvp);
        
        //draw
        glCullFace(GL_FRONT);
        geometries_[sphere_volume_geom_].render();
        glCullFace(GL_BACK);
    }
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    
    //blit depth
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer_.framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
    glBlitFramebuffer(0, 0, viewport_width_, viewport_height_, 0, 0, viewport_width_, viewport_height_, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void GraphicsSystem::renderGbuffer() {
    
    //activate shader
    useShader(deferred_shader_);
    
    auto lights = ECS.getAllComponents<Light>();
    for (size_t i = 0; i < lights.size(); i++) {
        //this static cast assumes shadowmap enums are consecutive
        UniformID new_enum = static_cast<UniformID>((int)U_SHADOW_MAP0 + (int)i);
        shader_->setTexture(new_enum, shadow_frame_[i].color_textures[0], (int)i);
    }
    
    //set light uniforms
    shader_->setUniformBlock(U_LIGHTS_UBO, LIGHTS_BINDING_POINT);
    shader_->setUniform(U_NUM_LIGHTS,
                                 (int)ECS.getAllComponents<Light>().size());
    
    //gbuffer textures
    shader_->setTexture(U_TEX_POSITION, gbuffer_.color_textures[0], 8);
    shader_->setTexture(U_TEX_NORMAL, gbuffer_.color_textures[1], 9);
    shader_->setTexture(U_TEX_ALBEDO, gbuffer_.color_textures[2], 10);
    shader_->setUniform(U_CAM_POS, ECS.getComponentInArray<Camera>(ECS.main_camera).position);
    
    //draw
    geometries_[screen_space_geom_].render();
    
    //blit depth
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer_.framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
    glBlitFramebuffer(0, 0, viewport_width_, viewport_height_, 0, 0, viewport_width_, viewport_height_, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

//renders a mesh from a Light/Camera, only setting its MVP
//i.e. only usable with a depth shader
void GraphicsSystem::renderDepth_(Mesh& comp, const Light& light) {
	//get transform and matrices
	Transform& transform = ECS.getComponentFromEntity<Transform>(comp.owner);
	lm::mat4 model_matrix = transform.getGlobalMatrix(ECS.getAllComponents<Transform>());
	lm::mat4 mvp_matrix = light.view_projection * model_matrix;
	//set sole uniform
	depth_shader_->setUniform(U_MVP, mvp_matrix);
	//render
	geometries_[comp.geometry].render();

}

//renders a given mesh component
void GraphicsSystem::renderMeshComponent_(Mesh& comp) {

	//get components and geom
	Transform& transform = ECS.getComponentFromEntity<Transform>(comp.owner);
	Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);
	Geometry& geom = geometries_[comp.geometry];

	//create mvp
	lm::mat4 model_matrix = transform.getGlobalMatrix(ECS.getAllComponents<Transform>());
	lm::mat4 mvp_matrix = cam.view_projection * model_matrix;

	//view frustum culling
	if (!BBInFrustum_(geom.aabb, mvp_matrix)) {
		return;
	}

	//normal matrix
	lm::mat4 normal_matrix = model_matrix;
	normal_matrix.inverse();
	normal_matrix.transpose();

	//transform uniforms
	shader_->setUniform(U_MVP, mvp_matrix);
	shader_->setUniform(U_MODEL, model_matrix);
	shader_->setUniform(U_NORMAL_MATRIX, normal_matrix);
	shader_->setUniform(U_CAM_POS, cam.position);

    //draw raw geom if no material sets
    if (geom.material_sets.size() == 0)
        geom.render();
    else {
        //loop material sets
        for (int i = 0; i < geom.material_sets.size(); i++) {
            //set current material id of set
            current_material_ = geom.material_set_ids[i];
            setMaterialUniforms();
            //render current set
            geom.render(i);
        }
    }
}

//render the skybox as a cubemap
void GraphicsSystem::renderEnvironment_() {
    
	//render cubemap only if we have both a shader, texture, and geometry
	if (!environment_program_ || !environment_tex_ || cube_map_geom_ < 0) return;

    //set shader
    useShader(environment_program_);
    
    //get camera
    Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);
        
    //view projection matrix, zeroing out
    lm::mat4 view_matrix = cam.view_matrix;
    view_matrix.m[12] = view_matrix.m[13] = view_matrix.m[14] = 0; view_matrix.m[15] = 1;
    lm::mat4 vp_matrix = cam.projection_matrix * view_matrix;

    //set vp uniform and texture
    shader_->setUniform(U_VP, vp_matrix);
    
    //bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environment_tex_);

	//no need to set sampler id, as it will default to 0
    
    // disable depth test, cull front faces (to draw inside of mesh)
    glDepthMask(false);
    glCullFace(GL_FRONT);
    
	geometries_[cube_map_geom_].render();
    
    // reset depth test and culling
    glDepthMask(true);
    glCullFace(GL_BACK);
    
}

//checks to see if current shader and material are
//the ones need for mesh passed as parameter
//if not, change them
void GraphicsSystem::checkShaderAndMaterial_(Mesh& mesh) {
    //get shader id from material. if same, don't change
    if (!shader_ || shader_->program != materials_[mesh.material].shader_id) {
		useShader(materials_[mesh.material].shader_id);
    }
    //set material uniforms if required
    if (current_material_ != mesh.material) {
        current_material_ = mesh.material;
        setMaterialUniforms();
    }
}

void GraphicsSystem::checkMaterial_(Mesh& mesh) {
    //set material uniforms if required
    if (current_material_ != mesh.material) {
        current_material_ = mesh.material;
        setMaterialUniforms();
    }
}

//sets uniforms for current material and current shader
void GraphicsSystem::setMaterialUniforms() {
    Material& mat = materials_[current_material_];

    //material uniforms
	shader_->setUniform(U_AMBIENT, mat.ambient);
	shader_->setUniform(U_DIFFUSE, mat.diffuse);
	shader_->setUniform(U_SPECULAR, mat.specular);
	shader_->setUniform(U_SPECULAR_GLOSS, mat.specular_gloss);
    shader_->setUniform(U_UV_SCALE, mat.uv_scale);
    shader_->setUniform(U_NORMAL_FACTOR, mat.normal_factor);
    shader_->setUniform(U_MAX_HEIGHT, mat.height);
    
    //texture uniforms - diffuse
    if (mat.diffuse_map != -1){
        shader_->setUniform(U_USE_DIFFUSE_MAP, 1);
        shader_->setTexture(U_DIFFUSE_MAP, mat.diffuse_map, 8);
    } else shader_->setUniform(U_USE_DIFFUSE_MAP, 0);
    
    //add extra diffuse maps
    if (mat.diffuse_map_2 != -1) {
        shader_->setUniform(U_USE_DIFFUSE_MAP_2, 1);
        shader_->setTexture(U_DIFFUSE_MAP_2, mat.diffuse_map_2, 9);
    }
    else shader_->setUniform(U_USE_DIFFUSE_MAP_2, 0);
    if (mat.diffuse_map_3 != -1) {
        shader_->setUniform(U_USE_DIFFUSE_MAP_3, 1);
        shader_->setTexture(U_DIFFUSE_MAP_3, mat.diffuse_map_3, 10);
    }
    else shader_->setUniform(U_USE_DIFFUSE_MAP_3, 0);
    //normal
    if (mat.normal_map != -1) {
        shader_->setUniform(U_USE_NORMAL_MAP, 1);
        shader_->setTexture(U_NORMAL_MAP, mat.normal_map, 11);
    } else shader_->setUniform(U_USE_NORMAL_MAP, 0);
    //specular
    if (mat.specular_map != -1) {
        shader_->setUniform(U_USE_SPECULAR_MAP, 1);
        shader_->setTexture(U_SPECULAR_MAP, mat.specular_map, 12);
    } else shader_->setUniform(U_USE_SPECULAR_MAP, 0);
    //reflection
    if (mat.cube_map) {
        shader_->setUniform(U_USE_REFLECTION_MAP, 1);
        shader_->setTextureCube(U_SKYBOX, mat.cube_map, 13);
    } else shader_->setUniform(U_USE_REFLECTION_MAP, 0);
    //noise map
    if (mat.noise_map != -1) {
        shader_->setUniform(U_USE_NOISE_MAP, 1);
        shader_->setTexture(U_NOISE_MAP, mat.noise_map, 14);
    }
    else shader_->setUniform(U_USE_NOISE_MAP, 0);

	auto lights = ECS.getAllComponents<Light>();
	for (size_t i = 0; i < lights.size(); i++) {

		glActiveTexture(GL_TEXTURE0 + (GLenum)i);
		glBindTexture(GL_TEXTURE_2D, shadow_frame_[i].color_textures[0]);

		std::string shadow_map_name = "u_shadow_map[" + std::to_string(i) + "]";
		GLint u_shadow_map_pos = glGetUniformLocation(shader_->program, shadow_map_name.c_str());
		if (u_shadow_map_pos != -1)
			glUniform1i(u_shadow_map_pos, (GLint)i);
	}
    
	//light uniforms
    shader_->setUniformBlock(U_LIGHTS_UBO, LIGHTS_BINDING_POINT);
	shader_->setUniform(U_NUM_LIGHTS, (int)lights.size());
}

//updates light ubo
void GraphicsSystem::updateLights_() {
	const std::vector<Light>& lights = ECS.getAllComponents<Light>();

	// 3 * vec4, 4 * float, 1 x matrix, 1 * int, which is blocked out to 16 bytes
	GLsizeiptr size_lights_ubo = (16 + 16 + 16 + 16 + 16 + 64) * lights.size();

	glBindBuffer(GL_UNIFORM_BUFFER, light_ubo_);
	glBufferData(GL_UNIFORM_BUFFER, size_lights_ubo, NULL, GL_STATIC_DRAW);

	GLsizeiptr offset = 0; //pointer to top of buffer

	for (auto& l : lights) {
		Transform& lt = ECS.getComponentFromEntity<Transform>(l.owner);

		float spot_inner_cosine = cos((l.spot_inner*DEG2RAD) / 2.0f);
		float spot_outer_cosine = cos((l.spot_outer*DEG2RAD) / 2.0f);

		GLfloat light_data[16] = {
			lt.m[12], lt.m[13], lt.m[14], 0.0,
			l.direction.x, l.direction.y, l.direction.z, 0.0,
			l.color.x, l.color.y, l.color.z, 0.0,
			l.linear_att,l.quadratic_att,spot_inner_cosine,spot_outer_cosine
		};
		//vec4s and floats data
		glBufferSubData(GL_UNIFORM_BUFFER, offset, 64, light_data);
		offset += 64;
        //light matrix
        glBufferSubData(GL_UNIFORM_BUFFER, offset, 64, l.view_projection.m);
        offset += 64;
		//type
		glBufferSubData(GL_UNIFORM_BUFFER, offset, 4, &(l.type));
		offset += 4;
        //type
        glBufferSubData(GL_UNIFORM_BUFFER, offset, 4, &(l.cast_shadow));
        offset += 12;
	}

	glBindBufferRange(GL_UNIFORM_BUFFER, LIGHTS_BINDING_POINT, light_ubo_, 0, size_lights_ubo);

	needUpdateLights = false;
}

//This function executes two sorts:
// i) sorts materials array by shader_id
// ii) sorts Mesh components by material id
//the result is that the mesh component array is
//ordered by both shader and material
void GraphicsSystem::sortMeshes_() {

	//sort materials by shader id
	//first we store the old index of each material in materials_ array
	for (size_t i = 0; i < materials_.size(); i++)
		materials_[i].index = (int)i; // 'index' is a new property of Material

	//second, we sort materials by shader_id
	std::sort(materials_.begin(), materials_.end(), [](const Material& a, const Material& b) {
		return a.shader_id < b.shader_id;
	});

	//now we map old indices to new indices
	std::map<int, int> old_new;
	for (size_t i = 0; i < materials_.size(); i++) {
		old_new[materials_[i].index] = (int)i;
	}

	//now we swap index of materials in all meshes
	auto& meshes = ECS.getAllComponents<Mesh>();
	for (auto& mesh : meshes) {
		int old_index = mesh.material;
		int new_index = old_new[old_index];
		mesh.material = new_index;
	}

	//store old mesh indices
	for (size_t i = 0; i < meshes.size(); i++)
		meshes[i].index = (int)i;

	//short meshes by material id
	std::sort(meshes.begin(), meshes.end(), [](const Mesh& a, const Mesh& b) {
		return a.material < b.material;
	});

	//clear map and refill with mesh index map
	old_new.clear();
	for (size_t i = 0; i < meshes.size(); i++) {
		old_new[meshes[i].index] = (int)i;
	}

	//update all entities with new mesh id
	auto& all_entities = ECS.entities;
	for (auto& ent : ECS.entities) {
		int old_index = ent.components[type2int<Mesh>::result];
		int new_index = old_new[old_index];
		ent.components[type2int<Mesh>::result] = new_index;
	}
}

//reset shader and material
void GraphicsSystem::resetShaderAndMaterial_() {
	
	useShader((GLuint)0);
	current_material_ = -1;
}

//update cameras
void GraphicsSystem::updateAllCameras_() {

	auto& cameras = ECS.getAllComponents<Camera>();
	for (auto &cam : cameras) cam.update();
}

void GraphicsSystem::bindAndClearScreen_() {
	glViewport(0, 0, viewport_width_, viewport_height_);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(screen_background_color.x, screen_background_color.y, screen_background_color.z, screen_background_color.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//change shader only if need to - note shader object must be in shaders_ map
//s - pointer to a shader object
void GraphicsSystem::useShader(Shader* s) {
	if (!s) {
		glUseProgram(0);
		shader_ = nullptr;
	}
	else if (!shader_ || shader_ != s) {
		glUseProgram(s->program);
		shader_ = s;
	}
}

//change shader only if need to - note shader object must be in shaders_ map
//p - GL id of shader
void GraphicsSystem::useShader(GLuint p) {
	if (!p) {
		glUseProgram(0);
		shader_ = nullptr;
	}
	else if (!shader_ || shader_->program != p) {
		glUseProgram(p);
		shader_ = shaders_[p];
	}
}

//sets internal variables
void GraphicsSystem::setEnvironment(GLuint tex_id, int geom_id, GLuint program) {

	//set cubemap geometry
	cube_map_geom_ = geom_id;

	//set cube faces
	environment_tex_ = tex_id;

	//set shader program
	environment_program_ = program;
}

//
////********************************************
//// Adding and creating functions
////********************************************

//loads a shader, stores it in a map where key is it's program id, and returns a pointer to shader
//-vs: either the path to the vertex shader, or the vertex shader string
//-fs: either the path to the fragment shader, or the fragment shader string
//-compile_direct: if false, assume other two parameters are paths, if true, assume they are shader strings
Shader* GraphicsSystem::loadShader(std::string vs, std::string fs, bool compile_direct) {
	Shader* new_shader;
	if (compile_direct) {
		new_shader = new Shader();
		new_shader->compileFromStrings(vs, fs);
	}
	else {
		new_shader = new Shader(vs, fs);
	}
	shaders_[new_shader->program] = new_shader;
	return new_shader;
}

//create a new material and return pointer to it
int GraphicsSystem::createMaterial() {
    materials_.emplace_back();
    return (int)materials_.size() - 1;
}


//create geometry from
//returns index in geometry array with stored geometry data
int GraphicsSystem::createGeometryFromFile(std::string filename) {
    
    std::vector<GLfloat> vertices, uvs, normals;
    std::vector<GLuint> indices;
    //check for supported format
    std::string ext = filename.substr(filename.size() - 4, 4);
    if (ext == ".obj" || ext == ".OBJ")
    {
        //fill it with data from object
        if (Parsers::parseOBJ(filename, vertices, uvs, normals, indices)) {
        
            //generate the OpenGL buffers and create geometry
			Geometry new_geom(vertices, uvs, normals, indices);
            geometries_.emplace_back(new_geom);

            return (int)geometries_.size() - 1;
        }
        else {
            std::cerr << "ERROR: Could not parse mesh file" << std::endl;
            return -1;
        }
    }
    else {
        std::cerr << "ERROR: Unsupported mesh format when creating geometry" << std::endl;
        return -1;
    }
    
}




int GraphicsSystem::createMultiGeometryFromFile(std::string filename) {
    
    std::vector<GLfloat> vertices, uvs, normals;
    std::vector<GLuint> indices;
    //check for supported format
    std::string ext = filename.substr(filename.size() - 4, 4);
    if (ext == ".obj" || ext == ".OBJ")
    {
        //fill it with data from object
        if (int p = Parsers::parseOBJ_multi(filename, geometries_, materials_) ) {
            return p;
        }
        else {
            std::cerr << "ERROR: Could not parse mesh file" << std::endl;
            return -1;
        }
    }
    else {
        std::cerr << "ERROR: Unsupported mesh format when creating geometry" << std::endl;
        return -1;
    }
    
}

//create terrain geometry and adds to geometry array
int GraphicsSystem::createTerrainGeometry(int resolution, float step, float max_height, ImageData& height_map) {
    Geometry new_geom;
    new_geom.createTerrain(resolution, step, max_height, height_map);
    geometries_.emplace_back(new_geom);
    return (int)geometries_.size() - 1;
}

// Given an array of floats (in sets of three, representing vertices) calculates and
// sets the AABB of a geometry
void GraphicsSystem::setGeometryAABB_(Geometry& geom, std::vector<GLfloat>& vertices) {
	//set very max and very min
	float big = 1000000.0f;
	float small = -1000000.0f;
	lm::vec3 min(big, big, big);
	lm::vec3 max(small, small, small);

	//for all verts, find max and min
	for (size_t i = 0; i < vertices.size(); i += 3) {
		float x = vertices[i];
		float y = vertices[i + 1];
		float z = vertices[i + 2];

		if (x < min.x) min.x = x;
		if (y < min.y) min.y = y;
		if (z < min.z) min.z = z;

		if (x > max.x) max.x = x;
		if (y > max.y) max.y = y;
		if (z > max.z) max.z = z;
	}
	//set center and halfwidth based on max and min
	geom.aabb.center = lm::vec3(
		(min.x + max.x) / 2,
		(min.y + max.y) / 2,
		(min.z + max.z) / 2);
	geom.aabb.half_width = lm::vec3(
		max.x - geom.aabb.center.x,
		max.y - geom.aabb.center.y,
		max.z - geom.aabb.center.z);
}

//relcalculates AABB from OOB
AABB GraphicsSystem::transformAABB_(const AABB& aabb, const lm::mat4& transform) {
	//get aabb min and max
	lm::vec3 aabb_min(
		aabb.center.x - aabb.half_width.x,
		aabb.center.y - aabb.half_width.y,
		aabb.center.z - aabb.half_width.z);
	lm::vec3 aabb_max(
		aabb.center.x + aabb.half_width.x,
		aabb.center.y + aabb.half_width.y,
		aabb.center.z + aabb.half_width.z);

	//transform min and max
	lm::vec3 min_t = transform * aabb_min;
	lm::vec3 max_t = transform * aabb_max;

	//create array of coords for easier parsing
	float vertices[6] = { min_t.x, min_t.y, min_t.z, max_t.x, max_t.y, max_t.z };

	//set very max and very min
	float big = 1000000.0f;
	float small = -1000000.0f;
	lm::vec3 min(big, big, big);
	lm::vec3 max(small, small, small);

	//calculate new aabb based on transformed verts
	for (int i = 0; i < 6; i += 3) {
		float x = vertices[i];
		float y = vertices[i + 1];
		float z = vertices[i + 2];

		if (x < min.x) min.x = x;
		if (y < min.y) min.y = y;
		if (z < min.z) min.z = z;

		if (x > max.x) max.x = x;
		if (y > max.y) max.y = y;
		if (z > max.z) max.z = z;
	}

	AABB new_aabb;
	//set new center and halfwidth based on max and min
	new_aabb.center = lm::vec3(
		(min.x + max.x) / 2,
		(min.y + max.y) / 2,
		(min.z + max.z) / 2);
	new_aabb.half_width = lm::vec3(
		max.x - new_aabb.center.x,
		max.y - new_aabb.center.y,
		max.z - new_aabb.center.z);

	return new_aabb;
}


//tests whether AABB or OOB is inside frustum or not, based on view_projection matrix
bool GraphicsSystem::AABBInFrustum_(const AABB& aabb, const lm::mat4& to_clip) {
	lm::vec4 points[8];
	points[0] = lm::vec4(aabb.center.x - aabb.half_width.x, aabb.center.y - aabb.half_width.y, aabb.center.z - aabb.half_width.z, 1.0);
	points[1] = lm::vec4(aabb.center.x - aabb.half_width.x, aabb.center.y - aabb.half_width.y, aabb.center.z + aabb.half_width.z, 1.0);
	points[2] = lm::vec4(aabb.center.x - aabb.half_width.x, aabb.center.y + aabb.half_width.y, aabb.center.z - aabb.half_width.z, 1.0);
	points[3] = lm::vec4(aabb.center.x - aabb.half_width.x, aabb.center.y + aabb.half_width.y, aabb.center.z + aabb.half_width.z, 1.0);
	points[4] = lm::vec4(aabb.center.x + aabb.half_width.x, aabb.center.y - aabb.half_width.y, aabb.center.z - aabb.half_width.z, 1.0);
	points[5] = lm::vec4(aabb.center.x + aabb.half_width.x, aabb.center.y - aabb.half_width.y, aabb.center.z + aabb.half_width.z, 1.0);
	points[6] = lm::vec4(aabb.center.x + aabb.half_width.x, aabb.center.y + aabb.half_width.y, aabb.center.z - aabb.half_width.z, 1.0);
	points[7] = lm::vec4(aabb.center.x + aabb.half_width.x, aabb.center.y + aabb.half_width.y, aabb.center.z + aabb.half_width.z, 1.0);

	//transform to clip space
	lm::vec4 clip_points[8];
	for (int i = 0; i < 8; i++) {
		clip_points[i] = to_clip * points[i];
	}

	//now test clip points against each plane. If all clip points are outside plane we return false
	//left plane
	int in = 0;
	for (int i = 0; i < 8; i++) {
		if (-clip_points[i].w < clip_points[i].x) in++;
	}
	if (!in) return false;

	//right plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (clip_points[i].x < clip_points[i].w) in++;
	}
	if (!in) return false;

	//bottom plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (-clip_points[i].w < clip_points[i].y) in++;
	}
	if (!in) return false;

	//top plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (clip_points[i].y < clip_points[i].w) in++;
	}
	if (!in) return false;

	//near plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (-clip_points[i].z < clip_points[i].z) in++;
	}
	if (!in) return false;

	//far plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (clip_points[i].z < clip_points[i].w) in++;
	}
	if (!in) return false;

	return true;
}

//tests whether Bounding box is inside frustum or not, based on model_view_projection matrix
bool GraphicsSystem::BBInFrustum_(const AABB& aabb, const lm::mat4& mvp) {
    //each corner point of box gets transformed into clip space, to give point PC, in HOMOGENOUS coords
    //point is inside clip space iff
    //-PC.w < PC.xyz < PC.w
    //so we first take each corner of AABB (note, using vec4 because of homogenous coords) and multiply
    //by matrix to clip space. Then we test each point against the 6 planes e.g. PC is on the 'right' side
    //of the left plane iff -PC.w < PC.x; and is on 'left' side of right plane is PC.x < PC.w etc.
    //For more info see:
    //http://www.lighthouse3d.com/tutorials/view-frustum-culling/clip-space-approach-extracting-the-planes/
    
    
    //the eight points of the box corners are calculated using center and +/- halfwith:
    //- - -
    //- - +
    //- + -
    //- + +
    //+ - -
    //+ - +
    //+ + -
    //+ + +
	lm::vec4 points[8];
	points[0] = lm::vec4(aabb.center.x - aabb.half_width.x, aabb.center.y - aabb.half_width.y, aabb.center.z - aabb.half_width.z, 1.0);
	points[1] = lm::vec4(aabb.center.x - aabb.half_width.x, aabb.center.y - aabb.half_width.y, aabb.center.z + aabb.half_width.z, 1.0);
	points[2] = lm::vec4(aabb.center.x - aabb.half_width.x, aabb.center.y + aabb.half_width.y, aabb.center.z - aabb.half_width.z, 1.0);
	points[3] = lm::vec4(aabb.center.x - aabb.half_width.x, aabb.center.y + aabb.half_width.y, aabb.center.z + aabb.half_width.z, 1.0);
	points[4] = lm::vec4(aabb.center.x + aabb.half_width.x, aabb.center.y - aabb.half_width.y, aabb.center.z - aabb.half_width.z, 1.0);
	points[5] = lm::vec4(aabb.center.x + aabb.half_width.x, aabb.center.y - aabb.half_width.y, aabb.center.z + aabb.half_width.z, 1.0);
	points[6] = lm::vec4(aabb.center.x + aabb.half_width.x, aabb.center.y + aabb.half_width.y, aabb.center.z - aabb.half_width.z, 1.0);
	points[7] = lm::vec4(aabb.center.x + aabb.half_width.x, aabb.center.y + aabb.half_width.y, aabb.center.z + aabb.half_width.z, 1.0);

	//transform to clip space
	lm::vec4 clip_points[8];
	for (int i = 0; i < 8; i++) {
		clip_points[i] = mvp * points[i];
	}

	//now test clip points against each plane. If all clip points are outside plane we return false
	//left plane
	int in = 0;
	for (int i = 0; i < 8; i++) {
		if (-clip_points[i].w < clip_points[i].x) in++;
	}
	if (!in) return false;

	//right plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (clip_points[i].x < clip_points[i].w) in++;
	}
	if (!in) return false;

	//bottom plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (-clip_points[i].w < clip_points[i].y) in++;
	}
	if (!in) return false;

	//top plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (clip_points[i].y < clip_points[i].w) in++;
	}
	if (!in) return false;

	//near plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (-clip_points[i].z < clip_points[i].z) in++;
	}
	if (!in) return false;

	//far plane
	in = 0;
	for (int i = 0; i < 8; i++) {
		if (clip_points[i].z < clip_points[i].w) in++;
	}
	if (!in) return false;

	return true;
}

//sets viewport of graphics system
void GraphicsSystem::updateMainViewport(int window_width, int window_height) {
    glViewport(0, 0, window_width, window_height);
    viewport_width_ = window_width;
    viewport_height_ = window_height;
}

//returns values of window width and height by reference
void GraphicsSystem::getMainViewport(int& width, int& height){
    width = viewport_width_; height = viewport_height_;
}

