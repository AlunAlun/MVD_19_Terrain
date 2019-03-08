#include "GraphicsUtilities.h"

// ****** GEOMETRY ***** //

//generates buffers in VRAM
Geometry::Geometry(std::vector<float>& vertices, std::vector<float>& uvs, std::vector<float>& normals, std::vector<unsigned int>& indices) {
	createVertexArrays(vertices, uvs, normals, indices);
}

void Geometry::render() {
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, num_tris * 3, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}


void Geometry::render(int set) {
    //bind the vao
    glBindVertexArray(vao);
    //if first set, draw from start to "end of set 0" (* 3 to convert from triangles
    //to indices)
    if (set == 0)
        glDrawElements(GL_TRIANGLES, material_sets[set] * 3, GL_UNSIGNED_INT, 0);
    else {
        //start triangle is end triangle of previous set
        GLuint start_index = material_sets[set - 1] * 3;
        //end triangle is end of current set
        GLuint end_index = material_sets[set] * 3;
        //count is the number of indices to draw
        GLuint count = end_index - start_index;
        
        glDrawElements(GL_TRIANGLES, //things to draw
                       count, //number of indices
                       GL_UNSIGNED_INT, //format of indices
                       (void*)(start_index * sizeof(GLuint))); //pointer to start!
    }
    glBindVertexArray(0);
}

void Geometry::createMaterialSet(int tri_count, int material_id) {
    material_sets.push_back(tri_count);
    material_set_ids.push_back(material_id);
}

void Geometry::createVertexArrays(std::vector<float>& vertices, std::vector<float>& uvs, std::vector<float>& normals, std::vector<unsigned int>& indices) {
    
    
	//generate and bind vao
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	GLuint vbo;
	//positions
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &(vertices[0]), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//texture coords
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(float), &(uvs[0]), GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//normals
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &(normals[0]), GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &(indices[0]), GL_STATIC_DRAW);
	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//set number of triangles
	num_tris = (GLuint)indices.size() / 3;

	//set AABB
	setAABB(vertices);
}

int Geometry::createTerrain(int resolution, float step, float the_max_height, ImageData& height_map){
    //set max_height of geometry
    max_terrain_height = the_max_height;
    
    std::vector<GLfloat> vertices, uvs, normals;
    std::vector<GLuint> indices;
    
    int height_value[3];
    height_map.getPixel(0, 0, height_value);
    
    float hw = ((float)resolution * step) / 2; //halfwidth, used to center terrain
    
    for (int vx = 0; vx < resolution; vx++) {
        for (int vz = 0; vz < resolution; vz++) {
                        
            float x_pos = ((float)vx * step) - hw;
			float y_pos =  0.0;
            float z_pos = ((float)-vz * step) + hw;
            
            GLfloat v[] = { x_pos, y_pos, z_pos };
			lm::vec3 nvec = lm::vec3(0,1,0); // calculateTerrainNormal(height_map, x_pixel, y_pixel);
            GLfloat* n = nvec.value_;
            GLfloat uv[] = { (float)vx / (float)(resolution -1), (float)vz / (float)(resolution -1) };
            
            vertices.insert(vertices.end(), v, v + 3);
            normals.insert(normals.end(), n, n + 3);
            uvs.insert(uvs.end(), uv, uv + 2);
        }
    }
    
    for (int jx = 0; jx < resolution - 1; jx++)
    {
        for (int iz = 0; iz < resolution - 1; iz++)
        {
            //one quad
            int row = iz; //current row
            int col1 = jx * resolution; //current columns
            int col2 = (jx + 1) * resolution; // next column along
            
            // B---D
            // | \ |
            // A---C
            // two tris: ACB, CDB
            GLuint A = row + col1;
            GLuint B = A + 1;
            GLuint C = row + col2;
            GLuint D = C + 1;
            
            GLuint square[] = { A, C, B, C, D, B };
            indices.insert(indices.end(), square, square + 6);
        }
    }
    
    //generate the OpenGL buffers and create geometry
    createVertexArrays(vertices, uvs, normals, indices);
    
    return 1;
}

lm::vec3 Geometry::calculateTerrainNormal(ImageData& height_map, int x, int y){
	lm::vec3 n(0,1,0);
    //TODO code terrain normal


    return n;
}

// Given an array of floats (in sets of three, representing vertices) calculates and
// sets the AABB of a geometry
void Geometry::setAABB(std::vector<GLfloat>& vertices) {
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
	aabb.center = lm::vec3((min.x + max.x) / 2,
		(min.y + max.y) / 2,
		(min.z + max.z) / 2);
	aabb.half_width = lm::vec3(max.x - aabb.center.x,
		max.y - aabb.center.y,
		max.z - aabb.center.z);
}

//creates a standard plane geometry and return its
int Geometry::createPlaneGeometry() {

	std::vector<GLfloat> vertices, uvs, normals;
	std::vector<GLuint> indices;
	vertices = { -1.0f, -1.0f, 0.0f,    1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f };
	uvs = { 0.0f, 0.0f,   1.0f, 0.0f,    1.0f, 1.0f,   0.0f, 1.0f };
	normals = { 0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f };
	indices = { 0, 1, 2, 0, 2, 3 };

	//generate the OpenGL buffers and create geometry
	createVertexArrays(vertices, uvs, normals, indices);

	return 1;
}

void Framebuffer::bindAndClear() {
	glViewport(0, 0, width, height);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Framebuffer::bindAndClear(lm::vec4 cc) {
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glClearColor(cc.x, cc.y, cc.z, cc.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Framebuffer::initColor(GLsizei w, GLsizei h) {

	width = w; height = h;

	glGenFramebuffers(1, &(framebuffer));
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glGenTextures(1, &(color_textures[0]));
	glBindTexture(GL_TEXTURE_2D, color_textures[0]);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_textures[0], 0);

	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::initDepth(GLsizei w, GLsizei h) {
	width = w; height = h;

	//bind framebuffer and texture as usual
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	//bind texture, but format with be only storing depth component
	glGenTextures(1, &(color_textures[0]));
	glBindTexture(GL_TEXTURE_2D, color_textures[0]);
	//generate depth texture
	glTexImage2D(GL_TEXTURE_2D, 0, 
		GL_DEPTH_COMPONENT, width, height, 0, 
		GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//wrap filters to make sure shadow stops at edge of frustum
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	//set border color so no risk of shadowing
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	//attach this texture to the framebuffers depth attachment
	//so anything drawn to this framebuffer is essentially just stored as depth, no colour
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, color_textures[0], 0);
	//tell openGL we are not going to draw to a color buffer - if we don't say this then the 
	//framebuffer is incomplete and we get an error
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	//safety first, safety second

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::initGbuffer(GLsizei w, GLsizei h) {
    width = w; height = h;
    
    //create and bind
    glGenFramebuffers(1, &(framebuffer));
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    //position
    glGenTextures(1, &(color_textures[0]));
    glBindTexture(GL_TEXTURE_2D, color_textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_textures[0], 0);
    
    //normal
    glGenTextures(1, &(color_textures[1]));
    glBindTexture(GL_TEXTURE_2D, color_textures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, color_textures[1], 0);
    
    //diffuse + specular (in A channel)
    glGenTextures(1, &(color_textures[2]));
    glBindTexture(GL_TEXTURE_2D, color_textures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, color_textures[2], 0);
    
    
    // - tell OpenGL which color attachments we'll use
    // (of this framebuffer) for rendering
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, rbo);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
