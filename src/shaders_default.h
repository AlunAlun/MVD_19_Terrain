#pragma once

//**** Screen shader (draws textured mesh in screen coords) **** //
static const char* g_shader_screen_vertex =
"#version 330\n"
"layout(location = 0) in vec3 a_vertex; \n"
"layout(location = 1) in vec2 a_uv; \n"
"out vec2 v_uv;\n"
"void main() {\n"
"    gl_Position = vec4(a_vertex, 1); \n"
"    v_uv = a_uv; \n"
"}\n";

static const char* g_shader_screen_fragment =
"#version 330\n"
"in vec2 v_uv;\n"
"layout(location = 0) out vec4 fragColor;\n"
"uniform sampler2D u_texture;\n"
"void main() {\n"
"   fragColor = texture(u_texture, v_uv);\n"
"}\n";

//**** Screen/depth shader **** //
static const char* g_shader_screen_depth_fragment =
"#version 330\n"
"in vec2 v_uv;\n"
"layout(location = 0) out vec4 fragColor;\n"
"uniform sampler2D u_texture;\n"
"void main() {\n"
"   float depth_value = texture(u_texture, v_uv).r;\n"
"   fragColor = vec4(vec3(depth_value),1.0);\n"
"}\n";

//**** Depth shader (paints depth buffer only) **** //

static const char* g_shader_depth_vertex =
"#version 330\n"
"layout(location = 0) in vec3 a_vertex; \n"
"uniform mat4 u_model;\n"
"uniform mat4 u_vp;\n"
"void main() {\n"
"    gl_Position = u_vp * u_model * vec4(a_vertex, 1); \n"
"}\n";

static const char* g_shader_depth_fragment =
"#version 330\n"
"void main() {\n"
"}\n";

//**** Line Shader (draws colored vertices) **** //

static const char* g_shader_line_vertex =
"#version 330\n"
"layout(location = 0) in vec4 a_vertex; \n"
"uniform mat4 u_mvp;\n"
"uniform vec3 u_color[4];\n"
"uniform int u_color_mod;\n"
"out lowp vec4 v_color;\n"
"void main() {;\n"
"    gl_Position = u_mvp * vec4(a_vertex.xyz, 1); \n"
"   v_color = vec4(u_color[ int(a_vertex.w) + u_color_mod ], 1.0f);\n"
"}\n";

static const char* g_shader_line_fragment =
"#version 330\n"
"in vec4 v_color;\n"
"layout(location = 0) out vec4 fragColor;\n"
"void main() {\n"
"    fragColor = v_color;\n"
"}\n";

//**** Icon Shader (draw textured mesh in MVP coordinates **** //

static const char* g_shader_icon_vertex =
"#version 330\n"
"layout(location = 0) in vec3 a_vertex; \n"
"layout(location = 1) in vec2 a_uv; \n"
"out vec2 v_uv;\n"
"uniform mat4 u_mvp;\n"
"void main() {\n"
"    gl_Position = u_mvp * vec4(a_vertex, 1); \n"
"    v_uv = a_uv; \n"
"}\n";

static const char* g_shader_icon_fragment =
"#version 330\n"
"in vec2 v_uv;\n"
"layout(location = 0) out vec4 fragColor;\n"
"uniform sampler2D u_icon;\n"
"void main() {\n"
"    fragColor = texture(u_icon, v_uv);\n"
"}\n";

//font shader
static const char* g_shader_font_vertex =
"#version 330\n"
"layout(location = 0) in vec3 a_vertex;\n"
"layout(location = 1) in vec2 a_uv;\n"
"uniform mat4 u_mvp;\n"
"out vec2 v_uv;\n"
"void main() {\n"
"	gl_Position = u_mvp * vec4(a_vertex, 1);\n"
"	v_uv = a_uv;\n"
"}\n";

static const char* g_shader_font_fragment =
"#version 330\n"
"in vec2 v_uv;\n"
"out vec4 fragColor;\n"
"uniform sampler2D u_icon;\n"
"uniform vec3 u_color;\n"
"void main() {\n"
"	float final_color = texture(u_icon, v_uv).r;\n"
"	fragColor = vec4(u_color, final_color);\n"
"}\n";

//environment shader
static const char* g_shader_environnment_vertex =
"#version 330\n"
"layout(location = 0) in vec3 a_vertex;\n"
"out vec3 v_tex;\n"
"uniform mat4 u_vp;\n"
"void main(){\n"
"    v_tex = a_vertex;\n"
"    vec4 pos = u_vp * vec4(a_vertex, 1.0);\n"
"    gl_Position = pos.xyww;\n"
"}\n";

static const char* g_shader_environment_fragment =
"#version 330\n"
"in vec3 v_tex;\n"
"out vec4 fragColor;\n"
"uniform samplerCube u_skybox;\n"
"void main(){\n"
"    fragColor = texture(u_skybox, v_tex);\n"
"}\n";

