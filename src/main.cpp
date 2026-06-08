#include "common/gl_includes.hpp"
#include <iostream>
#include <memory>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <unordered_map>

#include "vertices.hpp" 
#include "camera.hpp"

#include "app/Config.hpp"
#include "core/Window.hpp"
#include "core/InputCallbacks.hpp"
#include "gfx/Input.hpp"
#include "gfx/MeshUtils.hpp"
#include "gfx/Model.hpp"
#include "gfx/Attenuation.hpp"
#include "gfx/Gui.hpp"
#include "io/FileLoader.hpp"
#include "Shader.hpp"

const glm::mat4 identityMatrix = glm::mat4(1.0f);

int main(void)
{    
    GLFWwindow* window;
    int initSuccess = 1;

    if (Window::InitializeOpenGL(window) != initSuccess)
    {
        return -1;
    }

    Gui::ImGuiInit(window);

    unsigned int skyboxVao, skyboxVbo;
    
    const char* terrainVertShaderPath = "../shaders/terrain/terrain_vert.glsl";
    const char* terrainFragShaderPath = "../shaders/terrain/terrain_frag.glsl";

    Shader terrainShader(terrainVertShaderPath, terrainFragShaderPath);

    const char* terrainPolygonVertShaderPath = "../shaders/terrain/terrain_polygon_vert.glsl";
    const char* terrainPolygonFragShaderPath = "../shaders/terrain/terrain_polygon_frag.glsl";

    Shader terrainPolygonShader(terrainPolygonVertShaderPath, terrainPolygonFragShaderPath);

    const char* houseVertShaderPath = "../shaders/house/house_vert.glsl";
    const char* houseFragShaderPath = "../shaders/house/house_frag.glsl";
        
    Shader houseShader(houseVertShaderPath, houseFragShaderPath);

    const char* skyboxVertShaderPath = "../shaders/skybox/skybox_vert.glsl";
    const char* skyboxFragShaderPath = "../shaders/skybox/skybox_frag.glsl";
        
    Shader skyboxShader(skyboxVertShaderPath, skyboxFragShaderPath);

    int numOfVerticesInBox = 36;

    glm::vec3 terrainPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 terrainColor =  glm::vec3(0.12f, 0.48f, 0.07f);  //dark green
    glm::vec3 terrainPolygonColor =  glm::vec3(0.75f, 1.0f, 0.75f); //light green
    
    int boxBufferSize = numOfVerticesInBox * 6;
    CreateBoxVao(skyboxVao, skyboxVbo, skyboxVertices, boxBufferSize);

    //screen quad for postprocessing
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glEnable(GL_DEPTH_TEST);

    std::unique_ptr<Camera> mainCamera = std::make_unique<Camera>(
        glm::vec3(-4.0f, 1.4f,  4.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));

    glfwSetWindowUserPointer(window, mainCamera.get());

    float lastFrame = 0.0f;
    float aspectRatio = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;
    float nearClippingPlane = 0.1f;
    float farClippingPlane = 100.0f;

    glm::mat4 terrainModelMatrix = identityMatrix;
    glm::mat4 houseModelMatrix = identityMatrix;
    glm::mat4 skyboxModelMatrix = identityMatrix;

    glm::mat4 projectionMatrix = 
        glm::perspective(
            glm::radians(mainCamera->Zoom),
            aspectRatio, 
            nearClippingPlane,
            farClippingPlane);

    terrainShader.UseProgram();
    terrainShader.SetUniformMat4("projection", projectionMatrix);
    
    // Terrain material
    terrainShader.SetUniformVec3("albedo", glm::vec3(0.12f, 0.48f, 0.07f));
    terrainShader.SetUniformFloat("metallic", 0.0f);
    terrainShader.SetUniformFloat("roughness", 0.9f);
    terrainShader.SetUniformFloat("ao", 1.0f);

    // Directional light
    terrainShader.SetUniformVec3("dirLightDirection", glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f)));
    terrainShader.SetUniformVec3("dirLightColor", glm::vec3(2.0f, 2.0f, 2.0f));

    // Fog
    terrainShader.SetUniformVec3("fogColor", glm::vec3(0.69f, 0.76f, 0.82f));
    terrainShader.SetUniformFloat("fogDensity", 0.006f);

    houseShader.UseProgram();
    houseShader.SetUniformMat4("projection", projectionMatrix);

    skyboxShader.UseProgram();
    skyboxShader.SetUniformMat4("projection", projectionMatrix);

    //
    terrainPolygonShader.UseProgram();
    terrainPolygonShader.SetUniformMat4("projection", projectionMatrix);
    terrainPolygonShader.SetUniformVec3("polygonModeColor", terrainPolygonColor);


    houseShader.UseProgram();
    houseShader.SetUniformMat4("projection", projectionMatrix);

    skyboxShader.UseProgram();
    skyboxShader.SetUniformMat4("projection", projectionMatrix);

    // ---CUSTOM FRAMEBUFFER OBJECTS---

    unsigned int multisampleFbo;
    glGenFramebuffers(1, &multisampleFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, multisampleFbo);

    unsigned int msTextureColorBuffer;
    glGenTextures(1, &msTextureColorBuffer);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msTextureColorBuffer);
    int numOfAntialiasingSamples = 4;
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numOfAntialiasingSamples, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msTextureColorBuffer, 0);

    unsigned int msRbo;
    glGenRenderbuffers(1, &msRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, msRbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, numOfAntialiasingSamples, GL_DEPTH24_STENCIL8, WINDOW_WIDTH, WINDOW_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    unsigned int intermediateFbo;
    glGenFramebuffers(1, &intermediateFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFbo);

    unsigned int textureColorBuffer;
    glGenTextures(1, &textureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WINDOW_WIDTH, WINDOW_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    const char* postProcVertShaderPath_1 = "../shaders/postprocessing/postProc_vert.glsl";
    const char* postProcFragShaderPath_1 = "../shaders/postprocessing/postProc_frag1.glsl";

    Shader postProcShader1(postProcVertShaderPath_1, postProcFragShaderPath_1);
    postProcShader1.UseProgram();
    postProcShader1.SetUniformInt("screenTexture", 0);

    const char* postProcVertShaderPath_2 = "../shaders/postprocessing/postProc_vert.glsl";
    const char* postProcFragShaderPath_2 = "../shaders/postprocessing/postProc_frag2.glsl";

    Shader postProcShader2(postProcVertShaderPath_2, postProcFragShaderPath_2);
    postProcShader2.UseProgram();
    postProcShader2.SetUniformInt("screenTexture", 0);
    
    const char* postProcVertShaderPath_3 = "../shaders/postprocessing/postProc_vert.glsl";
    const char* postProcFragShaderPath_3 = "../shaders/postprocessing/postProc_frag3.glsl";

    Shader postProcShader3(postProcVertShaderPath_3, postProcFragShaderPath_3);
    postProcShader3.UseProgram();
    postProcShader3.SetUniformInt("screenTexture", 0);

    const char* postProcVertShaderPath_4 = "../shaders/postprocessing/postProc_vert.glsl";
    const char* postProcFragShaderPath_4 = "../shaders/postprocessing/postProc_frag4.glsl";

    Shader postProcShader4(postProcVertShaderPath_4, postProcFragShaderPath_4);
    postProcShader4.UseProgram();
    postProcShader4.SetUniformInt("screenTexture", 0);

    const char* postProcVertShaderPath_5 = "../shaders/postprocessing/postProc_vert.glsl";
    const char* postProcFragShaderPath_5 = "../shaders/postprocessing/postProc_frag5.glsl";

    Shader postProcShader5(postProcVertShaderPath_5, postProcFragShaderPath_5);
    postProcShader5.UseProgram();
    postProcShader5.SetUniformInt("screenTexture", 0);

    std::unordered_map<short, Shader> postprocessingShaders =
    {
        { 1, postProcShader1 },
        { 2, postProcShader2 },
        { 3, postProcShader3 },
        { 4, postProcShader4 },
        { 5, postProcShader5 },
    };
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ---------------------------------

    const char* houseModelPath = "../assets/models/house/HouseSuburban.glb";
    Model houseModel(houseModelPath);

    std::cout << "Entering main loop\n";

    short currentPostProcShaderIndex = 1;

    glm::vec3 dirLightPosition = glm::vec3(-20.0f, 50.0f, 0.0f);
    glm::vec3 dirLightColor = glm::vec3(1.0f, 1.0f, 1.0f);

    bool antialiasingOn = true;
    std::vector<std::string> facePaths
    {
        "../assets/textures/skybox/right.png",
        "../assets/textures/skybox/left.png",
        "../assets/textures/skybox/top.png",
        "../assets/textures/skybox/bottom.png",
        "../assets/textures/skybox/front.png",
        "../assets/textures/skybox/back.png"
    };
    
    unsigned int cubemapTexture = LoadCubemap(facePaths); 

    int terrainHeight, terrainWidth;
    GLuint terrainVAO, terrainVBO, terrainEBO;
    GenerateTerrain(terrainHeight, terrainWidth, terrainVAO, terrainVBO, terrainEBO);
    const unsigned int NUM_STRIPS = terrainHeight - 1;
    const unsigned int NUM_VERTS_PER_STRIP = terrainWidth * 2;

    while (!glfwWindowShouldClose(window))
    {
        Window::UpdateDeltaTime();

        if (antialiasingOn)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, multisampleFbo);
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, intermediateFbo);
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        
        // -house-
        houseShader.UseProgram();
        houseModelMatrix = glm::translate(identityMatrix, terrainPosition);
        houseModelMatrix = glm::scale(houseModelMatrix, glm::vec3(0.002f));
        houseShader.SetUniformMat4("model", houseModelMatrix);
        houseShader.SetUniformMat4("view", mainCamera->GetViewMatrix());

        houseShader.SetUniformVec3("lightPosition", glm::vec3(0.0f, 5.0f, 0.0f));
        houseShader.SetUniformVec3("lightColor", glm::vec3(60.0f, 60.0f, 60.0f));
        houseShader.SetUniformVec3("camPos", mainCamera->Position);

        houseModel.Draw(houseShader);

        // -------

        // -terrain-
        glBindVertexArray(terrainVAO);

        terrainPolygonShader.UseProgram();
      
        terrainPolygonShader.SetUniformMat4("model", terrainModelMatrix);
        terrainPolygonShader.SetUniformMat4("view", mainCamera->GetViewMatrix());

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        // draw mesh
    
        // render the mesh triangle strip by triangle strip - each row at a time
        for(unsigned int strip = 0; strip < NUM_STRIPS; ++strip)
        {
            glDrawElements(GL_TRIANGLE_STRIP,   // primitive type
                NUM_VERTS_PER_STRIP, // number of indices to render
                GL_UNSIGNED_INT,     // index data type
                (void*)(sizeof(unsigned int)
                * NUM_VERTS_PER_STRIP
                * strip)); // offset to starting index
        }

        terrainShader.UseProgram();
        terrainShader.SetUniformMat4("model", terrainModelMatrix);
        terrainShader.SetUniformMat4("view", mainCamera->GetViewMatrix());
        terrainShader.SetUniformVec3("camPos", mainCamera->Position);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        for(unsigned int strip = 0; strip < NUM_STRIPS; ++strip)
        {
            glDrawElements(GL_TRIANGLE_STRIP,   // primitive type
                NUM_VERTS_PER_STRIP, // number of indices to render
                GL_UNSIGNED_INT,     // index data type
                (void*)(sizeof(unsigned int)
                * NUM_VERTS_PER_STRIP
                * strip)); // offset to starting index
        }

        // -------

        // --skybox--

        glDepthFunc(GL_LEQUAL);
        skyboxShader.UseProgram();
        skyboxShader.SetUniformMat4("view", 
            glm::mat4(glm::mat3(mainCamera->GetViewMatrix())));
        glBindVertexArray(skyboxVao);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, numOfVerticesInBox);
        glDepthFunc(GL_LESS);
        
        // -------

        // --postprocessing--
        
        postprocessingShaders.at(currentPostProcShaderIndex).UseProgram();
        
        if (antialiasingOn)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampleFbo);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFbo);
            glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, intermediateFbo);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(quadVAO);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, textureColorBuffer);	// use the color attachment texture as the texture of the quad plane
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // ------------------

        Gui::ImGuiFrame(window);
        glfwSwapBuffers(window);
        ProcessInput(window, mainCamera.get(), currentPostProcShaderIndex, antialiasingOn);
        mainCamera->updateCameraVectors();
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
    glDeleteFramebuffers(1, &multisampleFbo);

    glfwTerminate();
    Gui::ImGuiShutdown();
    return 0;
}