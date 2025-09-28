#include "Core.hpp"

Core::Core(int width, int height, const char* title)
    : m_Running(false),
    // create window immediately (unique_ptr ensures RAII cleanup)
    m_Window(std::make_unique<gfx::Window>(width, height, title)) {}

void Core::Run() {
    m_Running = true;

    // Call user-defined init if provided (setup resources, GL state, etc.)
    if (init) init(*m_Window);

    auto t_prev = Clock::now();
    while (m_Running && !m_Window->shouldClose()) {
        // Process input/events (keyboard, mouse, OS signals)
        m_Window->pollEvents();

        // Measure frame delta time (in seconds, as float)
        const auto t_now = Clock::now();
        float dt = std::chrono::duration_cast<SecondsF>(t_now - t_prev).count();
        t_prev = t_now;

        // Clamp delta to avoid simulation explosion after stalls (>100 ms)
        if (dt > 0.1f) dt = 0.1f;

        // Game/logic update
        if (update) update(dt); 

        // Rendering stage
        m_Window->beginFrame();   // clear buffers, prepare GL state
        ImGuiLayer::BeginFrame();// start ImGui frame AFTER pollEvents and Before user render
        if (render) render();     // user drawing code
        ImGuiLayer::EndFrame();   // Draw ImGui last into the same framebuffer
        m_Window->endFrame();     // flush GL commands
        m_Window->swapBuffers();  // present frame to screen
    }

    // Cleanup stage
    if (shutdown) shutdown();
}

void Core::Quit() {
    // Allows external code to exit gracefully on next loop iteration
    m_Running = false;
}
