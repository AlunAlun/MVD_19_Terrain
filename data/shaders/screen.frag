#version 330

in vec2 v_uv;

out vec4 fragColor;

uniform sampler2D u_screen_texture;


void main(){

    vec3 col = texture(u_screen_texture, v_uv).xyz;
	fragColor = vec4(col, 1.0);
    
}
