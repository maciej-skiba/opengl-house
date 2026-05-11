#pragma once
#include <string>
#include <vector>

unsigned int LoadTexture(char const* path);
std::string LoadShaderSource(const std::string& path);
unsigned int LoadCubemap(const std::vector<std::string>& facePaths);