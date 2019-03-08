//
//  Game.h
//  02-GameLoop
//
//  Copyright � 2018 Alun Evans. All rights reserved.
//
#pragma once
#include "includes.h"
#include "GraphicsSystem.h"
#include "ControlSystem.h"
#include "DebugSystem.h"
#include "CollisionSystem.h"
#include "ScriptSystem.h"
#include "GUISystem.h"



class Game
{
public:
	Game();
	void init(int, int);
	void update(float dt);

	//pass input straight to input system, if we are not showing Debug GUI
	void updateMousePosition(int new_x, int new_y) {
		mouse_x_ = new_x; mouse_y_ = new_y;
		if (!debug_system_.isShowGUI()) {
			control_system_.updateMousePosition(new_x, new_y);
			gui_system_.updateMousePosition(new_x, new_y);
		}
	}
	void key_callback(int key, int scancode, int action, int mods) {

		if (key == GLFW_KEY_0 && action == GLFW_PRESS && mods == GLFW_MOD_ALT)
			debug_system_.toggleimGUI();

		if (!debug_system_.isShowGUI())
			control_system_.key_mouse_callback(key, action, mods);
	}
	void mouse_button_callback(int button, int action, int mods) {
		if (!debug_system_.isShowGUI()) {
			control_system_.key_mouse_callback(button, action, mods);
			gui_system_.key_mouse_callback(button, action, mods);
		}
		else
			debug_system_.setPickingRay(mouse_x_, mouse_y_, window_width_, window_height_);
	}
	void update_viewports(int window_width, int window_height);

private:
	GraphicsSystem graphics_system_;
	ControlSystem control_system_;
    DebugSystem debug_system_;
    CollisionSystem collision_system_;
    ScriptSystem script_system_;
	GUISystem gui_system_;

	int createFreeCamera_();
	int createPlayer_(float aspect, ControlSystem& sys);

	int window_width_;
	int window_height_;
	int mouse_x_;
	int mouse_y_;
};
