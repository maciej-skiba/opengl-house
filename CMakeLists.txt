cmake_minimum_required(VERSION 3.22)
project(opengl-house)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

set(CORE_FILES
    src/core/App.cpp
)

set(EXTERNAL_FILES
    src/external/imgui/imgui.cpp
    src/external/imgui/imgui_draw.cpp
    src/external/imgui/imgui_tables.cpp
    src/external/imgui/imgui_widgets.cpp
    src/external/imgui/backends/imgui_impl_glfw.cpp
    src/external/imgui/backends/imgui_impl_opengl3.cpp
)

set(IO_FILES
    src/io/FileLoader.cpp
    src/io/Input.cpp
    src/io/InputCallbacks.cpp
)

set(RENDERING_FILES
    src/rendering/Gui.cpp
    src/rendering/Mesh.cpp
    src/rendering/MeshUtils.cpp
    src/rendering/Model.cpp
    src/rendering/Window.cpp
)

add_executable(app
    src/main.cpp
    ${CORE_FILES}
    ${EXTERNAL_FILES}
    ${IO_FILES}
    ${RENDERING_FILES}
)

target_include_directories(app PUBLIC 
    src/
    src/core
    src/external
    src/external/glm
    src/external/imgui
    src/external/imgui/backends
    src/helpers
    src/io
    src/rendering
)

target_link_libraries(app
    PRIVATE
        OpenGL::GL
        glfw
        GLEW::GLEW
        glm::glm
        assimp
)

target_compile_features(app PRIVATE cxx_std_17)
