#pragma once
#include "includes.h"
#include "Shader.h"
#include "Components.h"
struct AABB {
	lm::vec3 center;
	lm::vec3 half_width;
};
//assume 3 channel
struct ImageData {
    GLubyte* data;
    int width;
    int height;
    int bytes_pp;
    bool getPixel(int x, int y, int pixel[3]) {
        
        if (x > width || y > height) return false;

        int pixel_location = width * bytes_pp * y + x * bytes_pp;
        pixel[0] = data[pixel_location];
        pixel[1] = data[pixel_location + 1];
        pixel[2] = data[pixel_location + 2];
        return true;
    }
};

struct Geometry {
	GLuint vao;
	GLuint num_tris;
	AABB aabb;
    
    
    std::vector<int> material_sets; //each entry is upper limit of set, in TRIANGLES
    std::vector<int> material_set_ids; //each entry is id of material for material set
    
    void render(int set); //new render function - render only certain amount of faces
	
	//constrctors
	Geometry() { vao = 0; num_tris = 0; }
	Geometry(int a_vao, int a_tris) : vao(a_vao), num_tris(a_tris) {}
	Geometry(std::vector<float>& vertices, std::vector<float>& uvs, std::vector<float>& normals, std::vector<unsigned int>& indices);
	
	//creation functions
	void createVertexArrays(std::vector<float>& vertices, std::vector<float>& uvs, std::vector<float>& normals, std::vector<unsigned int>& indices);
	void setAABB(std::vector<GLfloat>& vertices);

	int createPlaneGeometry();
    
    //terrain
    float max_terrain_height;
    int createTerrain(int resolution, float step, float max_height, ImageData& height_map);
    lm::vec3 calculateTerrainNormal(ImageData& height_map, int x, int y);
    
	//rendering functions
    void createMaterialSet(int tri_count, int material_id);
    void render();

};

struct Material {
	std::string name;
	int index = -1;
	int shader_id;
	lm::vec3 ambient;
	lm::vec3 diffuse;
	lm::vec3 specular;
	float specular_gloss;

	int diffuse_map;
    int diffuse_map_2; //NEW!! - note must init in constructor to -1!!
    int diffuse_map_3; //NEW!!
	int cube_map;
    int normal_map;
    int specular_map;
    float normal_factor;
    int noise_map; //NEW!!
    float height; //NEW!! for terrain (and maybe other stuff)
    
    lm::vec2 uv_scale;;

	Material() {
		name = "";
		ambient = lm::vec3(0.1f, 0.1f, 0.1f);
		diffuse = lm::vec3(1.0f, 1.0f, 1.0f);
		specular = lm::vec3(1.0f, 1.0f, 1.0f);
		diffuse_map = -1;
        diffuse_map_2 = -1;
        diffuse_map_3 = -1;
        noise_map = -1;
		cube_map = -1;
        normal_map = -1;
        specular_map = -1;
        normal_factor = 1.0f;
		specular_gloss = 80.0f;
        uv_scale = lm::vec2(1.0f, 1.0f);
        height = 0.0f;
	}
};

struct Framebuffer {
	GLuint width, height;
	GLuint framebuffer = -1;
	GLuint num_color_attachments = 0;
	GLuint color_textures[10] = { 0,0,0,0,0,0,0,0,0,0 };
	void bindAndClear();
    void bindAndClear(lm::vec4 clear_color);
	void initColor(GLsizei width, GLsizei height);
	void initDepth(GLsizei width, GLsizei height);
    void initGbuffer(GLsizei width, GLsizei height);
};

