#pragma once
struct GLFWwindow;
#include "rendering/camera.hpp"

void ProcessInput(GLFWwindow* window, Camera* camera, short &postProcShaderIndex, bool &antialiasingOn);

extern bool flashlightOn;