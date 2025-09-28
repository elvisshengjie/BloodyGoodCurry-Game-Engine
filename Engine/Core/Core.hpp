#pragma once
#include <memory>
#include <chrono>
#include "Graphics/Window.hpp"
#include "Debug/ImGuiLayer.h"
class Core {
public:
    // Callback types (simple C-style function pointers).
    // Each is optional (nullptr by default).
    using InitFn = void(*)(gfx::Window&); // Called once at startup
    using UpdateFn = void(*)(float);        // Called every frame with delta time
    using RenderFn = void(*)();             // Called every frame to draw
    using ShutdownFn = void(*)();             // Called once at shutdown

    // Constructor: creates a window of given width, height, and title.
    Core(int width, int height, const char* title);
    ~Core() = default; // default destructor is fine (unique_ptr cleans up)

    // Runs the main application loop until Quit() is called or window closes.
    void Run();

    // Requests loop termination (sets m_Running = false).
    void Quit();

    // Sets the lifecycle callbacks (all at once).
    // Alternative design: provide individual setters for flexibility.
    void SetCallbacks(InitFn i, UpdateFn u, RenderFn r, ShutdownFn s) {
        init = i; update = u; render = r; shutdown = s;
    }

private:
    using Clock = std::chrono::steady_clock;      // monotonic clock (safe for dt)
    using SecondsF = std::chrono::duration<float>;   // time duration in float seconds

    bool m_Running{ false };                         // main loop flag
    std::unique_ptr<gfx::Window> m_Window;           // owns the window object

    // Callback storage
    InitFn     init{ nullptr };
    UpdateFn   update{ nullptr };
    RenderFn   render{ nullptr };
    ShutdownFn shutdown{ nullptr };
};
