/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "Application.hpp"

#include "Environment.hpp"
#include "ImGuiHelper.hpp"
#include "Logo.hpp"
#include "Title.hpp"
#include "assets/Path.hpp"
#include "input/Environment.hpp"
#include "opengl/Environment.hpp"
#include "opengl/GL.hpp"
#include "util/Directory.hpp"
#include "util/Logger.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <imgui.h>
#include <sstream>

namespace
{
    void hint_gl(SDL_GLattr attr, int value)
    {
        // https://wiki.libsdl.org/SDL_GL_SetAttribute
        if (const auto success = SDL_GL_SetAttribute(attr, value); success != 0)
        {
            util::log_error("Failed to Set GL Attribute: {}", SDL_GetError());
        }
    }

    template <typename... Messages>
    [[noreturn]] void throw_error_message(Messages&&... more_messages)
    {
        std::ostringstream sout;
        (sout << ... << more_messages);
        util::log_error("{}", sout.str());
        throw std::runtime_error{ sout.str() };
    }

}

namespace window
{

    Application::Application(demos::Demos demo)
    {
        util::log_event("Application Start");
        setupSDLWindow(window::Title);
        setupOpenGL();
        util::log_debug("Writable App Directory : {}", util::get_writable_app_directory().string());
        setupWindowSizeAndDPI();
        setupImGui();

        if (demo != demos::Demos::None)
        {
            settings.CurrentDemo = demo;
        }
        ptrActiveDemo   = demos::create_demo(settings.CurrentDemo);
        currentViewport = { 0, 0, window::DisplayWidth, window::DisplayHeight };

        timer.ResetTimeStamp();
    }

    Application::~Application()
    {
        delete ptrActiveDemo;
        ImGuiHelper::Shutdown();
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(sdlWindow);
        SDL_Quit();
        util::log_event("Application End");
    }

    void Application::Update()
    {
        updateEnvironment();
        updateWindowEvents();
        updateDisplayViewport();
        ptrActiveDemo->Update();
        ptrActiveDemo->Draw();
        currentViewport = ImGuiHelper::Begin();
        imguiDraw();
        ImGuiHelper::End(sdlWindow, glContext);
        SDL_GL_SwapWindow(sdlWindow);
    }

    bool Application::IsDone() const noexcept
    {
        return is_done;
    }

    void Application::ForceResize(int desired_width, int desired_height) const
    {
        SDL_SetWindowSize(sdlWindow, desired_width, desired_height);
    }

    void Application::imguiDraw()
    {
        if (ImGui::BeginMainMenuBar())
        {
            imguiSelectDemo();
            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Demo Settings", "", &settings.ShowDemoSettings);
                ImGui::MenuItem("FPS", "", &settings.ShowFPS);
                ImGui::MenuItem("Window Information", "", &settings.ShowWindowInformation);
                ImGui::MenuItem("Mouse Information", "", &settings.ShowMouseInformation);
                ImGui::MenuItem("Keyboard Information", "", &settings.ShowKeyboardInformation);
                ImGui::MenuItem("OpenGL Information", "", &settings.ShowOpenGLInformation);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (settings.ShowFPS)
        {
            ImGui::Begin("FPS", &settings.ShowFPS);
            ImGui::Text("%d", window::FPS);
            ImGui::End();
        }

        if (settings.ShowWindowInformation)
        {
            ImGui::Begin("Window Information", &settings.ShowWindowInformation);
            ImGui::Text("Window Size (%d, %d)", window::WindowWidth, window::WindowHeight);
            ImGui::Text("Display Size (%d, %d)", window::DisplayWidth, window::DisplayHeight);
            ImGui::Text("DPI Scale (%g, %g)", double(window::HorizontalDPIScale), double(window::VerticalDPIScale));
            ImGui::Text("GL Viewport x: %d y: %d", currentViewport.x, currentViewport.y);
            ImGui::Text("GL Viewport w: %d h: %d", currentViewport.width, currentViewport.height);
            ImGui::End();
        }

        if (settings.ShowMouseInformation)
        {
            using namespace std::string_literals;
            ImGui::Begin("Mouse Information", &settings.ShowMouseInformation);
            ImGui::Text("Mouse Window Position (%g, %g)", double(window::input::MouseWindowX), double(window::input::MouseWindowY));
            ImGui::Text("Mouse Display Position (%g, %g)", double(window::input::MouseDisplayX), double(window::input::MouseDisplayY));
            ImGui::Text("Last Mouse Velocity (%g, %g)", double(input.LastMouseVelocityX()), double(input.LastMouseVelocityY()));
            ImGui::Text("Last Mouse Wheel %g", double(input.LastMouseWheel()));
            std::string collection = "{";
            for (const auto button : window::input::PressedMouseButtons)
            {
                collection += window::input::to_string(button) + ", "s;
            }
            collection += "}";
            ImGui::Text("Pressed %s", collection.c_str());
            collection = "{";
            for (const auto button : input.LastReleasedMouseButtons())
            {
                collection += window::input::to_string(button) + ", "s;
            }
            collection += "}";
            ImGui::Text("Last Released %s", collection.c_str());
            ImGui::End();
        }

        if (settings.ShowKeyboardInformation)
        {
            using namespace std::string_literals;
            ImGui::Begin("Keyboard Information", &settings.ShowKeyboardInformation);
            std::string collection = "{";
            for (const auto button : window::input::PressedKeyboardButtons)
            {
                collection += window::input::to_string(button) + ", "s;
            }
            collection += "}";
            ImGui::Text("Pressed %s", collection.c_str());
            collection = "{";
            for (const auto button : input.LastReleasedKeyboardButtons())
            {
                collection += window::input::to_string(button) + ", "s;
            }
            collection += "}";
            ImGui::Text("Last Released %s", collection.c_str());
            ImGui::End();
        }

        if (settings.ShowOpenGLInformation)
        {
            ImGui::Begin("OpenGL Information", &settings.ShowOpenGLInformation);
            ImGui::Text("Vendor %s", openglStrings.Vendor.c_str());
            ImGui::Text("Renderer %s", openglStrings.Renderer.c_str());
            ImGui::Text("Version %s", openglStrings.Version.c_str());
            ImGui::Text("Active Version OpenGL %d.%d", opengl::MajorVersion, opengl::MinorVersion);
            ImGui::Text("GLSLVersion %s", openglStrings.GLSLVersion.c_str());
            ImGui::Text("Is OpenGL ES %s", opengl::IsWebGL ? "true" : "false");
            ImGui::Text("MaxElementVertices %d", opengl::MaxElementVertices);
            ImGui::Text("MaxElementIndices %d", opengl::MaxElementIndices);
            ImGui::Text("MaxTextureImageUnits %d", opengl::MaxTextureImageUnits);
            ImGui::Text("MaxTextureSize %d", opengl::MaxTextureSize);
            ImGui::End();
        }

        if (settings.ShowDemoSettings)
        {
            ImGui::Begin("Demo Settings", &settings.ShowDemoSettings);
            ptrActiveDemo->ImGuiDraw();
            ImGui::End();
        }
    }

    void Application::setupSDLWindow(gsl::czstring title)
    {
        // https://wiki.libsdl.org/SDL_Init
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            throw_error_message("Failed to init SDK error: ", SDL_GetError());
        }

        settings = window::load_settings(util::get_writable_app_directory() / window::Settings::FileName);
        setStartingWindowPlacement();


#if defined(IS_WEBGL2)
        hint_gl(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        hint_gl(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        hint_gl(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
        hint_gl(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
        hint_gl(SDL_GL_DOUBLEBUFFER, true);
        hint_gl(SDL_GL_STENCIL_SIZE, 8);
        hint_gl(SDL_GL_DEPTH_SIZE, 24);
        hint_gl(SDL_GL_RED_SIZE, 8);
        hint_gl(SDL_GL_GREEN_SIZE, 8);
        hint_gl(SDL_GL_BLUE_SIZE, 8);
        hint_gl(SDL_GL_ALPHA_SIZE, 8);
        hint_gl(SDL_GL_MULTISAMPLEBUFFERS, 1);
        hint_gl(SDL_GL_MULTISAMPLESAMPLES, 4);


        // https://wiki.libsdl.org/SDL_CreateWindow
        sdlWindow = SDL_CreateWindow(
            title, settings.Startup.PositionX, settings.Startup.PositionY, settings.Startup.Width, settings.Startup.Height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        if (sdlWindow == nullptr)
        {
            throw_error_message("Failed to create window: ", SDL_GetError());
        }

        set_sdl_icon(sdlWindow);
    }

    void Application::setStartingWindowPlacement()
    {
        SDL_Rect  display_bounds;
        const int num_displays = SDL_GetNumVideoDisplays();
        for (int i = 0; i < num_displays; ++i)
        {
            SDL_GetDisplayBounds(i, &display_bounds);
            if (settings.Startup.PositionX >= display_bounds.x && settings.Startup.PositionX + settings.Startup.Width <= display_bounds.x + display_bounds.w &&
                settings.Startup.PositionY >= display_bounds.y && settings.Startup.PositionY + settings.Startup.Height <= display_bounds.y + display_bounds.h)
            {
                // saved settings are still valid
                return;
            }
        }

        // reset to main display
        SDL_GetDisplayBounds(0, &display_bounds);
        settings.Startup.PositionX = SDL_WINDOWPOS_UNDEFINED;
        settings.Startup.PositionY = SDL_WINDOWPOS_UNDEFINED;
        if (settings.Startup.Width > display_bounds.w || settings.Startup.Height > display_bounds.h)
        {
            settings.Startup.Width         = window::Settings::DEFAULT_WIDTH;
            settings.Startup.Height        = window::Settings::DEFAULT_HEIGHT;
            settings.Startup.ResetImGuiIni = true;
        }
    }

    void Application::setupOpenGL()
    {
        // https://wiki.libsdl.org/SDL_GL_CreateContext
        if (glContext = SDL_GL_CreateContext(sdlWindow); glContext == nullptr)
        {
            throw_error_message("Failed to create opengl context: ", SDL_GetError());
        }

        // https://wiki.libsdl.org/SDL_GL_MakeCurrent
        SDL_GL_MakeCurrent(sdlWindow, glContext);

        // http://glew.sourceforge.net/basic.html
        if (const auto result = glewInit(); GLEW_OK != result)
        {
            throw_error_message("Unable to initialize GLEW - error: ", glewGetErrorString(result));
        }

        // https://wiki.libsdl.org/SDL_GL_SetSwapInterval
        constexpr int ADAPTIVE_VSYNC = -1;
        constexpr int VSYNC          = 1;
        if (const auto result = SDL_GL_SetSwapInterval(ADAPTIVE_VSYNC); result != 0)
        {
            SDL_GL_SetSwapInterval(VSYNC);
        }

        getOpenGLSettings();
    }

    void Application::getOpenGLSettings()
    {
        openglStrings.Vendor      = reinterpret_cast<const char*>(GL::GetString(GL_VENDOR));
        openglStrings.Renderer    = reinterpret_cast<const char*>(GL::GetString(GL_RENDERER));
        openglStrings.Version     = reinterpret_cast<const char*>(GL::GetString(GL_VERSION));
        openglStrings.GLSLVersion = reinterpret_cast<const char*>(GL::GetString(GL_SHADING_LANGUAGE_VERSION));
        int major = 0, minor = 0;
        GL::GetIntegerv(GL_MAJOR_VERSION, &major);
        GL::GetIntegerv(GL_MINOR_VERSION, &minor);
        if (opengl::version(major, minor) < opengl::version(opengl::MinimumRequiredMajorVersion, opengl::MinimumRequiredMinorVersion))
            throw_error_message("Unsupported OpenGL version ", major, '.', minor, "\n We need OpenGL ", opengl::MinimumRequiredMajorVersion, '.', opengl::MinimumRequiredMinorVersion, " or higher");
        if (opengl::MajorVersion == 0)
        {
            opengl::MajorVersion = major;
            opengl::MinorVersion = minor;
        }
        GL::GetIntegerv(GL_MAX_ELEMENTS_VERTICES, &opengl::MaxElementVertices);
        GL::GetIntegerv(GL_MAX_ELEMENTS_INDICES, &opengl::MaxElementIndices);
        GL::GetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &opengl::MaxTextureImageUnits);
        GL::GetIntegerv(GL_MAX_TEXTURE_SIZE, &opengl::MaxTextureSize);
    }

    void Application::setupWindowSizeAndDPI() const
    {
        SDL_GetWindowSize(sdlWindow, &window::WindowWidth, &window::WindowHeight);
        SDL_GL_GetDrawableSize(sdlWindow, &window::DisplayWidth, &window::DisplayHeight);
        // handle DPI scaling
        if (window::WindowWidth != window::DisplayWidth || window::WindowHeight != window::DisplayHeight)
        {
            window::HorizontalDPIScale = static_cast<float>(window::DisplayWidth) / static_cast<float>(window::WindowWidth);
            window::VerticalDPIScale   = static_cast<float>(window::DisplayHeight) / static_cast<float>(window::WindowHeight);
        }
        else
        {
            window::HorizontalDPIScale = 1.0;
            window::VerticalDPIScale   = 1.0;
        }
    }

    void Application::setupImGui()
    {
        const auto imgui_ini_location = util::get_writable_app_directory() / "imgui.ini";
        if (settings.Startup.ResetImGuiIni || !std::filesystem::exists(imgui_ini_location))
        {
            std::filesystem::copy_file(assets::get_base_path() / "imgui_starter.ini", imgui_ini_location, std::filesystem::copy_options::overwrite_existing);
            settings.Startup.ResetImGuiIni = false;
        }
        ImGuiHelper::Initialize(sdlWindow, glContext, imgui_ini_location);
    }

    void Application::updateEnvironment()
    {
        window::DeltaTime = timer.GetElapsedSeconds();
        timer.ResetTimeStamp();
        window::ElapsedTime += window::DeltaTime;
        ++window::FrameCount;
        fps.Update(window::DeltaTime);
        window::FPS = fps;
    }

    void Application::updateWindowEvents()
    {
        input.Begin();
        SDL_Event event{ 0 };
        while (SDL_PollEvent(&event) != 0)
        {
            ImGuiHelper::FeedEvent(event);
            input.HandleEvent(event, sdlWindow, currentViewport.x, currentViewport.y);
            switch (event.type)
            {
                case SDL_WINDOWEVENT:
                    switch (event.window.event)
                    {
                        case SDL_WINDOWEVENT_CLOSE:
                            {
                                is_done = true;
                            }
                            break;
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            {
                                window::WindowWidth   = event.window.data1;
                                window::WindowHeight  = event.window.data2;
                                window::DisplayWidth  = static_cast<int>(static_cast<float>(window::WindowWidth) * window::HorizontalDPIScale);
                                window::DisplayHeight = static_cast<int>(static_cast<float>(window::WindowHeight) * window::VerticalDPIScale);
                            }
                            break;
                    }
                    break;
                case SDL_QUIT:
                    [[unlikely]] is_done = true;
                    SDL_GetWindowPosition(sdlWindow, &settings.Startup.PositionX, &settings.Startup.PositionY);
                    settings.Startup.Width  = window::WindowWidth;
                    settings.Startup.Height = window::WindowHeight;
                    window::save_settings(settings, util::get_writable_app_directory() / window::Settings::FileName);
                    break;
            }
        }

        input.End();
    }

    void Application::updateDisplayViewport()
    {
        if (currentViewport != lastViewport)
        {
            lastViewport = currentViewport;
            GL::Viewport(currentViewport.x, currentViewport.y, currentViewport.width, currentViewport.height);
            window::DisplayWidth  = currentViewport.width;
            window::DisplayHeight = currentViewport.height;
            ptrActiveDemo->SetDisplaySize(currentViewport.width, currentViewport.height);
        }
    }

    void Application::imguiSelectDemo()
    {
        using namespace demos;
        Demos selected_demo = settings.CurrentDemo;
        if (ImGui::BeginMenu("Select Demo"))
        {
            for (const auto& [name, demo] : demos::get_supported_demos())
            {
                if (ImGui::MenuItem(name.data(), "", settings.CurrentDemo == demo))
                {
                    selected_demo = demo;
                    util::log_event("Selected Demo {}", name);
                }
            }
            ImGui::EndMenu();
        }

        if (selected_demo != settings.CurrentDemo)
        {
            util::log_event("Shutting Down Demo {}", demos::to_string(settings.CurrentDemo));
            settings.CurrentDemo = selected_demo;
            delete ptrActiveDemo;
            util::log_event("Starting Up Demo {}", demos::to_string(selected_demo));
            ptrActiveDemo = create_demo(selected_demo);
        }
    }
}
