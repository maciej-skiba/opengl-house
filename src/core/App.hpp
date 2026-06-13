#pragma once

#include <iostream>
#include <memory>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <unordered_map>

#include "helpers/vertices.hpp" 
#include "helpers/Config.hpp"

#include "io/FileLoader.hpp"
#include "io/InputCallbacks.hpp"
#include "io/Input.hpp"

#include "rendering/camera.hpp"
#include "rendering/MeshUtils.hpp"
#include "rendering/Model.hpp"
#include "rendering/Gui.hpp"
#include "rendering/Shader.hpp"
#include "rendering/Window.hpp"

#include "RenderResources.hpp"
#include "SceneResources.hpp"

constexpr int boxBufferSize = 36 * 6;
const glm::mat4 identityMatrix = glm::mat4(1.0f);

class App
{
private:
    std::unique_ptr<SceneResources> scene;
    std::unique_ptr<RenderResources> renderer;

    void InitShaders(RenderResources* renderer);
    void InitScene(SceneResources* scene);
    void CreateFramebuffers(RenderResources* renderer);
    void PickFramebuffer(RenderResources* renderer);
    void DrawHouse(RenderResources* renderer, SceneResources* scene);
    void DrawTerrain(RenderResources* renderer, SceneResources* scene);
    void DrawSkyBox(RenderResources* renderer, SceneResources* scene);
    void ApplyPostprocessing(RenderResources* renderer, SceneResources* scene);
public:
    App() = default;

    GLFWwindow* window = nullptr;

    bool Init();
    void Update();
    void Terminate();
};