#version 330

//varyings and out color
in vec2 v_uv;
in vec3 v_normal;
in vec3 v_vertex_world_pos;
out vec4 fragColor;

uniform vec3 u_cam_pos;
uniform samplerCube u_skybox; 


void main(){
    //calculate direction to camera in world space
    vec3 I = normalize(v_vertex_world_pos - u_cam_pos);
    vec3 R = reflect(I, normalize(v_normal));

    vec3 reflect_color = texture(u_skybox, R).rgb;
    


    fragColor = vec4(reflect_color, 1.0);
    
    //fragColor = vec4(0.0,0.0,0.0,1.0);
}
