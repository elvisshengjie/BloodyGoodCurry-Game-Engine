/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

namespace util
{
    class FPS
    {
        float timeCounter = 0;
        float numFrames   = 0;
        float fps         = 0;

    public:
        constexpr void Update(float delta_time_seconds) noexcept
        {
            ++numFrames;
            timeCounter += delta_time_seconds;
            if (timeCounter > 1.0f)
            {
                fps = numFrames / timeCounter;
                timeCounter -= 1.0f;
                numFrames = 0;
            }
        }

        constexpr operator long long() const noexcept
        {
            return static_cast<long long>(fps);
        }

        constexpr operator int() const noexcept
        {
            return static_cast<int>(fps);
        }
    };
}
