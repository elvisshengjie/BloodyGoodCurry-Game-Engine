/*********************************************************************************************
 \file      Core.cpp
 \par       SofaSpuds
 \author    yimo.kong (yimo.kong@digipen.edu) - Primary Author, 70%
            elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) 30%
 \brief     Minimal game/application core driving the main loop, timing, window events,
            and ImGui frame lifecycle.
 \details   Responsibilities:
            - Owns a gfx::Window via RAII and exposes user-supplied callbacks: init/update/render/shutdown.
            - Runs the main loop: poll events → compute/clamp dt → update → render.
            - Orders rendering with ImGui: beginFrame → ImGui BeginFrame → user render → ImGui EndFrame
              → endFrame → swapBuffers.
            - Provides Quit() to request a graceful exit on the next loop iteration.
            Notes:
            * dt is clamped to <= 0.1s to avoid simulation explosions after stalls.
            * Callbacks are checked for null before use, keeping Core lightweight and embeddable.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "Core.hpp"
#include "Debug/Perf.h" 
#include "Common/CRTDebug.h"   // <- bring in DBG_NEW
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#ifdef _DEBUG
#define new DBG_NEW       // <- redefine new AFTER all includes
#endif
Core::Core(int width, int height, const char* title, bool fullscreen)
    : m_Running(false),
    // create window immediately (unique_ptr ensures RAII cleanup)
    m_Window(std::make_unique<gfx::Window>(width, height, title, fullscreen)) {
}

void Core::FinalizeRun()
{
    if (m_Finalized)
        return;

    m_Finalized = true;
    if (shutdown)
        shutdown();
}

#if defined(__EMSCRIPTEN__)
void Core::WebMainLoop(void* userData)
{
    auto* core = static_cast<Core*>(userData);
    if (!core)
        return;

    core->TickOneFrame();
}
#endif

void Core::TickOneFrame()
{
    if (!m_Running || !m_Window || m_Window->shouldClose())
    {
        m_Running = false;
#if defined(__EMSCRIPTEN__)
        emscripten_cancel_main_loop();
#endif
        FinalizeRun();
        return;
    }

    // safety cap to avoid a spiral of death
    constexpr int kMaxSubSteps = 5;

    // Process input/events (keyboard, mouse, OS signals)
    m_Window->pollEvents();

    // Determine whether the application should be suspended.
    // Suspended when:
    //   • Window is iconified (minimized)   → IsIconified() == true
    //   • OR window has no focus (ALT-TAB, CTRL-ALT-DEL, Task Manager, etc.) → !HasFocus()
    // Browser focus semantics differ from desktop GLFW and can report unfocused at startup.
    // Do not suspend the web main loop based on focus state.
#if defined(__EMSCRIPTEN__)
    const bool suspended = false;
#else
    const bool suspended = m_Window->IsIconified() || !m_Window->HasFocus();
#endif

    // -----------------------------------------------------------------------------
    // ENTERING SUSPENDED STATE
    // -----------------------------------------------------------------------------
    if (suspended)
    {
        if (!m_WasSuspended && onSuspend)
            onSuspend(true);

        m_WasSuspended = true;
        m_Accumulator = SecondsF::zero();
        m_PreviousTick = Clock::now();
        return;
    }

    // -----------------------------------------------------------------------------
    // EXITING SUSPENDED STATE
    // -----------------------------------------------------------------------------
    if (m_WasSuspended)
    {
        if (onSuspend)
            onSuspend(false);

        m_Accumulator = SecondsF::zero();
        m_PreviousTick = Clock::now();
        m_WasSuspended = false;
    }

    // Measure frame delta time (in seconds, as float)
    const auto t_now = Clock::now();
    float frameDt = std::chrono::duration_cast<SecondsF>(t_now - m_PreviousTick).count();
    m_PreviousTick = t_now;

    // Clamp delta to avoid simulation explosion after stalls (>100 ms)
    if (frameDt > 0.1f)
        frameDt = 0.1f;

    Framework::PerfFrameStart(frameDt, false);

    // Accumulate elapsed time and step the simulation with a fixed timestep
    m_Accumulator += SecondsF{ frameDt };
    int subSteps = 0;
    while (m_Accumulator >= m_FixedStep && subSteps < kMaxSubSteps)
    {
        if (update)
            update(m_FixedStep.count());
        m_Accumulator -= m_FixedStep;
        ++subSteps;
    }

    // Drop excess time if we hit the cap to prevent spiral of death
    if (subSteps == kMaxSubSteps && m_Accumulator > m_FixedStep)
        m_Accumulator = SecondsF::zero();

    m_CurrentNumSteps = subSteps;

    // Rendering stage
    m_Window->beginFrame();    // clear buffers, prepare GL state
    ImGuiLayer::BeginFrame();  // start ImGui frame AFTER pollEvents and BEFORE user render
    if (render)
        render();      // user drawing code
    ImGuiLayer::EndFrame();    // draw ImGui last into the same framebuffer
    m_Window->endFrame();      // flush GL commands
    m_Window->swapBuffers();   // present frame to screen
}

/*************************************************************************************
 \brief  Enter the main loop and drive the app until the window closes or Quit() is called.
         Calls optional user hooks: init/update/render/shutdown.
*************************************************************************************/
void Core::Run() {
    m_Running = true;
    m_Finalized = false;
    m_CurrentNumSteps = 0;
    m_Accumulator = SecondsF::zero();
    m_PreviousTick = Clock::now();
    m_WasSuspended = false;

    // Call user-defined init if provided (setup resources, GL state, etc.)
    if (init) init(*m_Window);

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg(&Core::WebMainLoop, this, 0, 1);
#else
    while (m_Running && !m_Window->shouldClose())
    {
        TickOneFrame();
    }
    FinalizeRun();
#endif
}

/*************************************************************************************
 \brief  Request a graceful exit; the loop will stop on the next iteration.
*************************************************************************************/
void Core::Quit() {
    // Allows external code to exit gracefully on next loop iteration
    m_Running = false;
}
