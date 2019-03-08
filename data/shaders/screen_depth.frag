#version 330

in vec2 v_uv;

layout(location = 0) out vec4 fragColor;

uniform sampler2D u_screen_texture;

void main(){

    float depth_value = texture(u_screen_texture, v_uv).r;
    fragColor = vec4(vec3(depth_value), 1.0);
    
}
