#pragma once
#include <memory>
#include "Graphics/Window.hpp"

class Core {
public:
    Core(int width, int height, const char* title);
    ~Core() = default;

    // Main game loop
    void Run();

    // Stop the loop
    void Quit();

private:
    bool m_Running;
    std::unique_ptr<gfx::Window> m_Window;

   
    void Update(float dt);
    void Render();
};
