/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "Settings.hpp"

#include "util/Logger.hpp"

#include <SDL.h>
#include <fstream>


namespace window
{
    void save_settings(const Settings& settings, const std::filesystem::path& filename)
    {
        std::ofstream file(filename, std::ios::binary);
        if (file)
        {
            file.write(reinterpret_cast<const char*>(&settings), sizeof(settings));
        }
        else
        {
            util::log_error("Error: Unable to open file \"{}\" for saving.", filename.string());
        }
    }

    Settings load_settings(const std::filesystem::path& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (file)
        {
            Settings data;
            if (file.read(reinterpret_cast<char*>(&data), sizeof(Settings)) && data.Version == Settings::LAYOUT_VERSION)
            {
                return data;
            }
        }
        Settings data{};
        data.Startup.PositionX = SDL_WINDOWPOS_UNDEFINED;
        data.Startup.PositionY = SDL_WINDOWPOS_UNDEFINED;
        return data;
    }
}
