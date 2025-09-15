#include "Core.hpp"
#include <chrono>
#include <iostream>

using Clock = std::chrono::steady_clock;
using SecondsF = std::chrono::duration<float>;

Core::Core(int width, int height, const char* title)
    : m_Running(false), m_Window(std::make_unique<gfx::Window>(width, height, title))
{
}

void Core::Run() {
    m_Running = true;
    auto t_prev = Clock::now();

    while (m_Running && !m_Window->shouldClose()) {
        m_Window->pollEvents();

        auto t_now = Clock::now();
        float dt = std::chrono::duration_cast<SecondsF>(t_now - t_prev).count();
        t_prev = t_now;
       
        Update(dt);

        m_Window->beginFrame();
        Render();
        m_Window->endFrame();
        m_Window->swapBuffers();
    }
}

void Core::Quit() {
    m_Running = false;
}

void Core::Update(float dt) {
    // TODO: Replace with game logic
    // For debug, print dt
    // std::cout << "Update, dt=" << dt << "\n";
}

void Core::Render() {
    // TODO: Replace with real rendering
    // For now, it just clears the screen (already done in beginFrame)
}
