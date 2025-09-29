// Engine/Graphics/ImGuiLayer.cpp
#include "ImGuiLayer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

static bool s_imguiReady = false;

void ImGuiLayer::Initialize(gfx::Window& win, const ImGuiLayerConfig& cfg) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	// Options
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	if (cfg.gamepad)  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	if (cfg.dockspace) io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui::StyleColorsDark();

	// Backends
	GLFWwindow* native = win.raw();
	// true = install callbacks and chain to existing ones
	ImGui_ImplGlfw_InitForOpenGL(native, /*install_callbacks*/ true);
	ImGui_ImplOpenGL3_Init(cfg.glsl_version);

	s_imguiReady = true;
}

void ImGuiLayer::BeginFrame() {
	if (!s_imguiReady) return;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiLayer::EndFrame() {
	if (!s_imguiReady) return;
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::Shutdown() {
	if (!s_imguiReady) return;
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	s_imguiReady = false;
}
