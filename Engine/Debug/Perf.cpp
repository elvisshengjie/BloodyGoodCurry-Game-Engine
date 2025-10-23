/*********************************************************************************************
 \file      Perf.cpp
 \par       SofaSpuds
 \author    yimo.kong (yimo.kong@digipen.edu) - Primary Author, 100%
 \brief     Implements a lightweight per-frame CPU profiler for Update / Render / ImGui.
            FPS is computed from the engine's Core dt (not ImGui).
*********************************************************************************************/

#include "Perf.h"
#include "imgui.h"
#include <algorithm>   // std::max
#include <cstddef>     // size_t

/// \internal Anonymous namespace for private state
namespace {
    /// Aggregated timings for one frame (CPU-side, milliseconds).
    struct Values {
        double gUpdateMs = 0.0;   // CPU ms in Update
        double gRenderMs = 0.0;   // CPU ms in Render (aggregate)
        double gImGuIMs = 0.0;    // CPU ms in ImGui (build + draw)
        double TrackedTotal() const { return gUpdateMs + gRenderMs + gImGuIMs; }
    };

    // Double-buffer for last/current measurements (so UI shows a stable "last frame").
    static Values gCurr;   // being written this frame
    static Values gLast;   // shown by UI (previous frame)

    // Overlay state (F1 edge-toggle)
    static bool  sPerfVisible = true;
    static bool  sPrevToggleKey = false;

    // Our own engine timing (from Core), in seconds
    static float sLastDtSec = 0.0f;

    // FPS history ring buffer (computed from our dt)
    static float sFpsPlot[120] = { 0.f };
    static int   sFpsPlotIdx = 0;

    // Simple moving average of FPS for a steadier readout
    static float sAvgFps = 0.0f;
    static int   sSamplesForAvg = 60;     // average over ~60 most recent samples (clamped by buffer size)

    inline float pushFpsSampleAndReturn(float dtSec) {
        sLastDtSec = dtSec;
        const float fpsNow = (dtSec > 1e-6f) ? (1.0f / dtSec) : 0.f;

        sFpsPlot[sFpsPlotIdx] = fpsNow;
        sFpsPlotIdx = (sFpsPlotIdx + 1) % (int)(sizeof(sFpsPlot) / sizeof(sFpsPlot[0]));

        // recompute moving average over up to sSamplesForAvg samples actually filled
        const int bufSize = (int)(sizeof(sFpsPlot) / sizeof(sFpsPlot[0]));
        const int count = std::max(1, std::min(sSamplesForAvg, bufSize));
        float sum = 0.f;
        for (int i = 0; i < count; ++i) sum += sFpsPlot[i];
        sAvgFps = sum / count;

        return fpsNow;
    }
} // anonymous namespace

// ---------- public API ----------
void Framework::FlipFrame() {
    gLast = gCurr;     // promote current to last
    gCurr = Values{};  // clear current for fresh measurements
}

void Framework::setUpdate(double ms) { gCurr.gUpdateMs = ms; }
void Framework::setRender(double ms) { gCurr.gRenderMs = ms; }
void Framework::setImGui(double ms) { gCurr.gImGuIMs = ms; }

// ---------- mini summary (embed-only, no Begin/End) ----------
void Framework::DrawInCurrentWindow() {
    const double totalTracked = gLast.TrackedTotal();
    const double denom = (totalTracked > 0.0) ? totalTracked : 0.0001; // avoid divide-by-zero

    ImGui::SeparatorText("Performance (last frame)");
    ImGui::Text("Tracked CPU total: %.2f ms", totalTracked);
    ImGui::TextDisabled("(no Core/swap/vsync/driver included)");
    ImGui::Spacing();

    ImGui::Text("Update:   %.3f ms (%.1f%%)", gLast.gUpdateMs, (gLast.gUpdateMs / denom) * 100.0);
    ImGui::Text("Render:   %.3f ms (%.1f%%)", gLast.gRenderMs, (gLast.gRenderMs / denom) * 100.0);
    ImGui::Text("ImGui:    %.3f ms (%.1f%%)", gLast.gImGuIMs, (gLast.gImGuIMs / denom) * 100.0);
}

// ---------- per-frame hook + full overlay window ----------
void Framework::PerfFrameStart(float dt, bool toggleKeyDown) {
    // Edge toggle for visibility (e.g., F1)
    if (toggleKeyDown && !sPrevToggleKey) sPerfVisible = !sPerfVisible;
    sPrevToggleKey = toggleKeyDown;

    // Roll last/current buffers at the start of the frame
    FlipFrame();

    // Store FPS sample for plot (OUR dt, not ImGui's)
    pushFpsSampleAndReturn(dt);
}

void Framework::DrawPerformanceWindow() {
    if (!sPerfVisible) return;

    ImGui::SetNextWindowBgAlpha(0.7f);
    ImGui::Begin("Performance", nullptr,
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoFocusOnAppearing);

    // Show our own FPS (derived from Core dt)
    const float fpsNow = (sLastDtSec > 1e-6f) ? (1.0f / sLastDtSec) : 0.f;
    const float frameMs = (fpsNow > 1e-6f) ? (1000.0f / fpsNow) : 0.f;
    const float avgMs = (sAvgFps > 1e-6f) ? (1000.0f / sAvgFps) : 0.f;

    ImGui::Text("Engine FPS: %.1f (%.2f ms)   |   Avg: %.1f (%.2f ms over ~%d frames)",
        fpsNow, frameMs, sAvgFps, avgMs, sSamplesForAvg);
    ImGui::TextDisabled("Derived from Core dt (full frame), not ImGui.");
    ImGui::Separator();

    // Show THIS frame's raw section times (gCurr)
    ImGui::Text("Update: %.2f ms", (float)gCurr.gUpdateMs);
    ImGui::Text("Render: %.2f ms", (float)gCurr.gRenderMs);
    ImGui::Text("ImGui : %.2f ms", (float)gCurr.gImGuIMs);

    // Compare measured frame time vs tracked CPU sections
    const double measuredFrameMs = sLastDtSec * 1000.0; // dt from Core
    const double trackedMs = gLast.TrackedTotal();
    const double untrackedMs = std::max(0.0, measuredFrameMs - trackedMs);
    ImGui::Separator();
    ImGui::Text("Measured frame (Core dt): %.2f ms", measuredFrameMs);
    ImGui::Text("Tracked sections total:   %.2f ms", trackedMs);
    ImGui::Text("Unaccounted remainder:    %.2f ms", untrackedMs);
    ImGui::TextDisabled("(swap buffers, vsync, driver/GPU queueing, etc.)");

    // Last ~120 FPS samples
    ImGui::Separator();
    ImGui::PlotLines("FPS history", sFpsPlot, IM_ARRAYSIZE(sFpsPlot),
        sFpsPlotIdx, nullptr, 0.0f, 240.0f, ImVec2(260, 80));

    ImGui::Spacing();
    // Also embed the "last frame" breakdown table
    Framework::DrawInCurrentWindow();

    ImGui::End();
}

// ---------- Optional helpers / getters ----------
void Framework::SetVisible(bool v) { sPerfVisible = v; }
void Framework::ToggleVisible() { sPerfVisible = !sPerfVisible; }
bool Framework::IsVisible() { return sPerfVisible; }

float Framework::GetLastDtSec() { return sLastDtSec; }
float Framework::GetFps() { return (sLastDtSec > 1e-6f) ? (1.0f / sLastDtSec) : 0.0f; }
float Framework::GetAvgFps() { return sAvgFps; }
void  Framework::SetFpsAvgWindow(int n) { sSamplesForAvg = (n <= 1) ? 1 : n; }
