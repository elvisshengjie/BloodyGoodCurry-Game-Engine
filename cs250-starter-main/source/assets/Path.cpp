/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "Path.hpp"

#include <SDL.h>
#include <optional>

namespace
{
    std::optional<std::filesystem::path> try_get_asset_path(const std::filesystem::path& starting_directory)
    {
        namespace fs        = std::filesystem;
        fs::path       p    = fs::absolute(starting_directory);
        const fs::path root = p.root_path();

        // note that the do while loop is needed because on the web platform the starting folder is the root and that's also where the assets folder should be
        do
        {
            const fs::path assets_folder = p / "assets";
            if (fs::is_directory(assets_folder))
            {
                return assets_folder;
            }
            // if assets not found then try looking the in parent folder
            p = p.parent_path();
        } while (p != root);

        return std::nullopt;
    }
}

namespace assets
{

    std::filesystem::path get_base_path()
    {
        namespace fs                  = std::filesystem;
        static fs::path assets_folder = []()
        {
            auto result = try_get_asset_path(fs::current_path());
            if (result)
                return result.value();
            // try from the exe path rather than the current working directory
            const auto base_path = SDL_GetBasePath();
            result               = try_get_asset_path(base_path);
            SDL_free(base_path);
            if (result)
                return result.value();
            throw std::runtime_error{ "Failed to find assets folder in parent folders" };
        }();
        return assets_folder;
    }
}
