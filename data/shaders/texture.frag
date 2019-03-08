#version 330

//varyings and out color
in vec2 v_uv;
in vec3 v_normal;
in vec3 v_light_dir;
in vec3 v_cam_dir;
in vec3 v_vertex_world_pos;
out vec4 fragColor;

//basic material uniforms
uniform vec3 u_ambient;
uniform vec3 u_diffuse;
uniform vec3 u_specular;
uniform float u_specular_gloss;

//texture uniforms
uniform sampler2D u_diffuse_map;

//light structs and uniforms
struct PointLight {
	vec3 position;
	vec3 color;
};
const int MAX_LIGHTS = 8;
uniform PointLight lights[MAX_LIGHTS];
uniform int u_num_lights;


void main(){

	vec3 diffuse_map = texture(u_diffuse_map, v_uv).xyz;

	//ambient light
	vec3 final_color = diffuse_map;
	

	fragColor = vec4(final_color, 1.0);
}