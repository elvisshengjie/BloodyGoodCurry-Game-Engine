/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "opengl/Environment.hpp"
#include "util/Logger.hpp"
#include "window/Application.hpp"
#include "window/Environment.hpp"
#include "window/Title.hpp"

#if defined(__EMSCRIPTEN__)
#    include <emscripten.h>
#    include <emscripten/bind.h>

namespace
{
    int  gWindowWidth  = window::WindowWidth;
    int  gWindowHeight = window::WindowHeight;
    bool gNeedResize   = false;
}

void main_loop(window::Application* application)
{
    if (gNeedResize)
    {
        application->ForceResize(gWindowWidth, gWindowHeight);
        gNeedResize = false;
    }
    application->Update();
    if (application->IsDone())
        emscripten_cancel_main_loop();
}

EMSCRIPTEN_BINDINGS(main_window)
{
    emscripten::function(
        "setWindowSize", emscripten::optional_override(
                             [](int sizeX, int sizeY)
                             {
                                 sizeX = (sizeX < 400) ? 400 : sizeX;
                                 sizeY = (sizeY < 400) ? 400 : sizeY;
                                 if (sizeX != window::WindowWidth || sizeY != window::WindowHeight)
                                 {
                                     gNeedResize   = true;
                                     gWindowWidth  = sizeX;
                                     gWindowHeight = sizeY;
                                 }
                             }));
}
#endif

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
try
{
    demos::Demos starting_demo = demos::Demos::None;
    if (argc > 1)
        starting_demo = demos::to_enum(argv[1]);
    window::Application application(starting_demo);
#if !defined(__EMSCRIPTEN__)
    while (!application.IsDone())
    {
        application.Update();
    }
#else
    // https://kripken.github.io/emscripten-site/docs/api_reference/emscripten.h.html#c.emscripten_set_main_loop_arg
    constexpr int simulate_infinite_loop  = 1;
    constexpr int match_browser_framerate = -1;
    emscripten_set_main_loop_arg(reinterpret_cast<void (*)(void*)>(&main_loop), reinterpret_cast<void*>(&application), match_browser_framerate, simulate_infinite_loop);
#endif

    return 0;
}
catch (const std::exception& e)
{
    util::log_error("{}", e.what());
    return -1;
}
