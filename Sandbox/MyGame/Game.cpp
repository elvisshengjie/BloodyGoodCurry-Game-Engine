// Sandbox/MyGame/Game.cpp
#include "../../Engine/Graphics/Window.hpp"  
#include "Game.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>

using Clock = std::chrono::steady_clock;
using SecondsF = std::chrono::duration<float>;

namespace mygame {

    void run() {
        gfx::Window win(800, 600, "MyGame");

        auto t_prev = Clock::now();
        while (!win.shouldClose()) {
            win.pollEvents();

            const auto t_now = Clock::now();
            const float dt = std::chrono::duration_cast<SecondsF>(t_now - t_prev).count();
            t_prev = t_now;

            // ---- update(dt) ----
            (void)dt;
            
            win.beginFrame();
            // ---- render() ----
            win.endFrame();
            win.swapBuffers();
        }
    }

} // namespace mygame
