#pragma once
/*********************************************************************************************
 \file      Perf.h
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%
 \brief     Lightweight per-frame CPU timing HUD for debugging/performance profiling.

 \copyright
    All content © 2025 DigiPen Institute of Technology Singapore.
    All rights reserved.
*********************************************************************************************/

namespace Framework {

    /***************************************************************************************
     \brief   Roll frame buffers: make last = current, then clear current for a new frame.
     \details Call exactly once at the start of each frame (e.g., at the top of update()).
              This ensures the on-screen panel (DrawInCurrentWindow) displays a complete
              set of metrics from the previous frame, including ImGui’s own cost.
     \note    If you forget to call this, the panel may display stale or zeroed values.
    ***************************************************************************************/
    void FlipFrame();

    /***************************************************************************************
     \brief   Store the current frame’s CPU time spent in the **Update** section.
     \param   ms  Elapsed time in milliseconds for Update (this frame).
     \details Measure Update like:
                \code
                  auto t0 = clock::now();
                  // ... your update work ...
                  Framework::setUpdate(std::chrono::duration<double, std::milli>(clock::now()-t0).count());
                \endcode
    ***************************************************************************************/
    void setUpdate(double ms);

    /***************************************************************************************
     \brief   Store the current frame’s CPU time spent in the **Render** section (aggregate).
     \param   ms  Elapsed time in milliseconds for your overall render block (this frame).
     \details If you prefer a finer breakdown (BG/Sprites/Rects/Circles), you can either:
              - Replace this with multiple setters in your own extension, or
              - Treat this as the total render time measured from the start to end of draw().
    ***************************************************************************************/
    void setRender(double ms);

    /***************************************************************************************
     \brief   Store the current frame’s CPU time spent in **ImGui** building/drawing.
     \param   ms  Elapsed time in milliseconds for ImGui (this frame).
     
    ***************************************************************************************/
    void setImGui(double ms);

    /***************************************************************************************
     \brief   Draw the performance summary into the **currently open** ImGui window.
     \details Renders a compact table showing the “last frame” timings and percentages of the
              tracked total. Call this inside an existing ImGui window (no Begin/End inside).
     
    ***************************************************************************************/
    void DrawInCurrentWindow();

} // namespace Framework
