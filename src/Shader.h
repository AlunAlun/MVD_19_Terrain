#pragma once

#include "includes.h"
#include <unordered_map>
#include <vector>

//Uniform IDs are global so that we can access them in Graphics System
enum UniformID {
    U_VP,
	U_MVP,
	U_MODEL,
	U_NORMAL_MATRIX,
	U_CAM_POS,
	U_COLOR,
	U_COLOR_MOD,
	U_AMBIENT,
	U_DIFFUSE,
	U_SPECULAR,
	U_SPECULAR_GLOSS,
	U_USE_DIFFUSE_MAP,
    U_USE_DIFFUSE_MAP_2,
    U_USE_DIFFUSE_MAP_3,
	U_DIFFUSE_MAP,
    U_DIFFUSE_MAP_2,
    U_DIFFUSE_MAP_3,
    U_USE_NORMAL_MAP,
    U_NORMAL_MAP,
    U_NORMAL_FACTOR,
    U_USE_SPECULAR_MAP,
    U_SPECULAR_MAP,
    U_USE_NOISE_MAP,
    U_NOISE_MAP,
	U_SKYBOX,
	U_USE_REFLECTION_MAP,
	U_NUM_LIGHTS,
    U_LIGHTS_UBO,
	U_SCREEN_TEXTURE,
	U_NEAR_PLANE,
	U_FAR_PLANE,
	U_LIGHT_MATRIX,
	U_SHADOW_MAP,
    U_TEX_POSITION,
    U_TEX_NORMAL,
    U_TEX_ALBEDO,
    U_SHADOW_MAP0,
    U_SHADOW_MAP1,
    U_SHADOW_MAP2,
    U_SHADOW_MAP3,
    U_SHADOW_MAP4,
    U_SHADOW_MAP5,
    U_SHADOW_MAP6,
    U_SHADOW_MAP7,
    U_LIGHT_ID,
    U_UV_SCALE,
    U_MAX_HEIGHT,
	UNIFORMS_COUNT
};

//this map allows us to map the uniform string name to our enum ID
const std::unordered_map<std::string, UniformID> uniform_string2id_ = {
	{ "u_vp", U_VP },
    { "u_mvp", U_MVP },
	{ "u_model", U_MODEL },
	{ "u_normal_matrix", U_NORMAL_MATRIX },
	{ "u_cam_pos", U_CAM_POS },
	{ "u_color", U_COLOR },
	{ "u_color_mod", U_COLOR_MOD },
	{ "u_ambient", U_AMBIENT },
	{ "u_diffuse", U_DIFFUSE },
	{ "u_specular", U_SPECULAR },
	{ "u_specular_gloss", U_SPECULAR_GLOSS },
	{ "u_use_diffuse_map", U_USE_DIFFUSE_MAP },
    { "u_use_diffuse_map2", U_USE_DIFFUSE_MAP_2 },
    { "u_use_diffuse_map3", U_USE_DIFFUSE_MAP_3 },
	{ "u_diffuse_map", U_DIFFUSE_MAP },
    { "u_diffuse_map_2", U_DIFFUSE_MAP_2 },
    { "u_diffuse_map_3", U_DIFFUSE_MAP_3 },
    { "u_use_normal_map", U_USE_NORMAL_MAP },
    { "u_normal_map", U_NORMAL_MAP },
    { "u_normal_factor", U_NORMAL_FACTOR },
    { "u_use_specular_map", U_USE_SPECULAR_MAP },
    { "u_specular_map", U_SPECULAR_MAP },
    { "u_noise_map", U_NOISE_MAP },
	{ "u_skybox", U_SKYBOX },
	{ "u_use_reflection_map", U_USE_REFLECTION_MAP },
	{ "u_num_lights", U_NUM_LIGHTS },
	{ "u_near_plane", U_NEAR_PLANE },
	{ "u_far_plane", U_FAR_PLANE },
	{ "u_light_matrix", U_LIGHT_MATRIX },
	{ "u_shadow_map", U_SHADOW_MAP },
    { "u_num_lights", U_NUM_LIGHTS },
	{ "u_screen_texture", U_SCREEN_TEXTURE },
    { "u_tex_position", U_TEX_POSITION },
    { "u_tex_normal", U_TEX_NORMAL },
    { "u_tex_albedo", U_TEX_ALBEDO },
    { "u_shadow_map[0]", U_SHADOW_MAP0 },
    { "u_shadow_map[1]", U_SHADOW_MAP1 },
    { "u_shadow_map[2]", U_SHADOW_MAP2 },
    { "u_shadow_map[3]", U_SHADOW_MAP3 },
    { "u_shadow_map[4]", U_SHADOW_MAP4 },
    { "u_shadow_map[5]", U_SHADOW_MAP5 },
    { "u_shadow_map[6]", U_SHADOW_MAP6 },
    { "u_shadow_map[7]", U_SHADOW_MAP7 },
    { "u_light_id", U_LIGHT_ID },
    { "u_uv_scale", U_UV_SCALE},
    { "u_max_height", U_MAX_HEIGHT}
    
    
};

const std::unordered_map<std::string, UniformID> uniformblock_string2id_ = {
    { "u_lights_ubo", U_LIGHTS_UBO },
};

class Shader {
private:
	//stores, for each uniform enum, it's location
	std::vector<GLuint> uniform_locations_;
	void initUniforms_();
    
public:
    GLuint program;
	std::string name;
	Shader();
    Shader(std::string vertSource, std::string fragSource);
    std::string readFile(std::string filename);
	GLuint compileFromStrings(std::string vsh, std::string fsh);
    GLuint makeVertexShader(const char* shaderSource);
    GLuint makeFragmentShader(const char* shaderSource);
    void makeShaderProgram(GLuint vertexShaderID, GLuint fragmentShaderID);
    GLint bindAttribute(const char* attribute_name);
    void saveProgramInfoLog(GLuint obj);
    void saveShaderInfoLog(GLuint obj);
    std::string log;
    
	//
    GLuint getUniformLocation(UniformID name);
    
    bool setUniform(UniformID id, const int data);
    bool setUniform(UniformID id, const float data);
    bool setUniform(UniformID id, const lm::vec2& data);
    bool setUniform(UniformID id, const lm::vec3& data);
    bool setUniform(UniformID id, const lm::mat4& data);
    bool setUniformBlock(UniformID id, const int binding_point);
    bool setTexture(UniformID id, GLuint tex_id, GLuint unit);
    bool setTextureCube(UniformID id, GLuint tex_id, GLuint unit);
    
    
};

