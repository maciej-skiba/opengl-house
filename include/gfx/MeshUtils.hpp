#pragma once
#include <GL/glew.h>

void CreateBoxVao(unsigned int& VAO, unsigned int& VBO, float* boxVertices, int bufferSize);
void GenerateTerrain(int &height, int &width, GLuint &terrainVAO, GLuint &terrainVBO, GLuint &terrainEBO);
