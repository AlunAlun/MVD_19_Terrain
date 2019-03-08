#version 330

in vec3 v_tex;
out vec4 fragColor;

uniform samplerCube skybox;
//uniform sampler2D skybox;

void main(){

    fragColor = texture(skybox, v_tex);
}
