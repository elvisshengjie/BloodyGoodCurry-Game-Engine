/**
 * \file
 * \author Rudy Castan
 * \date 2024 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

namespace util
{
    float random(float min_inclusive = 0.0f, float max_exclusive = 1.0f) noexcept;
    int   random(int min_inclusive, int max_exclusive) noexcept;
    int   random(int max_exclusive) noexcept;
}
