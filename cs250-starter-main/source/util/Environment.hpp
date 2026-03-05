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
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
#    define CAN_USE_THREADS
    constexpr bool CanUseThreads = true;
#else
    constexpr bool CanUseThreads = false;
#endif
}
