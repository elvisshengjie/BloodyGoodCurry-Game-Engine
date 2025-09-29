#pragma once
#include "Graphics/Window.hpp"


struct ImGuiLayerConfig {
	const char* glsl_version = "#version 330"; // set to what your context supports
	bool dockspace = true;                     // optional docking flag
	bool gamepad = false;                     // enable if you want
};

class ImGuiLayer {
public:
	static void Initialize(gfx::Window& win, const ImGuiLayerConfig& cfg = {});
	static void BeginFrame();   // call each frame (after pollEvents)
	static void EndFrame();     // call each frame (before swapBuffers)
	static void Shutdown();
};