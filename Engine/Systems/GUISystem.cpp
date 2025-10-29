#include "GUISystem.hpp"
#include "Systems/InputSystem.h"
#include "Systems/RenderSystem.h"
#include "Graphics/Graphics.hpp"
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>

using namespace mygame;

void GUISystem::AddButton(float x, float y, float w, float h, const std::string& label, std::function<void()> onClick)
{
    buttons.push_back({ x, y, w, h, label, onClick });
}

void GUISystem::Clear()
{
    buttons.clear();
}

void GUISystem::Update(Framework::InputSystem* input)
{
    if (!input) return;

    auto mouse = input->Manager().GetMouseState();
    const double mx = mouse.x;
    const double my = mouse.y;
    const bool clicked = input->IsMousePressed(GLFW_MOUSE_BUTTON_LEFT);

    up = down = left = right = false;

    for (auto& b : buttons)
    {
        b.hovered = (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h);

        if (b.hovered && clicked && b.onClick)
            b.onClick();

        if (b.label == "Up" && b.hovered && clicked) up = true;
        if (b.label == "Down" && b.hovered && clicked) down = true;
        if (b.label == "Left" && b.hovered && clicked) left = true;
        if (b.label == "Right" && b.hovered && clicked) right = true;
    }
}

void GUISystem::Draw(Framework::RenderSystem* render)
{
    if (!render) return;

    for (auto& b : buttons)
    {
        const float brightness = b.hovered ? 0.75f : 0.4f;

        // Button background using your custom renderer
        gfx::Graphics::renderRectangle(b.x, b.y, 0.f, b.w, b.h,
            brightness, brightness, brightness, 1.0f);

        // Button label via RenderSystem’s text renderer
        if (render->IsTextReadyHint())
        {
            render->GetTextHint().RenderText(
                b.label.c_str(),
                b.x + 10.f,
                b.y + (b.h * 0.5f) - 5.f,
                0.7f,
                glm::vec3(1.f, 1.f, 1.f)
            );
        }
    }
}
