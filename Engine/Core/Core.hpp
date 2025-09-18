#pragma once
#include <memory>
#include <chrono>
#include "Graphics/Window.hpp"

class Core {
public:
    using InitFn = void(*)(gfx::Window&);
    using UpdateFn = void(*)(float);
    using RenderFn = void(*)();
    using ShutdownFn = void(*)();

    Core(int width, int height, const char* title);
    ~Core() = default;

   
    void Run();

    
    void Quit();

    
    void SetCallbacks(InitFn i, UpdateFn u, RenderFn r, ShutdownFn s) {
        init = i; update = u; render = r; shutdown = s;
    }

private:
    using Clock = std::chrono::steady_clock;
    using SecondsF = std::chrono::duration<float>;

    bool m_Running{ false };
    std::unique_ptr<gfx::Window> m_Window;

  
    InitFn     init{ nullptr };
    UpdateFn   update{ nullptr };
    RenderFn   render{ nullptr };
    ShutdownFn shutdown{ nullptr };
};
