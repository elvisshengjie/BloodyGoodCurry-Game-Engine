/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "Directory.hpp"

#include "window/Title.hpp"

#include <SDL_filesystem.h>
#include <gsl/gsl>

namespace
{
    std::string format_folder_name(const std::string& input)
    {
        // Remove punctuation, symbols, and special characters
        std::string formatted;
        for (const char c : input)
        {
            if (std::isalnum(c))
            {
                formatted += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            else if (c == ' ')
            {
                // Replace spaces with hyphens
                formatted += '-';
            }
        }

        // Remove consecutive hyphens
        formatted.erase(std::unique(formatted.begin(), formatted.end(), [](char a, char b) { return a == '-' && b == '-'; }), formatted.end());

        return formatted;
    }

    std::filesystem::path get_writable_directory()
    {
        std::filesystem::path writable_directory;
        if (const auto sdl_path = SDL_GetPrefPath("digipen.student.edu", format_folder_name(window::Title).c_str()))
        {
            writable_directory = sdl_path;
            SDL_free(sdl_path);
        }
        return writable_directory;
    }
}

namespace util
{
    std::filesystem::path get_writable_app_directory() noexcept
    {
        static auto location = ::get_writable_directory();
        return location;
    }
}
