#version 330
//these outs correspond to the three color buffers
layout (location = 0) out vec3 g_position;
layout (location = 1) out vec3 g_normal;
layout (location = 2) out vec4 g_albedo;
//data from vertex shader
in vec2 v_uv;
in vec3 v_normal;
in vec3 v_cam_dir;
in vec3 v_vertex_world_pos;
//we need to upload these uniforms from material
uniform int u_use_diffuse_map;
uniform sampler2D u_diffuse_map;
uniform int u_use_normal_map;
uniform sampler2D u_normal_map;
uniform int u_use_specular_map;
uniform sampler2D u_specular_map;

uniform vec3 u_diffuse;
uniform vec3 u_specular;
uniform float u_normal_factor;
uniform vec2 u_uv_scale;

//given a normal vector, a position vector, and uv coordinates
//creates a mat3 which represents tangent space for
//frame of reference
//http://www.thetenthplanet.de/archives/1180
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );
    
    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    
    // construct a scale-invariant frame
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
}

// perturbs the normal using a tangent space normal map
// N - normal vector from geometry
// P - vertex position
// texcoord - the current texture coordinates
// normal_sample - the sample from the normal map
vec3 perturbNormal( vec3 N, vec3 P, vec2 texcoord, vec3 normal_sample )
{
    
    normal_sample = normal_sample * 2.0 - 1.0;
    mat3 TBN = cotangent_frame(N, -P, texcoord);
    return normalize(TBN * normal_sample);
}


void main() {
    //store the vertex world position
    g_position = v_vertex_world_pos;
    
    //scale uvs
    vec2 s_uv = v_uv * u_uv_scale;
    
    //normal
    vec3 N = normalize(v_normal);
    if (u_use_normal_map != 0) {
        vec3 Nmap = perturbNormal(N, normalize(v_cam_dir), s_uv, texture(u_normal_map, s_uv).xyz);
        N = mix(N, Nmap, u_normal_factor);
    }
    //store the vertex normal
    g_normal = N;
    
    
    //compress specular to one number
    vec3 spec_3 = u_specular;
    if (u_use_specular_map != 0)
        spec_3 = u_specular * texture(u_specular_map, s_uv).xyz;
    float specular = (spec_3.x + spec_3.y + spec_3.z) / 3;
    
    //store the albedo color and specular
    vec3 diffuse_color = u_diffuse;
   	if (u_use_diffuse_map > 0) 
   		diffuse_color *= texture(u_diffuse_map, s_uv).xyz;
    g_albedo = vec4(diffuse_color, specular);
}
