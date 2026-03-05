/**
 * \file
 * \author Rudy Castan
 * \date 2024 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

namespace demos
{
    class IDemo
    {
    public:
        constexpr IDemo() noexcept = default;
        virtual ~IDemo() noexcept  = default;

        constexpr IDemo(const IDemo&) noexcept  = default;
        constexpr IDemo(IDemo&&) noexcept       = default;
        IDemo& operator=(const IDemo&) noexcept = default;
        IDemo& operator=(IDemo&&) noexcept      = default;

        constexpr virtual void SetDisplaySize(int width, int height) = 0;
        constexpr virtual void Update()                              = 0;
        constexpr virtual void Draw() const                          = 0;
        constexpr virtual void ImGuiDraw()                           = 0;
    };
}
