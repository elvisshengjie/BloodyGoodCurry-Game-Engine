#pragma once
/*********************************************************************************************
 \file      Perf.h
 \par       SofaSpuds
 \author    
 \brief     Lightweight per-frame CPU timing HUD for debugging/performance profiling.
*********************************************************************************************/

namespace Framework {

   
    void FlipFrame();        // call once at start of each frame
    void setUpdate(double ms);
    void setRender(double ms);
    void setImGui(double ms);

   
    void DrawInCurrentWindow();

    
    void PerfFrameStart(float dt, bool toggleKeyDown);

   
    void DrawPerformanceWindow();
} // namespace Framework
