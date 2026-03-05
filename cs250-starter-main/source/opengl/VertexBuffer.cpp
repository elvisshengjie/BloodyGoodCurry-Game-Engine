/**
 * \file
 * \author Rudy Castan
 * \author TODO Your Name
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

#include "VertexBuffer.hpp"

#include "GL.hpp"
#include <GL/glew.h>

namespace opengl
{

    VertexBuffer::VertexBuffer(GLsizei size_in_bytes) : size(size_in_bytes)
    {
        // TODO remove [[maybe_unused]]
        [[maybe_unused]] constexpr const void* no_data = nullptr;
        /* TODO
                GL::GenBuffers - https://docs.gl/es3/glGenBuffers
                GL::BindBuffer - https://docs.gl/es3/glBindBuffer
                GL::BufferData - https://docs.gl/es3/glBufferData
                GL::BindBuffer - (unbind)
        */
    }

    VertexBuffer::~VertexBuffer()
    {
        // TODO GL::DeleteBuffers - https://docs.gl/es3/glDeleteBuffers
    }

    VertexBuffer::VertexBuffer(VertexBuffer&& temp) noexcept : size(temp.size), buffer_handle(temp.buffer_handle)
    {
        temp.buffer_handle = 0;
        temp.size          = 0;
    }

    VertexBuffer& VertexBuffer::operator=(VertexBuffer&& temp) noexcept
    {
        std::swap(buffer_handle, temp.buffer_handle);
        std::swap(size, temp.size);

        return *this;
    }

    // TODO remove [[maybe_unused]]
    void VertexBuffer::Use([[maybe_unused]] bool bind) const
    {
        // TODO GL::BindBuffer - https://docs.gl/es3/glBindBuffer
    }

    // TODO remove [[maybe_unused]]
    void VertexBuffer::send_buffer_data_to_gpu([[maybe_unused]] const void* data, [[maybe_unused]] GLsizei size_bytes, [[maybe_unused]] GLsizei starting_offset) const
    {
        /* TODO
                GL::BindBuffer - https://docs.gl/es3/glBindBuffer
                GL::BufferSubData - https://docs.gl/es3/glBufferSubData
                GL::BindBuffer - unbind
        */
    }
}
