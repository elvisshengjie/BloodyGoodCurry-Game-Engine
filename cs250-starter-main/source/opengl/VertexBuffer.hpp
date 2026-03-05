/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

#pragma once

#include "Handle.hpp"
#include <GL/glew.h>
#include <span>

namespace opengl
{

    class [[nodiscard]] VertexBuffer
    {
        GLsizei size          = 0;
        Handle  buffer_handle = 0;

    public:
        explicit VertexBuffer(GLsizei size_in_bytes);

        template <typename T, auto N = std::dynamic_extent>
        VertexBuffer(std::span<T, N> buffer_data) : VertexBuffer(static_cast<GLsizei>(buffer_data.size_bytes()))
        {
            send_buffer_data_to_gpu(buffer_data.data(), size, 0);
        }

        ~VertexBuffer();

        VertexBuffer(const VertexBuffer&)            = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;
        VertexBuffer(VertexBuffer&& temp) noexcept;
        VertexBuffer& operator=(VertexBuffer&& temp) noexcept;

        void Use(bool bind = true) const;

        template <typename T, auto N = std::dynamic_extent>
        void SetData(std::span<T, N> buffer_data, GLsizei starting_offset = 0) const
        {
            send_buffer_data_to_gpu(buffer_data.data(), static_cast<GLsizei>(buffer_data.size_bytes()), starting_offset);
        }

        [[nodiscard]] Handle GetHandle() const noexcept
        {
            return buffer_handle;
        }

        [[nodiscard]] GLsizei GetSizeBytes() const noexcept
        {
            return size;
        }

    private:
        void send_buffer_data_to_gpu(const void* data, GLsizei size_bytes, GLsizei starting_offset) const;
    };
}
