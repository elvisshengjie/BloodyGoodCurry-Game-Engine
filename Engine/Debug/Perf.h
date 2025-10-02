#pragma once
/*********************************************************************************************
 \file      Perf.h
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%
 \brief     Lightweight per-frame CPU timing HUD for debugging/performance profiling.
*********************************************************************************************/

namespace Framework {

    // ---- frame bookkeeping (same as之前的API，不变) --------------------------------------
    void FlipFrame();        // call once at start of each frame
    void setUpdate(double ms);
    void setRender(double ms);
    void setImGui(double ms);

    // ---- mini summary：把“上帧汇总/占比”画到已打开的 ImGui 面板中（无 Begin/End） ----
    void DrawInCurrentWindow();

    // ---- 新增：把整块 Performance 窗口做成独立模块（含 Begin/End） --------------------
    // 在 update() 顶部调用：推入 FPS 样本、处理 F1 边沿切换可见性，并内部调用 FlipFrame()
    void PerfFrameStart(float dt, bool toggleKeyDown);

    // 在 draw() 的 ImGui 构建阶段调用：如果可见则绘制完整的 Performance 窗口
    void DrawPerformanceWindow();
} // namespace Framework
