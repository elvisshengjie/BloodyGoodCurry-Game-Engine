/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <filesystem>
#include <gsl/pointers>

struct SDL_Window;
typedef void* SDL_GLContext;
union SDL_Event;

namespace window::ImGuiHelper
{
    void Initialize(gsl::not_null<SDL_Window*> sdl_window, gsl::not_null<SDL_GLContext> gl_context, const std::filesystem::path& ini_file_path = "imgui.ini");
    void FeedEvent(const SDL_Event& event);

    struct Viewport
    {
        int  x = 0, y = 0;
        int  width = 0, height = 0;
        bool operator==(const Viewport&) const = default;
    };

    Viewport Begin();
    void     End(gsl::not_null<SDL_Window*> sdl_window, gsl::not_null<SDL_GLContext> gl_context);
    void     Shutdown();

}
