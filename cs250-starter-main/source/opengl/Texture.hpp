/**
 * \file
 * \author Rudy Castan
 * \author TODO Your Name
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "Handle.hpp"

#include <GL/glew.h>
#include <array>
#include <filesystem>

#pragma once

namespace opengl
{
    class [[nodiscard]] Texture
    {
    public:
        Texture() = default;
        ~Texture() noexcept;

        Texture(const Texture& other) = delete;
        Texture(Texture&& other) noexcept;
        Texture& operator=(const Texture& other) = delete;
        Texture& operator=(Texture&& other) noexcept;

        [[nodiscard]] bool LoadFromFileImage(std::filesystem::path image_filepath, bool flip_vertical = true) noexcept;

        using RGBA = unsigned int;
        static_assert(sizeof(RGBA) == 4, "RGBA should be the same as 4 bytes, so we can easily treat it as an RGBA int.");

        [[nodiscard]] bool LoadFromMemory(int image_width, int image_height, const RGBA* colors) noexcept;

        void UseForSlot(unsigned int texture_unit) const noexcept;

        [[nodiscard]] Handle GetHandle() const noexcept
        {
            return texture_handle;
        }

        [[nodiscard]] int GetWidth() const noexcept
        {
            return width;
        }

        [[nodiscard]] int GetHeight() const noexcept
        {
            return height;
        }

        enum Filtering : GLint
        {
            NearestPixel = GL_NEAREST,
            Linear       = GL_LINEAR
        };

        void SetFiltering(Filtering how_to_filter) noexcept;

        [[nodiscard]] Filtering GetFiltering() const noexcept
        {
            return filtering;
        }

        enum Coordinate
        {
            S,
            T,
            Both
        };

        enum Wrapping : GLint
        {
            Repeat            = GL_REPEAT,
            ClampToEdge       = GL_CLAMP_TO_EDGE,
            ClampToBorder     = GL_CLAMP_TO_BORDER,
            MirroredRepeat    = GL_MIRRORED_REPEAT,
            MirrorClampToEdge = GL_MIRROR_CLAMP_TO_EDGE
        };

        void SetWrapping(Wrapping how_to_wrap, Coordinate coord = Coordinate::Both) noexcept;

        [[nodiscard]] std::array<Wrapping, 2> GetWrapping() const noexcept
        {
            return wrapping;
        }

    private:
        void delete_texture() noexcept;

    private:
        Handle                texture_handle = 0;
        int                     width          = 0;
        int                     height         = 0;
        Filtering               filtering      = Filtering::NearestPixel;
        std::array<Wrapping, 2> wrapping       = { Wrapping::Repeat, Wrapping::Repeat };
    };
}
