/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <cstdint>
#include <filesystem>

namespace demos
{
    enum class Demos;
}

namespace window
{
    struct alignas(8) Settings
    {
        static constexpr int64_t LAYOUT_VERSION = 4;
        static constexpr auto    FileName       = "window_settings.dat";
        static constexpr int32_t DEFAULT_WIDTH  = 800;
        static constexpr int32_t DEFAULT_HEIGHT = 600;

        int64_t      Version = LAYOUT_VERSION;
        demos::Demos CurrentDemo{};
        bool         ShowDemoSettings        = true;
        bool         ShowFPS                 = true;
        bool         ShowWindowInformation   = false;
        bool         ShowMouseInformation    = false;
        bool         ShowKeyboardInformation = false;
        bool         ShowOpenGLInformation   = false;

        struct
        {
            int32_t PositionX = -1, PositionY = -1;
            int32_t Width         = DEFAULT_WIDTH;
            int32_t Height        = DEFAULT_HEIGHT;
            bool    ResetImGuiIni = true;
        } Startup;
    };

    void     save_settings(const Settings& settings, const std::filesystem::path& filename);
    Settings load_settings(const std::filesystem::path& filename);
}
