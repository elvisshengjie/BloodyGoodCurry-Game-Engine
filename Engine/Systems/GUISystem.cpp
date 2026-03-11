/*********************************************************************************************
 \file      GUISystem.cpp
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Main Author, 100%
 \brief     Lightweight immediate-mode GUI system for in-game/menu buttons.
*********************************************************************************************/

#include "GUISystem.hpp"

#include "Graphics/Graphics.hpp"
#include "Systems/RenderSystem.h"

#include <GLFW/glfw3.h>

#include <functional>

#include "Common/CRTDebug.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif

namespace
{
    struct VisualRect
    {
        float x{};
        float y{};
        float w{};
        float h{};
    };

    constexpr float kHoverScale = 1.03f;
    constexpr float kPressedScale = 0.96f;
    constexpr float kIdleTint = 1.0f;
    constexpr float kPressedTint = 0.78f;
    constexpr float kHoverHighlightAlpha = 0.28f;
    constexpr float kPressedHighlightAlpha = 0.08f;
    constexpr double kPressFeedbackDuration = 0.12;

    VisualRect MakeVisualRect(const GUISystem::Button& button)
    {
        const float scale = button.pressed ? kPressedScale : (button.hovered ? kHoverScale : 1.0f);
        const float drawW = button.w * scale;
        const float drawH = button.h * scale;

        return {
            button.x - (drawW - button.w) * 0.5f,
            button.y - (drawH - button.h) * 0.5f,
            drawW,
            drawH
        };
    }
}

void GUISystem::Clear()
{
    buttons_.clear();
    prevMouseDown_ = false;
    activeButtonIndex_ = -1;
    callbackDispatchTime_ = 0.0;
    callbackPending_ = false;
}

void GUISystem::AddButton(float x, float y, float w, float h,
    const std::string& label,
    std::function<void()> onClick)
{
    Button b;
    b.x = x;
    b.y = y;
    b.w = w;
    b.h = h;
    b.label = label;
    b.onClick = std::move(onClick);
    buttons_.push_back(std::move(b));
}

void GUISystem::AddButton(float x, float y, float w, float h,
    const std::string& label,
    unsigned idleTexture,
    unsigned hoverTexture,
    std::function<void()> onClick,
    bool drawLabelOnTexture)
{
    AddButton(x, y, w, h, label, std::move(onClick));
    if (buttons_.empty()) {
        return;
    }

    Button& b = buttons_.back();
    b.idleTexture = idleTexture;
    b.hoverTexture = hoverTexture ? hoverTexture : idleTexture;
    b.useTextures = (b.idleTexture != 0);
    b.drawLabelOnTexture = drawLabelOnTexture;
}

bool GUISystem::Contains(const Button& b, double mx, double my)
{
    return (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h);
}

bool GUISystem::RisingEdgeLeftClick(bool now, bool& prev)
{
    const bool edge = (now && !prev);
    prev = now;
    return edge;
}

void GUISystem::Update(Framework::InputSystem* /*input*/)
{
    GLFWwindow* w = glfwGetCurrentContext();
    if (!w) {
        return;
    }

    double mxLogical = 0.0;
    double myTopLogical = 0.0;
    glfwGetCursorPos(w, &mxLogical, &myTopLogical);

    int winW = 1;
    int winH = 1;
    glfwGetWindowSize(w, &winW, &winH);

    int fbW = winW;
    int fbH = winH;
    glfwGetFramebufferSize(w, &fbW, &fbH);

    const double scaleX = winW > 0 ? static_cast<double>(fbW) / static_cast<double>(winW) : 1.0;
    const double scaleY = winH > 0 ? static_cast<double>(fbH) / static_cast<double>(winH) : 1.0;

    const double mx = mxLogical * scaleX;
    const double my = (static_cast<double>(winH) - myTopLogical) * scaleY;
    const bool mouseNow = (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    const bool clickedThisFrame = RisingEdgeLeftClick(mouseNow, prevMouseDown_);
    const double now = glfwGetTime();

    for (auto& b : buttons_) {
        b.hovered = Contains(b, mx, my);
        b.pressed = false;
    }

    if (!callbackPending_ && clickedThisFrame) {
        for (size_t i = 0; i < buttons_.size(); ++i) {
            if (buttons_[i].hovered && buttons_[i].onClick) {
                activeButtonIndex_ = static_cast<int>(i);
                callbackDispatchTime_ = now + kPressFeedbackDuration;
                callbackPending_ = true;
                break;
            }
        }
    }

    if (callbackPending_ && activeButtonIndex_ >= 0 && activeButtonIndex_ < static_cast<int>(buttons_.size())) {
        buttons_[activeButtonIndex_].pressed = true;
    }

    if (callbackPending_ && now >= callbackDispatchTime_) {
        std::function<void()> onClick;
        if (activeButtonIndex_ >= 0 && activeButtonIndex_ < static_cast<int>(buttons_.size())) {
            onClick = buttons_[activeButtonIndex_].onClick;
        }

        activeButtonIndex_ = -1;
        callbackDispatchTime_ = 0.0;
        callbackPending_ = false;

        if (onClick) {
            onClick();
        }
    }
}

void GUISystem::Draw(Framework::RenderSystem* render)
{
    const int screenW = render ? render->ScreenWidth() : 1280;
    const int screenH = render ? render->ScreenHeight() : 720;

    for (const auto& b : buttons_) {
        const VisualRect drawRect = MakeVisualRect(b);
        const float tint = b.pressed ? kPressedTint : kIdleTint;
        bool renderedTexture = false;

        if (b.useTextures) {
            const unsigned tex = (b.hovered && b.hoverTexture) ? b.hoverTexture : b.idleTexture;
            if (tex != 0) {
                gfx::Graphics::renderSpriteUI(tex, drawRect.x, drawRect.y, drawRect.w, drawRect.h,
                    tint, tint, tint, 1.0f, screenW, screenH);
                renderedTexture = true;

                if (b.pressed || b.hovered) {
                    const float highlightAlpha = b.pressed ? kPressedHighlightAlpha : kHoverHighlightAlpha;
                    gfx::Graphics::renderRectangleUI(drawRect.x, drawRect.y, drawRect.w, drawRect.h,
                        1.0f, 1.0f, 1.0f, highlightAlpha, screenW, screenH);
                }
            }
        }

        if (!renderedTexture) {
            float c = 0.55f;
            if (b.hovered) {
                c = 0.88f;
            }
            if (b.pressed) {
                c = 0.38f;
            }

            gfx::Graphics::renderRectangleUI(drawRect.x, drawRect.y, drawRect.w, drawRect.h,
                c, c, c, 0.95f, screenW, screenH);
        }

        const bool shouldDrawLabel = (!b.useTextures || b.drawLabelOnTexture || !renderedTexture);
        if (shouldDrawLabel && render && render->IsTextReadyHint()) {
            const float labelX = drawRect.x + 24.0f;
            const float labelY = drawRect.y + (drawRect.h * 0.5f) - 8.0f;
            const float textTint = b.pressed ? 0.9f : 1.0f;
            render->GetTextHint().RenderText(b.label.c_str(), labelX, labelY, 0.9f, { textTint, textTint, textTint });
        }
    }
}
