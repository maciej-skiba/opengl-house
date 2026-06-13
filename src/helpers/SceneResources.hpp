#pragma once

#include <GLFW/glfw3.h>
#include "rendering/Model.hpp"

constexpr int numOfVerticesInBox = 36;

class SceneResources
{
private:
    const char* houseModelPath = "../assets/models/house/HouseSuburban.glb";
public:
    SceneResources() : houseModel(houseModelPath), terrainVAO(0), terrainVBO(0), terrainEBO(0)
    {
    }

    int terrainHeight, terrainWidth;
    GLuint terrainVAO, terrainVBO, terrainEBO;
    GLuint skyboxVao, skyboxVbo;
    unsigned int quadVAO, quadVBO;
    Model houseModel;
};