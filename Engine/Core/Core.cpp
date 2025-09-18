#include "Core.hpp"

Core::Core(int width, int height, const char* title)
    : m_Running(false),
    m_Window(std::make_unique<gfx::Window>(width, height, title)) {}

void Core::Run() {
    m_Running = true;

    if (init) init(*m_Window);

    auto t_prev = Clock::now();
    while (m_Running && !m_Window->shouldClose()) {
        m_Window->pollEvents();

        const auto t_now = Clock::now();
        float dt = std::chrono::duration_cast<SecondsF>(t_now - t_prev).count();
        t_prev = t_now;

        
        if (dt > 0.1f) dt = 0.1f;

        if (update) update(dt);

        m_Window->beginFrame();
        if (render) render();
        m_Window->endFrame();
        m_Window->swapBuffers();
    }

    if (shutdown) shutdown();
}

void Core::Quit() { m_Running = false; }
