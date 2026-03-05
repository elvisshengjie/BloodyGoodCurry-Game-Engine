/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <algorithm>
#include <gsl/gsl>
#include <span>

namespace window::input
{
    enum class MouseButtons;
    enum class KeyboardButtons;

    inline float                      MouseWindowX   = 0;
    inline float                      MouseWindowY   = 0;
    inline float                      MouseVelocityX = 0;
    inline float                      MouseVelocityY = 0;
    inline float                      MouseWheel     = 0;
    inline float                      MouseDisplayX  = 0;
    inline float                      MouseDisplayY  = 0;
    inline std::span<MouseButtons>    PressedMouseButtons;
    inline std::span<MouseButtons>    ReleasedMouseButtons;
    inline std::span<KeyboardButtons> PressedKeyboardButtons;
    inline std::span<KeyboardButtons> ReleasedKeyboardButtons;

    template <typename T, typename... Values>
    [[nodiscard]] constexpr bool contains(const std::span<T>& container, Values... values) noexcept
    {
        auto contains_value = [&values...](const T& element)
        {
            return ((element == values) || ...);
        };
        const auto the_end = std::end(container);
        auto       result  = std::find_if(std::begin(container), the_end, contains_value);
        return result != the_end;
    }

    enum class MouseButtons
    {
        None,
        Left,
        Middle,
        Right,
        Count
    };

    enum class KeyboardButtons
    {
        None,
        _0,
        _1,
        _2,
        _3,
        _4,
        _5,
        _6,
        _7,
        _8,
        _9,
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        NumPad_0,
        NumPad_1,
        NumPad_2,
        NumPad_3,
        NumPad_4,
        NumPad_5,
        NumPad_6,
        NumPad_7,
        NumPad_8,
        NumPad_9,
        Escape,
        Control,
        Shift,
        CapsLock,
        LeftSystem,
        RightSystem,
        Menu,
        LeftBracket,
        RightBracket,
        Semicolon,
        Comma,
        Period,
        Quote,
        Slash,
        Backslash,
        Tilde,
        Equal,
        Hyphen,
        Space,
        Enter,
        Backspace,
        Tab,
        PageUp,
        PageDown,
        End,
        Home,
        PrintScreen,
        Insert,
        Delete,
        Add,
        Subtract,
        Multiply,
        Divide,
        Left,
        Right,
        Up,
        Down,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        Pause,
        Count
    };

    constexpr gsl::czstring to_string(MouseButtons button) noexcept
    {
        switch (button)
        {
            case MouseButtons::None: return "None";
            case MouseButtons::Left: return "Left";
            case MouseButtons::Middle: return "Middle";
            case MouseButtons::Right: return "Right";
            case MouseButtons::Count: return "Count";
            default: return "Unknown";
        }
    }

    constexpr gsl::czstring to_string(KeyboardButtons button) noexcept
    {
        switch (button)
        {
            case KeyboardButtons::None: return "None";
            case KeyboardButtons::_0: return "0";
            case KeyboardButtons::_1: return "1";
            case KeyboardButtons::_2: return "2";
            case KeyboardButtons::_3: return "3";
            case KeyboardButtons::_4: return "4";
            case KeyboardButtons::_5: return "5";
            case KeyboardButtons::_6: return "6";
            case KeyboardButtons::_7: return "7";
            case KeyboardButtons::_8: return "8";
            case KeyboardButtons::_9: return "9";
            case KeyboardButtons::A: return "A";
            case KeyboardButtons::B: return "B";
            case KeyboardButtons::C: return "C";
            case KeyboardButtons::D: return "D";
            case KeyboardButtons::E: return "E";
            case KeyboardButtons::F: return "F";
            case KeyboardButtons::G: return "G";
            case KeyboardButtons::H: return "H";
            case KeyboardButtons::I: return "I";
            case KeyboardButtons::J: return "J";
            case KeyboardButtons::K: return "K";
            case KeyboardButtons::L: return "L";
            case KeyboardButtons::M: return "M";
            case KeyboardButtons::N: return "N";
            case KeyboardButtons::O: return "O";
            case KeyboardButtons::P: return "P";
            case KeyboardButtons::Q: return "Q";
            case KeyboardButtons::R: return "R";
            case KeyboardButtons::S: return "S";
            case KeyboardButtons::T: return "T";
            case KeyboardButtons::U: return "U";
            case KeyboardButtons::V: return "V";
            case KeyboardButtons::W: return "W";
            case KeyboardButtons::X: return "X";
            case KeyboardButtons::Y: return "Y";
            case KeyboardButtons::Z: return "Z";
            case KeyboardButtons::NumPad_0: return "NumPad_0";
            case KeyboardButtons::NumPad_1: return "NumPad_1";
            case KeyboardButtons::NumPad_2: return "NumPad_2";
            case KeyboardButtons::NumPad_3: return "NumPad_3";
            case KeyboardButtons::NumPad_4: return "NumPad_4";
            case KeyboardButtons::NumPad_5: return "NumPad_5";
            case KeyboardButtons::NumPad_6: return "NumPad_6";
            case KeyboardButtons::NumPad_7: return "NumPad_7";
            case KeyboardButtons::NumPad_8: return "NumPad_8";
            case KeyboardButtons::NumPad_9: return "NumPad_9";
            case KeyboardButtons::Escape: return "Escape";
            case KeyboardButtons::Control: return "Control";
            case KeyboardButtons::Shift: return "Shift";
            case KeyboardButtons::CapsLock: return "CapsLock";
            case KeyboardButtons::LeftSystem: return "LeftSystem";
            case KeyboardButtons::RightSystem: return "RightSystem";
            case KeyboardButtons::Menu: return "Menu";
            case KeyboardButtons::LeftBracket: return "LeftBracket";
            case KeyboardButtons::RightBracket: return "RightBracket";
            case KeyboardButtons::Semicolon: return "Semicolon";
            case KeyboardButtons::Comma: return "Comma";
            case KeyboardButtons::Period: return "Period";
            case KeyboardButtons::Quote: return "Quote";
            case KeyboardButtons::Slash: return "Slash";
            case KeyboardButtons::Backslash: return "Backslash";
            case KeyboardButtons::Tilde: return "Tilde";
            case KeyboardButtons::Equal: return "Equal";
            case KeyboardButtons::Hyphen: return "Hyphen";
            case KeyboardButtons::Space: return "Space";
            case KeyboardButtons::Enter: return "Enter";
            case KeyboardButtons::Backspace: return "Backspace";
            case KeyboardButtons::Tab: return "Tab";
            case KeyboardButtons::PageUp: return "PageUp";
            case KeyboardButtons::PageDown: return "PageDown";
            case KeyboardButtons::End: return "End";
            case KeyboardButtons::Home: return "Home";
            case KeyboardButtons::PrintScreen: return "PrintScreen";
            case KeyboardButtons::Insert: return "Insert";
            case KeyboardButtons::Delete: return "Delete";
            case KeyboardButtons::Add: return "Add";
            case KeyboardButtons::Subtract: return "Subtract";
            case KeyboardButtons::Multiply: return "Multiply";
            case KeyboardButtons::Divide: return "Divide";
            case KeyboardButtons::Left: return "Left";
            case KeyboardButtons::Right: return "Right";
            case KeyboardButtons::Up: return "Up";
            case KeyboardButtons::Down: return "Down";
            case KeyboardButtons::F1: return "F1";
            case KeyboardButtons::F2: return "F2";
            case KeyboardButtons::F3: return "F3";
            case KeyboardButtons::F4: return "F4";
            case KeyboardButtons::F5: return "F5";
            case KeyboardButtons::F6: return "F6";
            case KeyboardButtons::F7: return "F7";
            case KeyboardButtons::F8: return "F8";
            case KeyboardButtons::F9: return "F9";
            case KeyboardButtons::Pause: return "Pause";
            case KeyboardButtons::Count: return "Count";
            default: return "Unknown";
        }
    }
}
