#pragma once
#include "includes.h"
#include "Shader.h"
#include <vector>
#include "GraphicsSystem.h"


struct TransformNode {
	std::vector<TransformNode> children;
	int trans_id;
	int entity_owner;
	std::string ent_name;
	bool isTop = false;
};

class DebugSystem {
public:
	
	~DebugSystem();
	void init(GraphicsSystem* gs);
	void lateInit();
	void update(float dt);

	void setActive(bool a);

	//public imGUI functions
	bool isShowGUI() { return show_imGUI_; };
	void toggleimGUI() { show_imGUI_ = !show_imGUI_; };

	//set picking ray
	void setPickingRay(int mouse_x, int mouse_y, int screen_width, int screen_height);

private:
	//graphics system pointer
	GraphicsSystem* graphics_system_;
	
	//bools to draw or not
	bool draw_grid_;
	bool draw_icons_;
	bool draw_frustra_;
	bool draw_colliders_;

	//cube for frustra and boxes
	void createCube_();
	GLuint cube_vao_;

	//colliders
	void createRay_();
	GLuint collider_ray_vao_;
	GLuint collider_box_vao_;

	//icons
	void createIcon_();
	GLuint icon_vao_;
	GLuint icon_light_texture_;
	GLuint icon_camera_texture_;

	//grid
	void createGrid_();
	GLuint grid_vao_;
	GLuint grid_num_indices;
	float grid_colors[12] = {
		0.7f, 0.7f, 0.7f, //grey
		1.0f, 0.5f, 0.5f, //red
		0.5f, 1.0f, 0.5f, //green
		0.5f, 0.5f, 1.0f }; //blue

							//shaders
	Shader* grid_shader_;
	Shader* icon_shader_;

	//imGUI
	void imGuiRenderTransformNode(TransformNode& trans);
	bool show_imGUI_ = false;
	void updateimGUI_(float dt);

	//picking
	bool can_fire_picking_ray_ = true;
	int ent_picking_ray_;
	
};

