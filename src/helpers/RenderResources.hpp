#pragma once

#include "Shader.hpp"
#include <memory>
#include "camera.hpp"

class RenderResources
{
private:
    const char* terrainVertShaderPath = "../shaders/terrain/terrain_vert.glsl";
    const char* terrainFragShaderPath = "../shaders/terrain/terrain_frag.glsl";

    const char* terrainPolygonVertShaderPath = "../shaders/terrain/terrain_polygon_vert.glsl";
    const char* terrainPolygonFragShaderPath = "../shaders/terrain/terrain_polygon_frag.glsl";

    const char* houseVertShaderPath = "../shaders/house/house_vert.glsl";
    const char* houseFragShaderPath = "../shaders/house/house_frag.glsl";

    const char* skyboxVertShaderPath = "../shaders/skybox/skybox_vert.glsl";
    const char* skyboxFragShaderPath = "../shaders/skybox/skybox_frag.glsl";

    const char* postProcVertShaderPath_1 = "../shaders/postprocessing/postProc_vert.glsl";
    const char* postProcFragShaderPath_1 = "../shaders/postprocessing/postProc_frag1.glsl";

    const char* postProcVertShaderPath_2 = "../shaders/postprocessing/postProc_vert.glsl";
    const char* postProcFragShaderPath_2 = "../shaders/postprocessing/postProc_frag2.glsl";

    const char* postProcVertShaderPath_3 = "../shaders/postprocessing/postProc_vert.glsl";
    const char* postProcFragShaderPath_3 = "../shaders/postprocessing/postProc_frag3.glsl";

    const char* postProcVertShaderPath_4 = "../shaders/postprocessing/postProc_vert.glsl";
    const char* postProcFragShaderPath_4 = "../shaders/postprocessing/postProc_frag4.glsl";

    const char* postProcVertShaderPath_5 = "../shaders/postprocessing/postProc_vert.glsl";
    const char* postProcFragShaderPath_5 = "../shaders/postprocessing/postProc_frag5.glsl";

    std::vector<std::string> facePaths
    {
        "../assets/textures/skybox/right.png",
        "../assets/textures/skybox/left.png",
        "../assets/textures/skybox/top.png",
        "../assets/textures/skybox/bottom.png",
        "../assets/textures/skybox/front.png",
        "../assets/textures/skybox/back.png"
    };

    glm::mat4 identityMatrix = glm::mat4(1.0f);

public:
    RenderResources() :
        terrainShader(terrainVertShaderPath, terrainFragShaderPath),
        terrainPolygonShader(terrainPolygonVertShaderPath, terrainPolygonFragShaderPath),
        houseShader(houseVertShaderPath, houseFragShaderPath),
        skyboxShader(skyboxVertShaderPath, skyboxFragShaderPath),
        postProcShader1(postProcVertShaderPath_1, postProcFragShaderPath_1),
        postProcShader2(postProcVertShaderPath_2, postProcFragShaderPath_2),
        postProcShader3(postProcVertShaderPath_3, postProcFragShaderPath_3),
        postProcShader4(postProcVertShaderPath_4, postProcFragShaderPath_4),
        postProcShader5(postProcVertShaderPath_5, postProcFragShaderPath_5),
        postprocessingShaders({         
            { 1, postProcShader1 },
            { 2, postProcShader2 },
            { 3, postProcShader3 },
            { 4, postProcShader4 },
            { 5, postProcShader5 },}),
        cubemapTexture(LoadCubemap(facePaths))
    {
    }

    Shader terrainShader;
    Shader terrainPolygonShader;
    Shader houseShader;
    Shader skyboxShader;

    Shader postProcShader1;
    Shader postProcShader2;
    Shader postProcShader3;
    Shader postProcShader4;
    Shader postProcShader5;

    std::unordered_map<short, Shader> postprocessingShaders;

    GLuint multisampleFbo;
    GLuint intermediateFbo;
    GLuint textureColorBuffer;
    GLuint rbo;
    GLuint cubemapTexture;

    glm::vec3 dirLightPosition = glm::vec3(-20.0f, 50.0f, 0.0f);
    glm::vec3 dirLightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 terrainPolygonColor = glm::vec3(0.75f, 1.0f, 0.75f);
    glm::vec3 terrainPosition = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::mat4 houseModelMatrix = identityMatrix;
    glm::mat4 terrainModelMatrix = identityMatrix;
    glm::mat4 skyboxModelMatrix = identityMatrix;
    
    std::unique_ptr<Camera> mainCamera = std::make_unique<Camera>(
        glm::vec3(-4.0f, 1.4f,  4.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));

    bool antialiasingOn = true;

    short currentPostProcShaderIndex = 1;
};