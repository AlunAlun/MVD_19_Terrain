#version 330

layout(location = 0) in vec3 a_vertex;

out vec3 v_tex; //note: vec3!
uniform mat4 u_vp;

void main(){
	//v_tex is a vec3, not a vec2
	v_tex = a_vertex; 

	//calculate position
	vec4 pos = u_vp * vec4(a_vertex, 1.0);
    //gl_Position = pos;
    //optimisation
    gl_Position = pos.xyww;
}


