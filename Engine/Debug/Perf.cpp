/*********************************************************************************************
 \file      Perf.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%
 \brief     Implements a lightweight per-frame CPU profiler for Update / Render / ImGui.
 \details
    This module provides a tiny “tracked CPU time?HUD that you can embed in any ImGui
    window. It uses a double-buffer scheme:
      - Call \c FlipFrame() once at the start of each frame (e.g., top of update()) to
        copy the previous frame’s “current?values into a “last?buffer for display, and
        clear “current?for fresh measurements.
      - After timing each section within the frame, call \c setUpdate(), \c setRender(),
        and \c setImGui() with elapsed milliseconds for that section.
      - Call \c DrawInCurrentWindow() inside an already-open ImGui window to display the
        **last frame’s** totals and percentages (avoids measuring the panel as 0 ms).

    \note This profiler shows the **sum of tracked CPU sections only** (Option B). It does
          **not** include input polling, buffer swap, or VSync wait unless you explicitly
          measure and feed them.

 \copyright
    All content ?2025 DigiPen Institute of Technology Singapore.
    All rights reserved.
*********************************************************************************************/

#include "Perf.h"
#include "imgui.h"

/// \internal Anonymous namespace for private state
namespace {
    /// \brief Aggregated timings for one frame.
    struct Values {
        double gUpdateMs = 0.0;  ///< CPU ms spent in Update
        double gRenderMs = 0.0;  ///< CPU ms spent in Render (aggregate)
        double gImGuIMs = 0.0;   ///< CPU ms spent in ImGui (build + draw)

        /// \brief Sum of tracked sections (no core/swap/vsync).
        double TrackedTotal() const {
            return gUpdateMs + gRenderMs + gImGuIMs;
        }
    };

    static Values gCurr; ///< Values being written this frame
    static Values gLast; ///< Values shown by the UI (previous frame)
} // anonymous namespace

/*************************************************************************************
  \brief Rolls frame buffers: make last = current, then clear current.
  \details
    Call once at the start of every frame (e.g., top of update()) so the UI
    always shows a complete **previous** frame, including ImGui time.
*************************************************************************************/
void Framework::FlipFrame() {
    gLast = gCurr;     // Promote current to last (to be displayed)
    gCurr = Values{};  // Clear current for fresh measurements
}

/*************************************************************************************
  \brief Records the current frame’s CPU time spent in Update.
  \param ms Elapsed milliseconds for the Update section (this frame).
*************************************************************************************/
void Framework::setUpdate(double ms) {
    gCurr.gUpdateMs = ms;
}

/*************************************************************************************
  \brief Records the current frame’s CPU time spent in Render (aggregate).
  \param ms Elapsed milliseconds for the Render section (this frame).
  \note  If you want a finer breakdown (BG/Sprites/Rects/Circles), either
         replace this with additional setters or accumulate into this one.
*************************************************************************************/
void Framework::setRender(double ms) {
    gCurr.gRenderMs = ms;
}

/*************************************************************************************
  \brief Records the current frame’s CPU time spent in ImGui.
  \param ms Elapsed milliseconds for the ImGui section (this frame).
  \details Time the block where you build/draw your ImGui (e.g., Spawn panel,
           demo window, other debug UIs) and pass the elapsed value here.
*************************************************************************************/
void Framework::setImGui(double ms) {
    gCurr.gImGuIMs = ms;
}

/*************************************************************************************
  \brief Renders the last frame’s tracked CPU totals and percentages.
  \details
    Draws a compact readout into the **currently open** ImGui window.
    This function does not call Begin/End; call it inside an existing panel.
  \note
    - Totals reflect the sum of tracked sections only (no core/swap/vsync).
    - Percentages are section_ms / tracked_total * 100.
*************************************************************************************/
void Framework::DrawInCurrentWindow() {
    const double total = gLast.TrackedTotal();
    const double denom = total > 0.0 ? total : 0.0001; // avoid divide-by-zero

    ImGui::SeparatorText("Performance (last frame)");
    ImGui::Text("Tracked CPU total: %.2f ms", total);
    ImGui::TextDisabled("(no Core/swap/vsync included)");
    ImGui::Spacing();

    ImGui::Text("Update:   %.3f ms (%.1f%%)", gLast.gUpdateMs, (gLast.gUpdateMs / denom) * 100.0);
    ImGui::Text("Render:   %.3f ms (%.1f%%)", gLast.gRenderMs, (gLast.gRenderMs / denom) * 100.0);
    ImGui::Text("ImGui:    %.3f ms (%.1f%%)", gLast.gImGuIMs, (gLast.gImGuIMs / denom) * 100.0);
}
