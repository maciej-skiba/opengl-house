#include "core/App.hpp"
#include "helpers/gl_includes.hpp"

int main(void)
{    
    std::unique_ptr<App> app = std::make_unique<App>();

    if(!app->Init())
    {
        std::cerr << "\nApp init failed";

        return 1;
    }

    while (!glfwWindowShouldClose(app->window))
    {
        app->Update();
    }

    app->Terminate();

    return 0;
}