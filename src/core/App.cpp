#include "App.hpp"
#include "helpers/RenderResources.hpp"
#include "helpers/SceneResources.hpp"

bool App::Init()
{
    if (!Window::InitializeOpenGL(*&window))
    {
        std::cerr << "InitializeOpenGL() failed" << std::endl;

        return false;
    }

    Gui::ImGuiInit(window);
    glEnable(GL_DEPTH_TEST);

    this->renderer = std::make_unique<RenderResources>();
    this->scene = std::make_unique<SceneResources>();

    this->InitShaders(renderer.get());
    this->InitScene(scene.get());
    this->CreateFramebuffers(renderer.get());

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

void App::Update()
{
    Window::UpdateDeltaTime();

    if (renderer->antialiasingOn)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->multisampleFbo);
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->intermediateFbo);
    }

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    // -house-
    renderer->houseShader.UseProgram();
    renderer->houseModelMatrix = glm::translate(identityMatrix, renderer->terrainPosition);
    renderer->houseModelMatrix = glm::scale(renderer->houseModelMatrix, glm::vec3(0.002f));
    renderer->houseShader.SetUniformMat4("model", renderer->houseModelMatrix);
    renderer->houseShader.SetUniformMat4("view", renderer->mainCamera->GetViewMatrix());

    renderer->houseShader.SetUniformVec3("lightPosition", glm::vec3(0.0f, 5.0f, 0.0f));
    renderer->houseShader.SetUniformVec3("lightColor", glm::vec3(60.0f, 60.0f, 60.0f));
    renderer->houseShader.SetUniformVec3("camPos", renderer->mainCamera->Position);

    scene->houseModel.Draw(renderer->houseShader);

    // -------

    // -terrain-
    glBindVertexArray(scene->terrainVAO);

    renderer->terrainPolygonShader.UseProgram();
    
    renderer->terrainPolygonShader.SetUniformMat4("model", renderer->terrainModelMatrix);
    renderer->terrainPolygonShader.SetUniformMat4("view", renderer->mainCamera->GetViewMatrix());

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    // draw mesh
    const unsigned int NUM_STRIPS = scene->terrainHeight - 1;
    const unsigned int NUM_VERTS_PER_STRIP = scene->terrainWidth * 2;

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

    renderer->terrainShader.UseProgram();
    renderer->terrainShader.SetUniformMat4("model", renderer->terrainModelMatrix);
    renderer->terrainShader.SetUniformMat4("view", renderer->mainCamera->GetViewMatrix());
    renderer->terrainShader.SetUniformVec3("camPos", renderer->mainCamera->Position);

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
    renderer->skyboxShader.UseProgram();
    renderer->skyboxShader.SetUniformMat4("view", 
        glm::mat4(glm::mat3(renderer->mainCamera->GetViewMatrix())));
    glBindVertexArray(scene->skyboxVao);
    glBindTexture(GL_TEXTURE_CUBE_MAP, renderer->cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, numOfVerticesInBox);
    glDepthFunc(GL_LESS);
    
    // -------

    // --postprocessing--
    
    renderer->postprocessingShaders.at(renderer->currentPostProcShaderIndex).UseProgram();
    
    if (renderer->antialiasingOn)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer->multisampleFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->intermediateFbo);
        glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->intermediateFbo);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(scene->quadVAO);
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, renderer->textureColorBuffer);	// use the color attachment texture as the texture of the quad plane
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // ------------------

    Gui::ImGuiFrame(window);
    glfwSwapBuffers(window);
    ProcessInput(window, renderer->mainCamera.get(), renderer->currentPostProcShaderIndex, renderer->antialiasingOn);
    renderer->mainCamera->updateCameraVectors();
    glfwPollEvents();
}

void App::Terminate()
{
    glDeleteVertexArrays(1, &scene->terrainVAO);
    glDeleteVertexArrays(1, &scene->quadVAO);
    glDeleteVertexArrays(1, &scene->skyboxVao);
    glDeleteBuffers(1, &scene->terrainVBO);
    glDeleteBuffers(1, &scene->quadVBO);
    glDeleteBuffers(1, &scene->skyboxVbo);
    glDeleteFramebuffers(1, &renderer->multisampleFbo);
    glDeleteFramebuffers(1, &renderer->intermediateFbo);

    glfwTerminate();
    Gui::ImGuiShutdown();
}

void App::InitShaders(RenderResources* renderer)
{
    glfwSetWindowUserPointer(window, renderer->mainCamera.get());

    float lastFrame = 0.0f;
    float aspectRatio = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;
    float nearClippingPlane = 0.1f;
    float farClippingPlane = 100.0f;
    
    glm::mat4 terrainModelMatrix = identityMatrix;
    glm::mat4 houseModelMatrix = identityMatrix;
    glm::mat4 skyboxModelMatrix = identityMatrix;

    glm::mat4 projectionMatrix = 
        glm::perspective(
            glm::radians(renderer->mainCamera->Zoom),
            aspectRatio, 
            nearClippingPlane,
            farClippingPlane);

    renderer->terrainShader.UseProgram();
    renderer->terrainShader.SetUniformMat4("projection", projectionMatrix);
    
    // Terrain material
    renderer->terrainShader.SetUniformVec3("albedo", glm::vec3(0.12f, 0.48f, 0.07f));
    renderer->terrainShader.SetUniformFloat("metallic", 0.0f);
    renderer->terrainShader.SetUniformFloat("roughness", 0.9f);
    renderer->terrainShader.SetUniformFloat("ao", 1.0f);

    // Directional light
    renderer->terrainShader.SetUniformVec3("dirLightDirection", glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f)));
    renderer->terrainShader.SetUniformVec3("dirLightColor", glm::vec3(2.0f, 2.0f, 2.0f));

    // Fog
    renderer->terrainShader.SetUniformVec3("fogColor", glm::vec3(0.69f, 0.76f, 0.82f));
    renderer->terrainShader.SetUniformFloat("fogDensity", 0.006f);

    renderer->houseShader.UseProgram();
    renderer->houseShader.SetUniformMat4("projection", projectionMatrix);

    renderer->skyboxShader.UseProgram();
    renderer->skyboxShader.SetUniformMat4("projection", projectionMatrix);

    //
    renderer->terrainPolygonShader.UseProgram();
    renderer->terrainPolygonShader.SetUniformMat4("projection", projectionMatrix);
    renderer->terrainPolygonShader.SetUniformVec3("polygonModeColor", renderer->terrainPolygonColor);


    renderer->houseShader.UseProgram();
    renderer->houseShader.SetUniformMat4("projection", projectionMatrix);

    renderer->skyboxShader.UseProgram();
    renderer->skyboxShader.SetUniformMat4("projection", projectionMatrix);

    renderer->postProcShader1.UseProgram();
    renderer->postProcShader1.SetUniformInt("screenTexture", 0);
    
    renderer->postProcShader2.UseProgram();
    renderer->postProcShader2.SetUniformInt("screenTexture", 0);
    
    renderer->postProcShader3.UseProgram();
    renderer->postProcShader3.SetUniformInt("screenTexture", 0);

    renderer->postProcShader4.UseProgram();
    renderer->postProcShader4.SetUniformInt("screenTexture", 0);

    renderer->postProcShader5.UseProgram();
    renderer->postProcShader5.SetUniformInt("screenTexture", 0);
}

void App::InitScene(SceneResources* scene)
{
    CreateBoxVao(scene->skyboxVao, scene->skyboxVbo, skyboxVertices, boxBufferSize);

    //screen quad for postprocessing
    glGenVertexArrays(1, &scene->quadVAO);
    glGenBuffers(1, &scene->quadVBO);
    glBindVertexArray(scene->quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, scene->quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    GenerateTerrain(scene->terrainHeight, scene->terrainWidth, scene->terrainVAO, scene->terrainVBO, scene->terrainEBO);
}

void App::CreateFramebuffers(RenderResources* renderer)
{
    // ---CUSTOM FRAMEBUFFER OBJECTS---

    glGenFramebuffers(1, &renderer->multisampleFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->multisampleFbo);

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

    glGenFramebuffers(1, &renderer->intermediateFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->intermediateFbo);

    glGenTextures(1, &renderer->textureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, renderer->textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->textureColorBuffer, 0);

    glGenRenderbuffers(1, &renderer->rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, renderer->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WINDOW_WIDTH, WINDOW_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderer->rbo);
}