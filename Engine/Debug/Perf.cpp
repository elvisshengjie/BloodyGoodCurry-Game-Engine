/*********************************************************************************************
 \file      Perf.cpp
 \par       SofaSpuds
 \author    yimo.kong (yimo.kong@digipen.edu) - Primary Author, 100%
 \brief     Implements a lightweight per-frame CPU profiler for Update / Render / ImGui.
*********************************************************************************************/

#include "Perf.h"
#include "imgui.h"

/// \internal Anonymous namespace for private state
namespace {
    /// Aggregated timings for one frame.
    struct Values {
        double gUpdateMs = 0.0;   // CPU ms in Update
        double gRenderMs = 0.0;   // CPU ms in Render (aggregate)
        double gImGuIMs = 0.0;    // CPU ms in ImGui (build + draw)

        double TrackedTotal() const { return gUpdateMs + gRenderMs + gImGuIMs; }
    };

    // double-buffer for last/current frame timings
    static Values gCurr;   // being written this frame
    static Values gLast;   // shown by UI (previous frame)

    // overlay state (moved out of Game.cpp)
    static bool  sPerfVisible = true;     // toggled by F1 edge
    static bool  sPrevToggle = false;

    // FPS history ring buffer
    static float sFpsPlot[120] = { 0.f };
    static int   sFpsPlotIdx = 0;

    inline void pushFpsSample(float dt) {
        const float fpsNow = (dt > 1e-6f) ? (1.0f / dt) : 0.f;
        sFpsPlot[sFpsPlotIdx] = fpsNow;
        sFpsPlotIdx = (sFpsPlotIdx + 1) % (int)(sizeof(sFpsPlot) / sizeof(sFpsPlot[0]));
    }
} // anonymous namespace

// ---------- public API (unchanged) ----------
void Framework::FlipFrame() {
    gLast = gCurr;     // promote current to last
    gCurr = Values{};  // clear current for fresh measurements
}

void Framework::setUpdate(double ms) { gCurr.gUpdateMs = ms; }
void Framework::setRender(double ms) { gCurr.gRenderMs = ms; }
void Framework::setImGui(double ms) { gCurr.gImGuIMs = ms; }

// ---------- mini summary (embed-only, no Begin/End) ----------
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

// ---------- NEW: per-frame hook + full overlay window ----------
void Framework::PerfFrameStart(float dt, bool toggleKeyDown) {
    // edge toggle for visibility (e.g., F1)
    if (toggleKeyDown && !sPrevToggle) sPerfVisible = !sPerfVisible;
    sPrevToggle = toggleKeyDown;

    // roll last/current buffers at the start of the frame
    FlipFrame();

    // store FPS sample for plot
    pushFpsSample(dt);
}

void Framework::DrawPerformanceWindow() {
    if (!sPerfVisible) return;

    ImGui::SetNextWindowBgAlpha(0.7f);
    ImGui::Begin("Performance", nullptr,
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoFocusOnAppearing);

    // ImGui averaged framerate (current)
    const ImGuiIO& io = ImGui::GetIO();
    const float fps = io.Framerate;
    const float frameMs = (fps > 1e-6f) ? (1000.0f / fps) : 0.f;

    ImGui::Text("FPS: %.1f (%.2f ms)", fps, frameMs);
    ImGui::Separator();

    // show THIS frame's raw section times (gCurr)
    ImGui::Text("Update: %.2f ms", (float)gCurr.gUpdateMs);
    ImGui::Text("Render: %.2f ms", (float)gCurr.gRenderMs);
    ImGui::Text("ImGui : %.2f ms", (float)gCurr.gImGuIMs);

    // last ~120 FPS samples
    ImGui::PlotLines("FPS history", sFpsPlot, IM_ARRAYSIZE(sFpsPlot),
        sFpsPlotIdx, nullptr, 0.0f, 240.0f, ImVec2(260, 80));

    ImGui::Spacing();
    // also embed the "last frame" breakdown table
    Framework::DrawInCurrentWindow();

    ImGui::End();
}
