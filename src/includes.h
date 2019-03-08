//
//  includes.h
//  02-GameLoop
//
//  Copyright © 2018 Alun Evans. All rights reserved.
//
#pragma once

//imGUI defined before glfw, tell it we're using GLEW loader
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include "linmath.h"
#include "shaders_default.h"


bool glCheckError();
void print(lm::vec3 v);
void print(std::string s);
void print(float f);
void print(int i);