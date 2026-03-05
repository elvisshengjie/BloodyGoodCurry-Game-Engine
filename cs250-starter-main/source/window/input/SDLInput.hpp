/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <gsl/gsl>
#include <vector>
#include <window/input/Environment.hpp>

struct SDL_Window;
typedef union SDL_Event SDL_Event;

namespace window::input
{
    class SDLInput
    {
    public:
        using MouseButtons    = window::input::MouseButtons;
        using KeyboardButtons = window::input::KeyboardButtons;

    public:

        void Begin() noexcept;
        void HandleEvent(const SDL_Event& event, gsl::not_null<SDL_Window*> sdl_window, int viewport_x, int viewport_y) noexcept;
        void End() noexcept;

        std::span<const MouseButtons> LastReleasedMouseButtons() const noexcept
        {
            return std::span<const MouseButtons>{ lastReleasedMouseButtons };
        }

        std::span<const KeyboardButtons> LastReleasedKeyboardButtons() const noexcept
        {
            return std::span<const KeyboardButtons>{ lastReleasedKeyboardButtons };
        }

        float LastMouseVelocityX() const noexcept
        {
            return lastMouseVelocityX;
        }

        float LastMouseVelocityY() const noexcept
        {
            return lastMouseVelocityY;
        }

        float LastMouseWheel() const noexcept
        {
            return lastMouseWheel;
        }

    private:

        std::vector<MouseButtons>    pressedMouseButtons;
        std::vector<MouseButtons>    releasedMouseButtons;
        std::vector<MouseButtons>    lastReleasedMouseButtons;
        std::vector<KeyboardButtons> pressedKeyboardButtons;
        std::vector<KeyboardButtons> releasedKeyboardButtons;
        std::vector<KeyboardButtons> lastReleasedKeyboardButtons;
        float                        lastMouseVelocityX = 0;
        float                        lastMouseVelocityY = 0;
        float                        lastMouseWheel     = 0;

        void ProcessKeyboardEvent(const SDL_Event& event);
        void ProcessMouseEvent(const SDL_Event& event);
    };
}
