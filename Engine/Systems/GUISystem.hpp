/*********************************************************************************************
 \file      GUISystem.h
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Primary Author, 100%
 \brief     Immediate-mode GUI for clickable buttons (text or textured).
 \details   Manages a flat list of buttons, updates hover and rising-edge click state from
            InputSystem, and renders via RenderSystem. Absolute positioning is used; the
            renderer interprets coordinates and draws rectangles or textures as needed.
 \copyright
            All content (c)2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Framework {
    class InputSystem;
    class RenderSystem;
}

class GUISystem {
public:
    struct Button {
        float x{}, y{}, w{}, h{};
        std::string label;
        std::function<void()> onClick;
        bool hovered{ false };
        bool pressed{ false };
        unsigned idleTexture{ 0 };
        unsigned hoverTexture{ 0 };
        bool useTextures{ false };
        bool drawLabelOnTexture{ false };
        std::function<void()> onHover;
        bool wasHovered = false;
    };

    void Clear();

    void AddButton(float x, float y, float w, float h,
        const std::string& label,
        std::function<void()> onClick);

    void AddButton(float x, float y, float w, float h,
        const std::string& label,
        unsigned idleTexture,
        unsigned hoverTexture,
        std::function<void()> onClick,
        bool drawLabelOnTexture = false);

    void Update(Framework::InputSystem* input);
    void Draw(Framework::RenderSystem* render);
    void SetLastHoverCallback(std::function<void()> onHover);
    void SetSelectSoundCallback(std::function<void()> onSelect);

private:
    std::vector<Button> buttons_;
    bool prevMouseDown_{ false };
    int activeButtonIndex_{ -1 };
    double callbackDispatchTime_{ 0.0 };
    bool callbackPending_{ false };
    std::function<void()> pendingCallback_;
    std::function<void()> onSelectSound_;

    static bool Contains(const Button& b, double mx, double my);
    static bool RisingEdgeLeftClick(bool now, bool& prev);
};
