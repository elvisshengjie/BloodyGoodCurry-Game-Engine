/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "util/WatchFiles.hpp"
#include <atomic>
#include <filesystem>
#include <forward_list>
#include <span>
#include <string_view>
#include <vector>

namespace opengl
{
    class Shader;
    class Texture;
}

namespace assets
{
    class Reloader
    {
    public:

        // Will create the shader and start watching the shader files to reload it if they change
        void SetAndAutoReloadShader(opengl::Shader& shader_to_reload, std::string_view shader_name, std::span<const std::filesystem::path> shader_file_paths);
        void SetAndAutoReloadShader(opengl::Shader& shader_to_reload, std::string_view shader_name, const std::initializer_list<std::filesystem::path>& shader_file_paths);

        // Will create the texture and start watching the shader files to reload it if they change
        void SetAndAutoReloadTexture(opengl::Texture& texture_to_reload, const std::filesystem::path& image_filepath, bool flip_vertical = true);

        void Update();


    private:

        struct ShaderState
        {
            std::vector<std::filesystem::path> AbsolutePaths;
            std::atomic_bool                   ShouldTryReloadAsset;
            opengl::Shader*                    ShaderPtr;

            ShaderState(std::vector<std::filesystem::path>&& paths, opengl::Shader* shader_ptr) : AbsolutePaths{ std::move(paths) }, ShouldTryReloadAsset{ false }, ShaderPtr(shader_ptr)
            {
            }
        };

        struct TextureState
        {
            std::filesystem::path AbsolutePath;
            std::atomic_bool      ShouldTryReloadAsset;
            opengl::Texture*      TexturePtr;

            TextureState(std::filesystem::path&& path, opengl::Texture* texture_ptr) : AbsolutePath{ std::move(path) }, ShouldTryReloadAsset{ false }, TexturePtr(texture_ptr)
            {
            }
        };

        std::forward_list<ShaderState>  shadersWatchingList;
        std::forward_list<TextureState> texturesWatchingList;
        util::WatchFiles                fileWatcher{};
    };
}
