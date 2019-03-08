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
uniform vec2 u_uv_scale;
uniform float u_normal_factor;

//texture uniforms
uniform int u_use_diffuse_map;
uniform sampler2D u_diffuse_map;
uniform int u_use_normal_map;
uniform sampler2D u_normal_map;
uniform int u_use_specular_map;
uniform sampler2D u_specular_map;

const int MAX_LIGHTS = 8;

//shadows
uniform sampler2D u_shadow_map[MAX_LIGHTS];

//light structs and uniforms
struct Light {
    vec4 position;
    vec4 direction;
    vec4 color;
    float linear_att;
    float quadratic_att;
    float spot_inner_cosine;
    float spot_outer_cosine;
    mat4 view_projection;
    int type; // 0 - directional; 1 - point; 2 - spot
    int cast_shadow;
};


uniform int u_num_lights;

layout (std140) uniform u_lights_ubo
{
    Light lights[MAX_LIGHTS]; 
};

float random(vec4 seed4){
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float shadowCalculationHard(vec4 fragment_light_space, int light_index) {
    float shadow = 0.0; //default no shadow
    
    //gl_position does this divide automatically. But we need to do it manually
    //result is current fragment coordinates in light clip space
    vec3 proj_coords = fragment_light_space.xyz / fragment_light_space.w;
    
    //openGL clip space goes from -1 to +1, but our texture values are from 0->1
    //so lets remap our projected coords to 0->1
    proj_coords = proj_coords * 0.5 + 0.5;
    
    //we only want to deal with stuff which is inside the light frustum. So we clamp
    //and do not process anything outside
    if (clamp(proj_coords, 0.0, 1.0) == proj_coords) {
        
        //distances
        float current_depth = proj_coords.z;
        float shadow_map_depth = texture(u_shadow_map[light_index], proj_coords.xy).r;
        
        //subtract bias to remove acne
        float bias = 0.005;
        shadow = current_depth - bias > shadow_map_depth ? 1.0 : 0.0;
        
    }
    
    return shadow;
}

float shadowCalculationPCF(vec4 fragment_light_space, float NdotL, int light_index) {
    
    vec3 proj_coords = fragment_light_space.xyz / fragment_light_space.w;
    proj_coords = proj_coords * 0.5 + 0.5;

    float shadow = 0.0;
    if (clamp(proj_coords, 0.0, 1.0) == proj_coords) {

        //current depth of the projected sample
        float current_depth = proj_coords.z;

        float bias = max(0.001 * (1.0 - NdotL), 0.001);
        //PCF
        vec2 texel_size = 1.0 / textureSize(u_shadow_map[light_index], 0);
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                float pcf_depth = texture(u_shadow_map[light_index],
                                          proj_coords.xy + vec2(x,y) * texel_size).r;
                shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
    }
    
    return shadow;
}

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

void main(){

    //scale uvs
    vec2 s_uv = v_uv * u_uv_scale;
    
    //normal
    vec3 N = normalize(v_normal); //normal
    
    if (u_use_normal_map != 0) {
        vec3 Nmap = perturbNormal(N, normalize(v_cam_dir), s_uv, texture(u_normal_map, s_uv).xyz);
        N = mix(N, Nmap, u_normal_factor);
    }

    //specular
    vec3 mat_specular = u_specular;
    if (u_use_specular_map != 0)
        mat_specular = mat_specular * texture(u_specular_map, s_uv).xyz;
    
    
	vec3 mat_diffuse = u_diffuse; //colour from uniform
	//multiply by texture if present
	if (u_use_diffuse_map != 0)
		mat_diffuse = mat_diffuse * texture(u_diffuse_map, s_uv).xyz;

	//ambient light
	vec3 final_color = u_ambient * mat_diffuse;
	

	//loop lights
	for (int i = 0; i < u_num_lights; i++){

        float attenuation = 1.0;
        
        float spot_cone_intensity = 1.0;
        
		vec3 L = normalize(-lights[i].direction.xyz); // for directional light

		vec3 R = reflect(-L,N); //reflection vector
		vec3 V = normalize(v_cam_dir); //to camera
        
        if (lights[i].type > 0) {
        
            vec3 point_to_light = lights[i].position.xyz - v_vertex_world_pos;
            L = normalize(point_to_light);

            // soft spot cone
            if (lights[i].type == 2) {
                vec3 D = normalize(lights[i].direction.xyz);
                float cos_theta = dot(D, -L);
                
                float numer = cos_theta - lights[i].spot_outer_cosine;
                float denom = lights[i].spot_inner_cosine - lights[i].spot_outer_cosine;
                spot_cone_intensity = 1 - clamp(numer/denom, 0.0, 1.0);
            }
            
            //attenuation
            float distance = length(point_to_light);
            attenuation = 1.0 / (1.0 + lights[i].linear_att * distance + lights[i].quadratic_att * (distance * distance));
        }
        
        
		//diffuse color
		float NdotL = max(0.0, dot(N, L));
		vec3 diffuse_color = NdotL * mat_diffuse * lights[i].color.xyz;
							 
		//specular color
		float RdotV = max(0.0, dot(R, V)); //calculate dot product
		RdotV = pow(RdotV, u_specular_gloss); //raise to power for glossiness effect
        vec3 specular_color = RdotV * lights[i].color.xyz * mat_specular;

        //shadow
        vec4 position_light_space = lights[i].view_projection * vec4(v_vertex_world_pos, 1.0);
        
        float shadow = (lights[i].cast_shadow == 1 ? shadowCalculationPCF(position_light_space, NdotL, i) : 0.0);

		//final color
        final_color += ((diffuse_color + specular_color) * attenuation * spot_cone_intensity) * (1.0 - shadow);

	}
    
    //fragColor = vec4(texture(u_normal_map, s_uv).xyz, 1.0);
    fragColor = vec4(final_color, 1.0);
}
