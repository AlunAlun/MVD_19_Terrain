#pragma once
#include "includes.h"
#include "Components.h"
#include "Shader.h"

#include "ft2build.h"
#include FT_FREETYPE_H

class GUISystem {
public:
	void init(int w, int h);
	void lateInit();
	void update(float dt);

	void updateViewport(int new_width, int new_height);

	GLuint createTextTexture(std::string text, std::string font_path, int font_size, int tex_width, int tex_height);

	void updateMousePosition(int new_x, int new_y) { mouse_x_ = new_x; mouse_y_ = new_y; };
	void key_mouse_callback(int key, int action, int mods);
	
private:
	int width_, height_;
	GLuint vao_;
	Shader* icon_shader_;
    Shader* text_shader_;
	void createGeometry_();
	lm::mat4 view_projection;

	int mouse_x_; int mouse_y_;

	void anchorModelMatrix_(GUIAnchor anchor, int el_width, int el_height, lm::mat4& model);
};
