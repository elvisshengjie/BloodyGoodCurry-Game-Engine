#include "MainMenuPage.hpp"
#include "Graphics/Graphics.hpp"
#include "Resource_Manager/Resource_Manager.h"
#include <GLFW/glfw3.h>
#include <algorithm>

static bool RisingEdgeLeftClick() {
    if (GLFWwindow* w = glfwGetCurrentContext()) {
        static bool prev = false;
        const bool now = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool edge = (now && !prev);
        prev = now;
        return edge;
    }
    return false;
}

bool MainMenuPage::Contains(const RectF& r, double mx, double my) {
    return (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h);
}

void MainMenuPage::Init(int screenW, int screenH)
{
    sw = screenW; sh = screenH;
    hoverStart = hoverExit = startLatched = exitLatched = false;

    // Try Resource_Manager first (nice to reuse cache), fallback to raw load.
    Resource_Manager::load("menu_bg", "../../assets/Textures/menu.jpg");
    menuBgTex = Resource_Manager::resources_map["menu_bg"].handle;
    if (!menuBgTex) {
        menuBgTex = gfx::Graphics::loadTexture("../../assets/Textures/menu.jpg");
    }
}

void MainMenuPage::Update(Framework::InputSystem* /*input*/)
{
    GLFWwindow* w = glfwGetCurrentContext();
    if (!w) return;

    double mx, myTop; glfwGetCursorPos(w, &mx, &myTop);
    const double my = static_cast<double>(sh) - myTop; // bottom-left origin

    hoverStart = Contains(startBtn, mx, my);
    hoverExit = Contains(exitBtn, mx, my);

    if (RisingEdgeLeftClick()) {
        if (hoverStart) startLatched = true;
        if (hoverExit)  exitLatched = true;
    }
}

void MainMenuPage::Draw(Framework::RenderSystem* render)
{
    // --- draw full-screen menu background via the background pipeline ---
    if (menuBgTex) {
        gfx::Graphics::renderFullscreenTexture(menuBgTex);
    }
    const float lift = 14.f; // move everything up ~14 px
    // --- then draw your buttons/labels as before ---
    const float cs = hoverStart ? 0.78f : 0.45f;
    const float ce = hoverExit ? 0.78f : 0.45f;
    // buttons
    gfx::Graphics::renderRectangle(startBtn.x, startBtn.y + lift, 0.f, startBtn.w, startBtn.h, cs, cs, cs, 1.f);
    gfx::Graphics::renderRectangle(exitBtn.x, exitBtn.y + lift, 0.f, exitBtn.w, exitBtn.h, ce, ce, ce, 1.f);

    /*if (render && render->IsTextReadyTitle())
        render->GetTextTitle().RenderText("SOFA SPUDS", 58.f, 60.f, 1.15f, { 1.f,1.f,0.f });*/
    if (render && render->IsTextReadyHint()) {
        // labels
        render->GetTextHint().RenderText("Start",
            startBtn.x + 24.f,
            startBtn.y + lift + startBtn.h * 0.5f - 8.f,
            0.9f, { 1.f,1.f,1.f });

        render->GetTextHint().RenderText("Exit",
            exitBtn.x + 24.f,
            exitBtn.y + lift + exitBtn.h * 0.5f - 8.f,
            0.9f, { 1.f,1.f,1.f });
        const float drop = 20.f; // pixels to move down
        render->GetTextHint().RenderText(
            "Click a button to continue",
            58.f,                 // x stays
            120.f - drop,         // y lower
            0.55f, { 0.9f,0.9f,0.9f }
        );

    }
}

bool MainMenuPage::ConsumeStart() { if (!startLatched) return false; startLatched = false; return true; }
bool MainMenuPage::ConsumeExit() { if (!exitLatched) return false; exitLatched = false; return true; }
