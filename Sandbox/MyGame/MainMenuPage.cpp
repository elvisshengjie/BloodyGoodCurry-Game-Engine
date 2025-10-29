#include "MainMenuPage.hpp"
#include "Systems/RenderSystem.h"    // using getters: IsTextReadyTitle/Hint, GetTextTitle/Hint
#include "Systems/InputSystem.h"     // not strictly needed (we use GLFW directly), but fine
#include "Graphics/Graphics.hpp"     // gfx::Graphics::renderRectangle
#include <glm/vec3.hpp>
#include <GLFW/glfw3.h>

using namespace mygame;

// Detect PRESS edge (true only once per click)
static bool RisingEdgeLeftClick()
{
    GLFWwindow* w = glfwGetCurrentContext();
    if (!w) return false;
    static bool prev = false;
    const bool now = (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    const bool edge = (now && !prev);
    prev = now;
    return edge;
}

bool MainMenuPage::Contains(const RectF& r, double mx, double my)
{
    return (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h);
}

void MainMenuPage::Init(int screenW, int screenH)
{
    sw = screenW;
    sh = screenH;

    // Clear state
    hoverStart = false;
    hoverExit = false;
    startLatched = false;
    exitLatched = false;

    // (Optional) center buttons:
    // const float cx = sw * 0.5f - 130.f;
    // startBtn = { cx, sh * 0.40f, 260.f, 80.f };
    // exitBtn  = { cx, sh * 0.55f, 260.f, 80.f };
}

void MainMenuPage::Update(Framework::InputSystem* /*input*/)
{
    GLFWwindow* w = glfwGetCurrentContext();
    if (!w) return;

    // Window coords (origin top-left)
    double mx, myTop;
    glfwGetCursorPos(w, &mx, &myTop);

    // Convert to render coords (origin bottom-left)
    const double my = static_cast<double>(sh) - myTop;

    // Hover checks
    hoverStart = Contains(startBtn, mx, my);
    hoverExit = Contains(exitBtn, mx, my);

    // Latch clicks (consumed in Game.cpp)
    if (RisingEdgeLeftClick())
    {
        if (hoverStart) startLatched = true;
        if (hoverExit)  exitLatched = true;
    }
}

void MainMenuPage::Draw(Framework::RenderSystem* render)
{
    // Button rectangles
    const float cs = hoverStart ? 0.78f : 0.45f;
    const float ce = hoverExit ? 0.78f : 0.45f;

    gfx::Graphics::renderRectangle(startBtn.x, startBtn.y, 0.f, startBtn.w, startBtn.h, cs, cs, cs, 1.f);
    gfx::Graphics::renderRectangle(exitBtn.x, exitBtn.y, 0.f, exitBtn.w, exitBtn.h, ce, ce, ce, 1.f);

    // Text via your RenderSystem (uses Roboto you already load)
    if (render && render->IsTextReadyTitle())
    {
        render->GetTextTitle().RenderText(
            "SOFA SPUDS",
            100.f, 60.f,
            1.2f,
            glm::vec3(1.f, 1.f, 0.f)
        );
    }

    if (render && render->IsTextReadyHint())
    {
        render->GetTextHint().RenderText(
            "Click Start to enter the game",
            100.f, 120.f,
            0.75f,
            glm::vec3(0.9f, 0.9f, 0.9f)
        );

        render->GetTextHint().RenderText(
            "Start",
            startBtn.x + 24.f,
            startBtn.y + startBtn.h * 0.5f - 8.f,
            0.9f,
            glm::vec3(1.f, 1.f, 1.f)
        );

        render->GetTextHint().RenderText(
            "Exit",
            exitBtn.x + 24.f,
            exitBtn.y + exitBtn.h * 0.5f - 8.f,
            0.9f,
            glm::vec3(1.f, 1.f, 1.f)
        );
    }
}

bool MainMenuPage::ConsumeStart()
{
    if (!startLatched) return false;
    startLatched = false;
    return true;
}

bool MainMenuPage::ConsumeExit()
{
    if (!exitLatched) return false;
    exitLatched = false;
    return true;
}
