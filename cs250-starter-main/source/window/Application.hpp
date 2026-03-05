/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

#pragma once

#include "ImGuiHelper.hpp"
#include "Settings.hpp"
#include "demos/DemosFactory.hpp"
#include "input/SDLInput.hpp"
#include "util/FPS.hpp"
#include "util/Timer.hpp"
#include <gsl/gsl>

struct SDL_Window;
typedef void* SDL_GLContext;

namespace window
{
    class [[nodiscard]] Application
    {
    public:
        explicit Application(demos::Demos starting_demo = demos::Demos::HelloQuad);
        ~Application();

        Application(const Application&)                = delete;
        Application& operator=(const Application&)     = delete;
        Application(Application&&) noexcept            = delete;
        Application& operator=(Application&&) noexcept = delete;

        void Update();
        bool IsDone() const noexcept;

        void ForceResize(int desired_width, int desired_height) const;

    private:
        void imguiDraw();
        void setupSDLWindow(gsl::czstring title);
        void setStartingWindowPlacement();
        void setupOpenGL();
        void getOpenGLSettings();
        void setupWindowSizeAndDPI() const;
        void setupImGui();
        void updateEnvironment();
        void updateWindowEvents();
        void updateDisplayViewport();
        void imguiSelectDemo();

    private:
        util::FPS                 fps{};
        util::Timer               timer{};
        gsl::owner<demos::IDemo*> ptrActiveDemo = nullptr;
        gsl::owner<SDL_Window*>   sdlWindow     = nullptr;
        gsl::owner<SDL_GLContext> glContext     = nullptr;
        bool                      is_done       = false;
        ImGuiHelper::Viewport     lastViewport{ 0, 0, 0, 0 };
        ImGuiHelper::Viewport     currentViewport;
        input::SDLInput           input;
        window::Settings          settings;

        struct
        {
            std::string Vendor;
            std::string Renderer;
            std::string Version;
            std::string GLSLVersion;
        } openglStrings;
    };
}
