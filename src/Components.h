//
//  Copyright � 2018 Alun Evans. All rights reserved.
//
//  This file contains the definitions of an entity, and all the different types of component
//  At the end is a struct called the EntityComponentManager (ECM) which contains an std::vector for
//  each of the different component types, stored in an std::tuple. The advantage of this system is that
//  if a system wishes to interact/use/update all components of a certain type (e.g. draw all meshes),
//  then these components are stored in contiguous memory, which the various levels of caching can use
//  to improve performance
//
//    TO ADD A NEW COMPONENT TYPE:
//    - define it as a sub-class of Component
//    - add it to the ComponentArrays tuple
//    - add it as a subtemplate of typetoint() and increment 'result' variable
//    - increment NUM_TYPE_COMPONENTS
//
#pragma once
#include "includes.h"
#include <vector>
#include <functional>

/**** COMPONENTS ****/

//Component (base class)
// - owner: id of Entity which owns the instance of the component
struct Component {
    int owner;
    int index = -1;
};

// Transform Component
// - inherits a mat4 which represents a model matrix
// - all_transform - reference to vector of all transforms
struct Transform : public Component, public lm::mat4 {
    int parent = -1;
    lm::mat4 getGlobalMatrix(std::vector<Transform>& transforms) {
        if (parent != - 1){
            return transforms.at(parent).getGlobalMatrix(transforms) * *this;
        }
        else return *this;
    }
};

enum RenderMode {
    RenderModeForward = 0,
    RenderModeDeferred = 1
};

struct Mesh : public Component {
    int geometry;
    int material;
    RenderMode render_mode;
};


//Camera component
// - position, forward and up vectors for rapid access
// - view and projection matrices for camera
// - is_main bool so that ECM can quickly find main camera
struct Camera : public Component {
	lm::vec3 position;
	lm::vec3 forward;
	lm::vec3 up;
	lm::mat4 view_matrix;
	lm::mat4 projection_matrix;
	lm::mat4 view_projection;

	//constructor that sets placeholder matrices
	Camera() {
		position = lm::vec3(0.0f, 0.0f, 1.0f); forward = lm::vec3(0.0f, 0.0f, -1.0f); up = lm::vec3(0.0f, 1.0f, 0.0f);
		lm::vec3 target = position + forward;
		view_matrix.lookAt(position, target, up);
		projection_matrix.perspective(60.0f*DEG2RAD, 1, 0.01f, 100.0f);
	}

	//sets view and projection matrices based on current position, view direction and up vector
	void updateViewMatrix() {
		forward.normalize();
		lm::vec3 target = position + forward;
		view_matrix.lookAt(position, target, up);
	}

	//wraps orthographic projection matrix
	void setOrthographic(float left, float right, float bottom, float top, float near, float far) {
		projection_matrix.orthographic(left, right, bottom, top, near, far);
	}

	//wraps perspective projection matrix
	void setPerspective(float fov_rad, float aspect, float near, float far) {
		projection_matrix.perspective(fov_rad, aspect, near, far);
	}

	void update() {
		updateViewMatrix();
		view_projection = projection_matrix * view_matrix;
	}
};

//Light Component
// - id of transform in ECS array
struct Light : public Camera {
    //position is given by transform
    
    int type; //0 - directional; 1 - point; 2 - spot
    lm::vec3 direction;
    lm::vec3 color;
    float linear_att;
    float quadratic_att;
    float spot_inner;
    float spot_outer;
	int resolution;
    int cast_shadow;
	float radius = 0;
    
    Light() {
        type = 0;
        direction = lm::vec3(1.0f, 1.0f, 1.0f);
        color = lm::vec3(1.0f, 1.0f, 1.0f);
        linear_att = 0.09f;
        quadratic_att = 0.032f;
        spot_inner = 20.0f;
        spot_outer = 30.0f;
		resolution = 1024;
        cast_shadow = 0;
		calculateRadius();
    }

	void calculateRadius() {
		float lightMax = std::fmaxf(std::fmaxf(color.x, color.g), color.b);
		radius = (-linear_att + std::sqrtf(linear_att * linear_att - 
			4.0f * quadratic_att * (1.0f - (256.0f / 5.0f) * lightMax)))
			/ (2.0f * quadratic_att);
	}
};

enum ColliderType {
    ColliderTypeBox,
    ColliderTypeRay
};

//ColliderComponent. Only specifies size - collider location is given by any
//associated TransformComponent
// - collider_type is the type according to enum above
// - local_center is offset from transform
// - local_halfwidth is used for box,
// - direction is used for ray
// - max_distance is used to convert ray to segment
struct Collider: public Component {
    ColliderType collider_type;
    lm::vec3 local_center; //offset from transform component
    lm::vec3 local_halfwidth; // for box
    lm::vec3 direction; // for ray
    float max_distance; // for segment
    
    //collision state
    bool colliding;
    int other;
    lm::vec3 collision_point;
    float collision_distance;
    
    Collider() {
        local_halfwidth = lm::vec3(0.5, 0.5, 0.5); //default dimensions = 1 in each axis
        max_distance = 10000000.0f; //infinite ray by default
        colliding = false; // not colliding
        other = -1; //no other collider
    }
};

//Anchors GUI to part of screen
enum GUIAnchor {
	GUIAnchorTopLeft,
	GUIAnchorTop,
	GUIAnchorTopRight,
	GUIAnchorCenterLeft,
	GUIAnchorCenter,
	GUIAnchorCenterRight,
	GUIAnchorBottomLeft,
	GUIAnchorBottom,
	GUIAnchorBottomRight,
};

//screen space bounding box (for mouse click)
struct ScreenBounds {
	int x_min = 0;
	int x_max = 0;
	int y_min = 0;
	int y_max = 0;
	bool pointInBounds(int x, int y) {
		if (x > x_min && x < x_max && y > y_min && y < y_max)
			return true;
		return false;
	}
};

//GUI component
struct GUIElement : public Component {
	GLuint texture = 0;
	GLint width = 0;
	GLint height = 0;
	GUIAnchor anchor = GUIAnchorCenter;
	lm::vec2 offset;
	ScreenBounds screen_bounds;
	std::function<void()> onClick;
};

//Gui component only for text
struct GUIText : public GUIElement {
	std::string text = "";
	std::string font_face = "";
	int font_size = 32;
	lm::vec3 color = lm::vec3(1.0, 1.0, 1.0);
};

/**** COMPONENT STORAGE ****/

//add new component type vectors here to store them in *ECS*
typedef std::tuple<
std::vector<Transform>,
std::vector<Mesh>,
std::vector<Camera>,
std::vector<Light>,
std::vector<Collider>,
std::vector<GUIElement>,
std::vector<GUIText>
> ComponentArrays;

//way of mapping different types to an integer value i.e.
//the index within ComponentArrays
template< typename T >
struct type2int{};
template<> struct type2int<Transform> { enum { result = 0 }; };
template<> struct type2int<Mesh> { enum { result = 1 }; };
template<> struct type2int<Camera> { enum { result = 2 }; };
template<> struct type2int<Light> { enum { result = 3 }; };
template<> struct type2int<Collider> { enum { result = 4 }; };
template<> struct type2int<GUIElement> { enum { result = 5 }; };
template<> struct type2int<GUIText> { enum { result = 6 }; };
//UPDATE THIS!
const int NUM_TYPE_COMPONENTS = 7;

/**** ENTITY ****/

struct Entity {
    //name is used to store entity
    std::string name;
    //array of handles into ECM component arrays
    int components[NUM_TYPE_COMPONENTS];
    //sets active or not
    bool active = true;
    
    Entity() {
        for (int i = 0; i < NUM_TYPE_COMPONENTS; i++) { components[i] = -1;}
    }
    Entity(std::string a_name) : name(a_name) {
        for (int i = 0; i < NUM_TYPE_COMPONENTS; i++) { components[i] = -1;}
    }
};

