#pragma once
#include "includes.h"
#include "Components.h"
#include <map>

//struct to store mouse state
struct Mouse {
	int x;
	int y;
	int delta_x, delta_y;
};

enum ControlType {
	ControlTypeFree,
	ControlTypeFPS,
	ControlTypeOrbit
};

//System which manages all our controls
class ControlSystem {
public:
	void init();
	void update(float dt);

	//functions called directly from main.cpp, via game
	void updateMousePosition(int new_x, int new_y);
	void key_mouse_callback(int key, int action, int mods);

	//current active control type
	ControlType control_type = ControlTypeFPS;

	//public functions to get key and mouse
	bool GetKey(int code) { return input[code]; }
	bool GetButton(int code) { return input[code]; }
	//mouse is public, it's just four ints
	Mouse mouse;

	//FPS stuff
	int FPS_collider_down;
	int FPS_collider_left;
	int FPS_collider_right;
	int FPS_collider_forward;
	int FPS_collider_back;
	bool FPS_can_jump = true;
	float FPS_jump_force = 0.0f;
	float FPS_jump_initial_force = 12.0f;
	float FPS_jump_force_slowdown = 7.0f;
	float FPS_gravity = 9.8f;
	float FPS_height = 2.0f;

private:
	float move_speed_ = 10.0f;
	float turn_speed_ = 2.3f;

	bool input[GLFW_KEY_LAST];

	//function to update entity movement
	void updateFree(float dt);
	void updateFPS(float dt);
};
