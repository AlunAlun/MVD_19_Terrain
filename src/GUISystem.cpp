#include "GUISystem.h"
#include "extern.h"

#include "ft2build.h"
#include FT_FREETYPE_H

void GUISystem::init(int w, int h) {
	width_ = w;
	height_ = h;

	//create vp
	view_projection.orthographic((float)-width_/2, (float)width_/2, (float)-height_/2, (float)height_/2, -0.5, -2);

    icon_shader_ = new Shader();//"data/shaders/tmp.vert", "data/shaders/tmp.frag");
    icon_shader_->compileFromStrings(g_shader_icon_vertex, g_shader_icon_fragment);
    
	text_shader_ = new Shader();
	text_shader_->compileFromStrings(g_shader_font_vertex, g_shader_font_fragment);
	    
	createGeometry_();
}

void GUISystem::lateInit() {
	//for all images
	auto& elements = ECS.getAllComponents<GUIElement>();
	for (auto& el : elements) {

		//check to see if we have specified gui width and height, if not, set them according to texture
		glBindTexture(GL_TEXTURE_2D, el.texture);
		if (el.width == 0)
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &(el.width));
		if (el.height == 0)
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &(el.height));

		//get screen coords of element
		lm::mat4 model;
		model.makeScaleMatrix((float)el.width / 2, (float)el.height / 2, 1.0f);
		anchorModelMatrix_(el.anchor, el.width, el.height, model);
		model.translate(el.offset.x, el.offset.y, 0);
		lm::mat4 mvp = view_projection * model;
		
		lm::vec3 bl(-1, -1, 0);
		lm::vec3 tr(1, 1, 0);

		lm::vec3 blc = mvp * bl;
		lm::vec3 trc = mvp * tr;

		el.screen_bounds.x_min = (int)(((blc.x + 1) / 2) * width_);
		el.screen_bounds.x_max = (int)(((trc.x + 1) / 2) * width_);
		el.screen_bounds.y_min = (int)(height_ - ((trc.y + 1) / 2) * height_);
		el.screen_bounds.y_max = (int)(height_ - ((blc.y + 1) / 2) * height_);
	}

	//for all texts
	auto& text_elements = ECS.getAllComponents<GUIText>();
	for (auto& el : text_elements) {
		//check if texture has been created
		if (el.texture == 0) {
			el.texture = createTextTexture(el.text, el.font_face, el.font_size, el.width, el.height);
		}
	}
}
void GUISystem::update(float dt) {

	//we draw GUI last, want it to be on top of everything
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//draw GUI images first
	glUseProgram(icon_shader_->program);

	//for all images
	auto& elements = ECS.getAllComponents<GUIElement>();
	for (auto& el : elements) {

		//scale -1->+1 quad to size of image
		lm::mat4 model;
		model.makeScaleMatrix((float)el.width / 2, (float)el.height / 2, 1.0f);

		//apply anchor
		anchorModelMatrix_(el.anchor, el.width, el.height, model);

		//apply offset
		model.translate(el.offset.x, el.offset.y, 0);

		//set uniforms
		GLint u_mvp = glGetUniformLocation(icon_shader_->program, "u_mvp");
		glUniformMatrix4fv(u_mvp, 1, GL_FALSE, (view_projection * model).m);

		GLint u_icon = glGetUniformLocation(icon_shader_->program, "u_icon");
		glUniform1i(u_icon, 10);

		glActiveTexture(GL_TEXTURE0 + 10);
		glBindTexture(GL_TEXTURE_2D, el.texture);

		//draw
		glBindVertexArray(vao_);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	//use a different shader for text
	glUseProgram(text_shader_->program);

	//for all texts
	auto& text_elements = ECS.getAllComponents<GUIText>();
	for (auto& el : text_elements) {

		//scale -1->+1 quad to size of image
		lm::mat4 model;
		model.makeScaleMatrix((float)el.width / 2, (float)el.height / 2, 1.0f);

		//apply anchor
		anchorModelMatrix_(el.anchor, el.width, el.height, model);

		//apply offset
		model.translate(el.offset.x, el.offset.y, 0);

		//set uniforms
		GLint u_mvp = glGetUniformLocation(text_shader_->program, "u_mvp");
		glUniformMatrix4fv(u_mvp, 1, GL_FALSE, (view_projection * model).m);

		GLint u_color = glGetUniformLocation(text_shader_->program, "u_color");
		glUniform3fv(u_color, 1, el.color.value_);

		GLint u_icon = glGetUniformLocation(text_shader_->program, "u_icon");
		glUniform1i(u_icon, 10);

		glActiveTexture(GL_TEXTURE0 + 10);
		glBindTexture(GL_TEXTURE_2D, el.texture);

		//draw
		glBindVertexArray(vao_);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

}

void GUISystem::anchorModelMatrix_(GUIAnchor anchor, int el_width, int el_height, lm::mat4& model) {
	
	float hw = (float)width_ / 2; float hh = (float)height_ / 2;
	float ehw, ehh;
	ehw = (float)el_width / 2; ehh = (float)el_height / 2;

	switch (anchor) {
	case GUIAnchorTopLeft:
		model.translate(-hw + ehw, hh - ehh, 0);
		break;
	case GUIAnchorTop:
		model.translate(0, hh - ehh, 0);
		break;
	case GUIAnchorTopRight:
		model.translate(hw - ehw, hh - ehh, 0);
		break;
	case GUIAnchorCenterLeft:
		model.translate(-hw + ehw, 0, 0);
		break;
	case GUIAnchorCenter:
		//model.translate(0,0,0); !!
		break;
	case GUIAnchorCenterRight:
		model.translate(hw - ehw, 0, 0);
		break;
	case GUIAnchorBottomLeft:
		model.translate(-hw + ehw, -hh + ehh, 0);
		break;
	case GUIAnchorBottom:
		model.translate(0, -hh + ehh, 0);
		break;
	case GUIAnchorBottomRight:
		model.translate(hw - ehw, -hh + ehh, 0);
		break;
	}
}

GLuint GUISystem::createTextTexture(std::string text, std::string font_path, int font_size, int tex_width, int tex_height) {

	//initialise freetype
	FT_Library ft;
	FT_Init_FreeType(&ft);

	//load a font-face
	FT_Face face;
	FT_New_Face(ft, font_path.c_str(), 0, &face);

	// width and height - width of 0 = automatic
	FT_Set_Pixel_Sizes(face, 0, font_size);

	GLuint texture_id;
	//create texture according to size
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	// disable default 4-byte alignment as freetype creates textures as single byte greyscale
	// so set byte-alignment to 1
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	//we will create an 'empty' arrayi.e. all values 0, the same size as
	//our texture, with an 8-bit type (unsigned char) and set it all to zero.
	std::vector<unsigned char> empty_data(tex_width * tex_height, 0);

	//create gl texture. Freetype glyphs are 8-bit greyscale intensities. So we will create a single chanel
	//(GL_RED) texture which stores GL_UNSIGNED_BYTEs.
	//data pointer (last argument) is to our empty array (if we pass 0 as final parameter, the data will be random!)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, tex_width, tex_height, 0, GL_RED, GL_UNSIGNED_BYTE, &(empty_data[0]));

	//first loop to get height of highest glyph in string
	int maxY = 0;
	for (char& c : text) {
		FT_Load_Char(face, c, FT_LOAD_RENDER);
		if (face->glyph->metrics.horiBearingY > maxY)
			maxY = face->glyph->metrics.horiBearingY;
	}

	//Freetype units are 1/64th of a pixel, bit-shifting an int by 6 bits divides it by 64 :)
	maxY = maxY >> 6;

	//now draw to openGL texture.
	//loop our string again, loading the glyphs and copying their data to the 'relevant' part of the gl texture
	//using glTexSubImage2D. 
	int x = 0; //current position in x (from left)
	int y = 0; //current position in y (from top)
	int lineY = 0;
	for (char& c : text) {
		//newline character means reset x and advance y by a line, then skip character
		if (c == '\n' || c == '\r') {
			x = 0;
			lineY += face->glyph->metrics.vertAdvance >> 6;
			continue;
		}
		//load glyph
		FT_Load_Char(face, c, FT_LOAD_RENDER);
		//set y to correct height
		y = lineY + maxY - face->glyph->bitmap_top;
		//draw only a portion of the image
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, face->glyph->bitmap.width, face->glyph->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
		//advance x by horizontal advance
		x += face->glyph->metrics.horiAdvance >> 6;
		//if x is wider than texture, newline
		if (x > tex_width) {
			x = 0;
			lineY += face->glyph->metrics.vertAdvance >> 6;
		}
	}

	// Set texture filtering options because we're not using mipmaps
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset alignment
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//clean up free type
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	//return
	return texture_id;
}

void GUISystem::key_mouse_callback(int key, int action, int mods) {
	if (key == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {

		auto& elements = ECS.getAllComponents<GUIElement>();
		for (auto& el : elements) {
			if (el.screen_bounds.pointInBounds(mouse_x_, mouse_y_)) {
				el.onClick();
			}
		}
	}
}

void GUISystem::updateViewport(int new_width, int new_height) {
	//update values
	width_ = new_width; height_ = new_height;
	//update vp
	view_projection.orthographic((float)-width_ / 2, (float)width_ / 2, (float)-height_ / 2, (float)height_ / 2, -0.5, -2);
}

void GUISystem::createGeometry_() {
	std::vector<GLfloat> vertices, uvs;
	std::vector<GLuint> indices;
	vertices = { -1.0f, -1.0f, 1.0f, 
		1.0f, -1.0f, 1.0f, 
		1.0f, 1.0f, 1.0f, 
		-1.0f, 1.0f, 1.0f };
	uvs = { 0.0f, 1.0f,   1.0f, 1.0f,    1.0f, 0.0f,   0.0f, 0.0f };
	indices = { 0, 1, 2, 0, 2, 3 };

	//generate the OpenGL buffers and create geometry
	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);
	GLuint vbo;
	//positions
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &(vertices[0]), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//texture coords
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(float), &(uvs[0]), GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &(indices[0]), GL_STATIC_DRAW);
	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
