/*********************************************************************************************
 \file      GUISystem.hpp
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Main Author, 100%
 \brief     Lightweight custom GUI system for buttons and mobile-style controls.
 \details   Handles button registration, click detection, and hover feedback using
            the engine’s InputSystem and RenderSystem. GUI buttons are fully custom-drawn
            rectangles and text rendered through gfx::Graphics and gfx::TextRenderer.
            The system can be used for main menus or in-game control overlays.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
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

namespace mygame
{
    struct GUIButton
    {
        float x, y, w, h;
        std::string label;
        std::function<void()> onClick;
        bool hovered = false;
    };

    class GUISystem
    {
    public:
        void AddButton(float x, float y, float w, float h,
            const std::string& label, std::function<void()> onClick);
        void Clear();
        void Update(Framework::InputSystem* input);
        void Draw(Framework::RenderSystem* render);

        bool MoveUp() const { return up; }
        bool MoveDown() const { return down; }
        bool MoveLeft() const { return left; }
        bool MoveRight() const { return right; }

    private:
        std::vector<GUIButton> buttons;
        bool up = false, down = false, left = false, right = false;
    };
}
