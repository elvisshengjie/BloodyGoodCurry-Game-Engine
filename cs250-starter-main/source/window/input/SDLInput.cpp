/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "SDLInput.hpp"
#include "Environment.hpp"
#include "window/Environment.hpp"
#include <SDL.h>
#include <algorithm>

namespace
{
    window::input::KeyboardButtons sdl_scancode_to_button(SDL_Scancode scancode) noexcept;
}

namespace window::input
{
    void SDLInput::Begin() noexcept
    {
        if (window::input::MouseWheel != 0)
        {
            lastMouseWheel = window::input::MouseWheel;
        }
        if (window::input::MouseVelocityX != 0)
        {
            lastMouseVelocityX = window::input::MouseVelocityX;
        }
        if (window::input::MouseVelocityY != 0)
        {
            lastMouseVelocityY = window::input::MouseVelocityY;
        }

        if (!releasedMouseButtons.empty())
        {
            lastReleasedMouseButtons = releasedMouseButtons;
        }

        if (!releasedKeyboardButtons.empty())
        {
            lastReleasedKeyboardButtons = releasedKeyboardButtons;
        }

        window::input::MouseWheel     = 0;
        window::input::MouseVelocityX = 0;
        window::input::MouseVelocityY = 0;

        releasedMouseButtons.clear();
        releasedKeyboardButtons.clear();
    }

    void SDLInput::HandleEvent(const SDL_Event& event, gsl::not_null<SDL_Window*> sdl_window, int viewport_x, int viewport_y) noexcept
    {
        switch (event.type)
        {
            case SDL_KEYDOWN:
            case SDL_KEYUP: ProcessKeyboardEvent(event); break;
            case SDL_MOUSEWHEEL: window::input::MouseWheel = static_cast<float>(event.wheel.preciseY); break;
            case SDL_MOUSEMOTION:
                if (event.motion.windowID == SDL_GetWindowID(sdl_window))
                {
                    window::input::MouseWindowX   = static_cast<float>(event.motion.x);
                    window::input::MouseWindowY   = static_cast<float>(event.motion.y);
                    window::input::MouseVelocityX = static_cast<float>(event.motion.xrel);
                    window::input::MouseVelocityY = static_cast<float>(event.motion.yrel);
                    window::input::MouseDisplayX  = window::input::MouseWindowX * window::HorizontalDPIScale - static_cast<float>(viewport_x);
                    window::input::MouseDisplayY  = (static_cast<float>(window::WindowHeight) - window::input::MouseWindowY) * window::VerticalDPIScale - static_cast<float>(viewport_y);
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: ProcessMouseEvent(event); break;
        }
    }

    void SDLInput::End() noexcept
    {
        window::input::PressedMouseButtons     = std::span{ pressedMouseButtons };
        window::input::ReleasedMouseButtons    = std::span{ releasedMouseButtons };
        window::input::PressedKeyboardButtons  = std::span{ pressedKeyboardButtons };
        window::input::ReleasedKeyboardButtons = std::span{ releasedKeyboardButtons };
    }

    void SDLInput::ProcessKeyboardEvent(const SDL_Event& event)
    {
        const auto button = sdl_scancode_to_button(event.key.keysym.scancode);
        if (event.key.state == SDL_PRESSED)
        {
            if (std::find(pressedKeyboardButtons.begin(), pressedKeyboardButtons.end(), button) == pressedKeyboardButtons.end())
            {
                pressedKeyboardButtons.push_back(button);
            }
        }
        else
        {
            releasedKeyboardButtons.push_back(button);
            pressedKeyboardButtons.erase(std::remove(pressedKeyboardButtons.begin(), pressedKeyboardButtons.end(), button), pressedKeyboardButtons.end());
        }
    }

    void SDLInput::ProcessMouseEvent(const SDL_Event& event)
    {
        MouseButtons button = MouseButtons::None;
        switch (event.button.button)
        {
            case SDL_BUTTON_LEFT: button = MouseButtons::Left; break;
            case SDL_BUTTON_MIDDLE: button = MouseButtons::Middle; break;
            case SDL_BUTTON_RIGHT: button = MouseButtons::Right; break;
        }

        if (event.button.state == SDL_PRESSED)
        {
            if (std::find(pressedMouseButtons.begin(), pressedMouseButtons.end(), button) == pressedMouseButtons.end())
            {
                pressedMouseButtons.push_back(button);
            }
        }
        else // SDL_RELEASED
        {
            releasedMouseButtons.push_back(button);
            pressedMouseButtons.erase(std::remove(pressedMouseButtons.begin(), pressedMouseButtons.end(), button), pressedMouseButtons.end());
        }
    }

}

namespace
{
    window::input::KeyboardButtons sdl_scancode_to_button(SDL_Scancode scancode) noexcept
    {
        switch (scancode)
        {
            case SDL_SCANCODE_A: return window::input::KeyboardButtons::A;
            case SDL_SCANCODE_B: return window::input::KeyboardButtons::B;
            case SDL_SCANCODE_C: return window::input::KeyboardButtons::C;
            case SDL_SCANCODE_D: return window::input::KeyboardButtons::D;
            case SDL_SCANCODE_E: return window::input::KeyboardButtons::E;
            case SDL_SCANCODE_F: return window::input::KeyboardButtons::F;
            case SDL_SCANCODE_G: return window::input::KeyboardButtons::G;
            case SDL_SCANCODE_H: return window::input::KeyboardButtons::H;
            case SDL_SCANCODE_I: return window::input::KeyboardButtons::I;
            case SDL_SCANCODE_J: return window::input::KeyboardButtons::J;
            case SDL_SCANCODE_K: return window::input::KeyboardButtons::K;
            case SDL_SCANCODE_L: return window::input::KeyboardButtons::L;
            case SDL_SCANCODE_M: return window::input::KeyboardButtons::M;
            case SDL_SCANCODE_N: return window::input::KeyboardButtons::N;
            case SDL_SCANCODE_O: return window::input::KeyboardButtons::O;
            case SDL_SCANCODE_P: return window::input::KeyboardButtons::P;
            case SDL_SCANCODE_Q: return window::input::KeyboardButtons::Q;
            case SDL_SCANCODE_R: return window::input::KeyboardButtons::R;
            case SDL_SCANCODE_S: return window::input::KeyboardButtons::S;
            case SDL_SCANCODE_T: return window::input::KeyboardButtons::T;
            case SDL_SCANCODE_U: return window::input::KeyboardButtons::U;
            case SDL_SCANCODE_V: return window::input::KeyboardButtons::V;
            case SDL_SCANCODE_W: return window::input::KeyboardButtons::W;
            case SDL_SCANCODE_X: return window::input::KeyboardButtons::X;
            case SDL_SCANCODE_Y: return window::input::KeyboardButtons::Y;
            case SDL_SCANCODE_Z: return window::input::KeyboardButtons::Z;
            case SDL_SCANCODE_1: return window::input::KeyboardButtons::_1;
            case SDL_SCANCODE_2: return window::input::KeyboardButtons::_2;
            case SDL_SCANCODE_3: return window::input::KeyboardButtons::_3;
            case SDL_SCANCODE_4: return window::input::KeyboardButtons::_4;
            case SDL_SCANCODE_5: return window::input::KeyboardButtons::_5;
            case SDL_SCANCODE_6: return window::input::KeyboardButtons::_6;
            case SDL_SCANCODE_7: return window::input::KeyboardButtons::_7;
            case SDL_SCANCODE_8: return window::input::KeyboardButtons::_8;
            case SDL_SCANCODE_9: return window::input::KeyboardButtons::_9;
            case SDL_SCANCODE_0: return window::input::KeyboardButtons::_0;
            case SDL_SCANCODE_ESCAPE: return window::input::KeyboardButtons::Escape;
            case SDL_SCANCODE_LCTRL:
            case SDL_SCANCODE_RCTRL: return window::input::KeyboardButtons::Control;
            case SDL_SCANCODE_LGUI: return window::input::KeyboardButtons::LeftSystem;
            case SDL_SCANCODE_RGUI: return window::input::KeyboardButtons::RightSystem;
            case SDL_SCANCODE_MENU: return window::input::KeyboardButtons::Menu;
            case SDL_SCANCODE_LEFTBRACKET: return window::input::KeyboardButtons::LeftBracket;
            case SDL_SCANCODE_RIGHTBRACKET: return window::input::KeyboardButtons::RightBracket;
            case SDL_SCANCODE_SEMICOLON: return window::input::KeyboardButtons::Semicolon;
            case SDL_SCANCODE_COMMA: return window::input::KeyboardButtons::Comma;
            case SDL_SCANCODE_PERIOD: return window::input::KeyboardButtons::Period;
            case SDL_SCANCODE_APOSTROPHE: return window::input::KeyboardButtons::Quote;
            case SDL_SCANCODE_SLASH: return window::input::KeyboardButtons::Slash;
            case SDL_SCANCODE_BACKSLASH: return window::input::KeyboardButtons::Backslash;
            case SDL_SCANCODE_GRAVE: return window::input::KeyboardButtons::Tilde;
            case SDL_SCANCODE_EQUALS: return window::input::KeyboardButtons::Equal;
            case SDL_SCANCODE_MINUS: return window::input::KeyboardButtons::Hyphen;
            case SDL_SCANCODE_SPACE: return window::input::KeyboardButtons::Space;
            case SDL_SCANCODE_RETURN: return window::input::KeyboardButtons::Enter;
            case SDL_SCANCODE_BACKSPACE: return window::input::KeyboardButtons::Backspace;
            case SDL_SCANCODE_TAB: return window::input::KeyboardButtons::Tab;
            case SDL_SCANCODE_PAGEUP: return window::input::KeyboardButtons::PageUp;
            case SDL_SCANCODE_PAGEDOWN: return window::input::KeyboardButtons::PageDown;
            case SDL_SCANCODE_END: return window::input::KeyboardButtons::End;
            case SDL_SCANCODE_HOME: return window::input::KeyboardButtons::Home;
            case SDL_SCANCODE_PRINTSCREEN: return window::input::KeyboardButtons::PrintScreen;
            case SDL_SCANCODE_INSERT: return window::input::KeyboardButtons::Insert;
            case SDL_SCANCODE_DELETE: return window::input::KeyboardButtons::Delete;
            case SDL_SCANCODE_KP_PLUS: return window::input::KeyboardButtons::Add;
            case SDL_SCANCODE_KP_MINUS: return window::input::KeyboardButtons::Subtract;
            case SDL_SCANCODE_KP_MULTIPLY: return window::input::KeyboardButtons::Multiply;
            case SDL_SCANCODE_KP_DIVIDE: return window::input::KeyboardButtons::Divide;
            case SDL_SCANCODE_RIGHT: return window::input::KeyboardButtons::Right;
            case SDL_SCANCODE_LEFT: return window::input::KeyboardButtons::Left;
            case SDL_SCANCODE_DOWN: return window::input::KeyboardButtons::Down;
            case SDL_SCANCODE_UP: return window::input::KeyboardButtons::Up;
            case SDL_SCANCODE_F1: return window::input::KeyboardButtons::F1;
            case SDL_SCANCODE_F2: return window::input::KeyboardButtons::F2;
            case SDL_SCANCODE_F3: return window::input::KeyboardButtons::F3;
            case SDL_SCANCODE_F4: return window::input::KeyboardButtons::F4;
            case SDL_SCANCODE_F5: return window::input::KeyboardButtons::F5;
            case SDL_SCANCODE_F6: return window::input::KeyboardButtons::F6;
            case SDL_SCANCODE_F7: return window::input::KeyboardButtons::F7;
            case SDL_SCANCODE_F8: return window::input::KeyboardButtons::F8;
            case SDL_SCANCODE_F9: return window::input::KeyboardButtons::F9;
            case SDL_SCANCODE_KP_1: return window::input::KeyboardButtons::NumPad_1;
            case SDL_SCANCODE_KP_2: return window::input::KeyboardButtons::NumPad_2;
            case SDL_SCANCODE_KP_3: return window::input::KeyboardButtons::NumPad_3;
            case SDL_SCANCODE_KP_4: return window::input::KeyboardButtons::NumPad_4;
            case SDL_SCANCODE_KP_5: return window::input::KeyboardButtons::NumPad_5;
            case SDL_SCANCODE_KP_6: return window::input::KeyboardButtons::NumPad_6;
            case SDL_SCANCODE_KP_7: return window::input::KeyboardButtons::NumPad_7;
            case SDL_SCANCODE_KP_8: return window::input::KeyboardButtons::NumPad_8;
            case SDL_SCANCODE_KP_9: return window::input::KeyboardButtons::NumPad_9;
            case SDL_SCANCODE_KP_0: return window::input::KeyboardButtons::NumPad_0;
            case SDL_SCANCODE_PAUSE: return window::input::KeyboardButtons::Pause;
            default: return window::input::KeyboardButtons::None;
        }
    }
}
