#version 330

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
    int cast_shadow; // 0 - false; 1 - true
};

//...light struct as before.../

out vec4 fragColor;

uniform int u_light_id;

const int MAX_LIGHTS = 8;
layout (std140) uniform u_lights_ubo
{
    Light lights[MAX_LIGHTS];
};

uniform vec3 u_cam_pos;
uniform sampler2D u_tex_position;
uniform sampler2D u_tex_normal;
uniform sampler2D u_tex_albedo;

//shadows
uniform sampler2D u_shadow_map[MAX_LIGHTS];

float random(vec4 seed4){
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

vec2 poissonDisk[4] = vec2[](
                             vec2( -0.94201624, -0.39906216 ),
                             vec2( 0.94558609, -0.76890725 ),
                             vec2( -0.094184101, -0.92938870 ),
                             vec2( 0.34495938, 0.29387760 )
                             );

float shadowCalculationPoisson(vec4 fragment_light_space, float NdotL, int light_index) {
    
    //gl_position does this divide automatically. But we need to do it manually
    //result is current fragment coordinates in light clip space
    vec3 proj_coords = fragment_light_space.xyz / fragment_light_space.w;
    
    //openGL clip space goes from -1 to +1, but our texture values are from 0->1
    //so lets remap our projected coords to 0->1
    proj_coords = proj_coords * 0.5 + 0.5;
    
    //predeclare
    float shadow = 0.01;
    
    //we only want to deal with stuff which is inside the light frustum. So we clamp
    //and do not process anything outside
    if (clamp(proj_coords, 0.0, 1.0) == proj_coords) {
        
        //current depth of the projected sample
        float current_depth = proj_coords.z;
        
        float bias = max(0.0005 * (1.0 - NdotL), 0.0005);

        vec2 texel_size = 1.0 / textureSize(u_shadow_map[light_index], 0);
        for (int i = 0;i < 4; i++){
            
            int index = int(4*random(vec4(gl_FragCoord.xyy, i))) % 4;
            
            float poisson_depth = texture( u_shadow_map[light_index],
                                          proj_coords.xy + poissonDisk[index] * texel_size).r;
            
            shadow += current_depth - bias > poisson_depth ? 1.0 : 0.0;
        }
        shadow /= 4.0;
        
    }
    
    return shadow;
}

void main() {
    
    
    //calculate texture coordinate
    vec2 uv = gl_FragCoord.xy / textureSize(u_tex_position, 0).xy;

    //read textures
    vec3 position = texture(u_tex_position, uv).xyz;
    vec3 N = texture(u_tex_normal, uv).xyz;
    vec4 albedo_spec = texture(u_tex_albedo, uv);
    
    vec3 final_color = vec3(0);

    float attenuation = 1.0;
    float spot_cone_intensity = 1.0;

    //light vectors
    vec3 L = -normalize(lights[u_light_id].direction.xyz); 
    vec3 R = reflect(-L,N);
    vec3 V = normalize(u_cam_pos - position); 

    if (lights[u_light_id].type > 0) {
    
        vec3 point_to_light = lights[u_light_id].position.xyz - position;
        L = normalize(point_to_light);

        // soft spot cone
        if (lights[u_light_id].type == 2) {
            vec3 D = normalize(lights[u_light_id].direction.xyz);
            float cos_theta = dot(D, -L);

            float numer = cos_theta - lights[u_light_id].spot_outer_cosine;
            float denom = lights[u_light_id].spot_inner_cosine - lights[u_light_id].spot_outer_cosine;
            spot_cone_intensity = 1 - clamp(numer/denom, 0.0, 1.0);

        }
        
        //attenuation
        float distance = length(point_to_light);
        attenuation = 1.0 / (1.0 + lights[u_light_id].linear_att * distance + lights[u_light_id].quadratic_att * (distance * distance));
    }

    //diffuse shading
    float NdotL = max(0.0, dot(N, L));
    vec3 diffuse_color = NdotL * albedo_spec.xyz * lights[u_light_id].color.xyz;
    //specular
    float RdotV = max(0.0, dot(R, V)); 
    RdotV = pow(RdotV, 30.0);
    vec3 specular_color = RdotV * albedo_spec.w * lights[u_light_id].color.xyz;
    
    vec4 position_light_space = lights[u_light_id].view_projection * vec4(position, 1.0);
    
    float shadow = (lights[u_light_id].cast_shadow == 1 ? shadowCalculationPoisson(position_light_space, NdotL, u_light_id) : 0.0);

    final_color = ((diffuse_color + specular_color) * attenuation * spot_cone_intensity) * (1.0 - shadow);

    fragColor = vec4(final_color, 1.0);
    //fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
