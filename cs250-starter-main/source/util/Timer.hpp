/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <chrono>

namespace util
{
    class [[nodiscard]] Timer
    {
    private:
        using clock_t  = std::chrono::steady_clock;
        using second_t = std::chrono::duration<float, std::ratio<1>>;

        std::chrono::time_point<clock_t> timeStamp;

    public:
        Timer() noexcept : timeStamp(clock_t::now())
        {
        }

        void ResetTimeStamp() noexcept
        {
            timeStamp = clock_t::now();
        }

        float GetElapsedSeconds() const noexcept
        {
            return std::chrono::duration_cast<second_t>(clock_t::now() - timeStamp).count();
        }
    };
}
