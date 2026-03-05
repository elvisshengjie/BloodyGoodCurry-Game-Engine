/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "Reloader.hpp"

#include "Path.hpp"
#include "opengl/Shader.hpp"
#include "opengl/Texture.hpp"
#include "util/Logger.hpp"

namespace
{
    std::filesystem::path              to_absolute_path(const std::filesystem::path& path_to_check);
    std::vector<std::filesystem::path> to_absolute_paths(std::span<const std::filesystem::path> paths_to_check);
}

namespace assets
{
    void Reloader::SetAndAutoReloadShader(opengl::Shader& shader_to_reload, std::string_view shader_name, std::span<const std::filesystem::path> shader_file_paths)
    {
        shader_to_reload                  = opengl::Shader(shader_name, shader_file_paths);
        auto&      info                   = shadersWatchingList.emplace_front(to_absolute_paths(shader_file_paths), &shader_to_reload);
        const auto reload_shader_callback = [&](util::FileStatus status)
        {
            info.ShouldTryReloadAsset = (status == util::FileStatus::Created || status == util::FileStatus::Modified);
        };
        for (const auto& path : info.AbsolutePaths)
        {
            fileWatcher.Watch(path, reload_shader_callback);
        }
    }

    void Reloader::SetAndAutoReloadShader(opengl::Shader& shader_to_reload, std::string_view shader_name, const std::initializer_list<std::filesystem::path>& shader_file_paths)
    {
        SetAndAutoReloadShader(shader_to_reload, shader_name, std::span{ std::begin(shader_file_paths), std::end(shader_file_paths) });
    }

    void Reloader::SetAndAutoReloadTexture(opengl::Texture& texture_to_reload, const std::filesystem::path& image_filepath, bool flip_vertical)
    {
        std::filesystem::path absolute_path = to_absolute_path(image_filepath);
        if (const bool loaded = texture_to_reload.LoadFromFileImage(absolute_path, flip_vertical); !loaded)
        {
            util::log_error("Failed to load texture\nCould not open {}", absolute_path.string());
            return;
        }
        auto&      info                    = texturesWatchingList.emplace_front(std::move(absolute_path), &texture_to_reload);
        const auto reload_texture_callback = [&](util::FileStatus status)
        {
            info.ShouldTryReloadAsset = (status == util::FileStatus::Created || status == util::FileStatus::Modified);
        };
        fileWatcher.Watch(info.AbsolutePath, reload_texture_callback);
    }

    void Reloader::Update()
    {
        fileWatcher.Update();
        for (auto& shader_info : shadersWatchingList)
        {
            if (!shader_info.ShouldTryReloadAsset)
                continue;

            shader_info.ShouldTryReloadAsset = false;
            auto& shader                     = *shader_info.ShaderPtr;
            try
            {
                auto new_version = opengl::Shader(shader.GetName(), std::span{ shader_info.AbsolutePaths });
                shader           = std::move(new_version);
            }
            catch (const std::exception& e)
            {
                util::log_debug("Failed to reload shader\n{}", e.what());
            }
        }
        for (auto& texture_info : texturesWatchingList)
        {
            if (!texture_info.ShouldTryReloadAsset)
                continue;

            texture_info.ShouldTryReloadAsset = false;
            opengl::Texture new_version;
            if (const bool loaded = new_version.LoadFromFileImage(texture_info.AbsolutePath); !loaded)
            {
                util::log_debug("Failed to reload texture\nCould not open {}", texture_info.AbsolutePath.string());
            }
            else
            {
                new_version.SetFiltering(texture_info.TexturePtr->GetFiltering());
                const auto wrapping = texture_info.TexturePtr->GetWrapping();
                new_version.SetWrapping(wrapping[0], opengl::Texture::S);
                new_version.SetWrapping(wrapping[1], opengl::Texture::T);
                *texture_info.TexturePtr = std::move(new_version);
            }
        }
    }
}

namespace
{
    std::filesystem::path to_absolute_path(const std::filesystem::path& path_to_check)
    {
        auto file_path = path_to_check;
        if (!std::filesystem::exists(file_path))
        {
            // try prepending the asset directory path
            file_path = assets::get_base_path() / file_path;
            if (!std::filesystem::exists(file_path))
            {
                throw std::runtime_error{ "Cannot find " + path_to_check.string() + "\n" };
            }
        }
        return file_path;
    }

    std::vector<std::filesystem::path> to_absolute_paths(std::span<const std::filesystem::path> paths_to_check)
    {
        std::vector<std::filesystem::path> absolute_paths;
        absolute_paths.reserve(paths_to_check.size());
        for (const auto& path_to_check : paths_to_check)
        {
            absolute_paths.push_back(to_absolute_path(path_to_check));
        }
        return absolute_paths;
    }
}
