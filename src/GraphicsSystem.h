//
//  Copyright � 2018 Alun Evans. All rights reserved.
//
#pragma once
#include "includes.h"
#include "Shader.h"
#include "Components.h"
#include "GraphicsUtilities.h"
#include <unordered_map>

#define MAX_LIGHTS 8

class GraphicsSystem {
public:
	~GraphicsSystem();
    void init(int window_width, int window_height, std::string assets);
    void lateInit();
    void update(float dt);
    
	//viewport
	void updateMainViewport(int window_width, int window_height);
    void getMainViewport(int& width, int& height);
	lm::vec4 screen_background_color;

    //shader loader
	Shader* loadShader(std::string vs_path, std::string fs_path, bool compile_direct = false);

    //set the environment
    void setEnvironment(GLuint tex_id, int geom_id, GLuint program);
    
	//materials
    int createMaterial();
	Material& getMaterial(int mat_id) { return materials_.at(mat_id); }
    std::vector<Material>& getMaterials() { return materials_;}
    
    //geometry
    int createGeometryFromFile(std::string filename);
    int createMultiGeometryFromFile(std::string filename);
    int createTerrainGeometry(int resolution, float step, float max_height, ImageData& height_map);

	//lights update
	bool needUpdateLights = true;
    
private:
    //resources
    std::string assets_folder_;
	std::unordered_map<GLint, Shader*> shaders_; //compiled id, pointer
    std::vector<Geometry> geometries_;
    std::vector<Material> materials_;

    //viewport
    int viewport_width_, viewport_height_;
    
	//shader stuff
	Shader* shader_ = nullptr; //current shader
	void useShader(Shader* s);
	void useShader(GLuint p);



	//materials stuff
    GLint current_material_ = -1;
    void setMaterialUniforms();

	//sorting and checking and abstracting
	void sortMeshes_();
	void resetShaderAndMaterial_();
	void updateAllCameras_();
	void checkShaderAndMaterial_(Mesh& mesh);
    void checkMaterial_(Mesh& mesh);
	
	//binding and clearing
	void bindAndClearScreen_();

	//light uniform buffer object
	GLuint LIGHTS_BINDING_POINT = 1;
	GLuint light_ubo_;
	void updateLights_();
    void setLightUniforms_();

	//framebuffers
	Shader* screen_space_shader_;
	int screen_space_geom_;
	Framebuffer frame_;

	//shadowing
	Shader* depth_shader_ = nullptr;
	Shader* screen_depth_shader_ = nullptr;
	Framebuffer shadow_frame_[MAX_LIGHTS];
	void renderDepth_(Mesh& comp, const Light& light);
    
    //gbuffer
    Shader* gbuffer_shader_ = nullptr;
    Shader* deferred_shader_ = nullptr;
    Shader* deferred_volume_shader_ = nullptr;
    Framebuffer gbuffer_;
    void renderGbuffer();
    void renderLightVolumes();
    int sphere_volume_geom_;
    int cone_volume_geom_;
    
    //cubemap/environment
    int cube_map_geom_ = -1;
    GLuint environment_program_ = 0;
    GLuint environment_tex_ = 0;
    
    //rendering
    void renderMeshComponent_(Mesh& comp);
    void renderEnvironment_();
    void previewTextureViewport(GLuint texture_id);
    
	//AABB
	void setGeometryAABB_(Geometry& geom, std::vector<GLfloat>& vertices);
	AABB transformAABB_(const AABB& aabb, const lm::mat4& transform);
	bool BBInFrustum_(const AABB& aabb, const lm::mat4& model_view_projection);
	bool AABBInFrustum_(const AABB& aabb, const lm::mat4& view_projection);

	//shader strings
	const char* screen_vertex_shader_ =
		"#version 330\n"
		"layout(location = 0) in vec3 a_vertex; \n"
		"layout(location = 1) in vec2 a_uv; \n"
		"out vec2 v_uv;\n"
		"void main() {\n"
		"    gl_Position = vec4(a_vertex, 1); \n"
		"    v_uv = a_uv; \n"
		"}\n";

	const char* screen_fragment_shader_ =
		"#version 330\n"
		"in vec2 v_uv;\n"
		"layout(location = 0) out vec4 fragColor;\n"
		"uniform sampler2D u_texture;\n"
		"void main() {\n"
		"   fragColor = texture(u_texture, v_uv);\n"
		"}\n";

	const char* screen_depth_fragment_shader_ =
		"#version 330\n"
		"in vec2 v_uv;\n"
		"layout(location = 0) out vec4 fragColor;\n"
		"uniform sampler2D u_texture;\n"
		"void main() {\n"
		"   float depth_value = texture(u_texture, v_uv).r;\n"
		"   fragColor = vec4(vec3(depth_value),1.0);\n"
		"}\n";

	const char* depth_vertex_shader_ =
		"#version 330\n"
		"layout(location = 0) in vec3 a_vertex; \n"
		"uniform mat4 u_model;\n"
		"uniform mat4 u_vp;\n"
		"void main() {\n"
		"    gl_Position = u_vp * u_model * vec4(a_vertex, 1); \n"
		"}\n";

	const char* depth_fragment_shader_ =
		"#version 330\n"
		"void main() {\n"
		"}\n";

};
