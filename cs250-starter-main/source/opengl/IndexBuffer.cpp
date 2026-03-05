/**
 * \file
 * \author Rudy Castan
 * \author TODO Your Name
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

#include "IndexBuffer.hpp"

#include "GL.hpp"
#include <GL/glew.h>

namespace
{
    // TODO remove [[maybe_unused]]
    opengl::Handle create_indices_buffer([[maybe_unused]] const void* data, [[maybe_unused]] GLsizei size_bytes)
    {
        /*
        TODO
             GL::GenBuffers - https://docs.gl/es3/glGenBuffers
             GL::BindBuffer - bind newly created buffer https://docs.gl/es3/glBindBuffer
             GL::BufferData - https://docs.gl/es3/glBufferData
             GL::BindBuffer - unbind newly created buffer
        */
        return opengl::Handle{};
    }
}

namespace opengl
{

    IndexBuffer::IndexBuffer(std::span<const unsigned> indices) : element_type(IndexElement::UInt), count(static_cast<GLsizei>(indices.size()))
    {
        indices_handle = create_indices_buffer(indices.data(), static_cast<GLsizei>(indices.size_bytes()));
    }

    IndexBuffer::IndexBuffer(std::span<const unsigned short> indices) : element_type(IndexElement::UShort), count(static_cast<GLsizei>(indices.size()))
    {
        indices_handle = create_indices_buffer(indices.data(), static_cast<GLsizei>(indices.size_bytes()));
    }

    IndexBuffer::IndexBuffer(std::span<const unsigned char> indices) : element_type(IndexElement::UByte), count(static_cast<GLsizei>(indices.size()))
    {
        indices_handle = create_indices_buffer(indices.data(), static_cast<GLsizei>(indices.size_bytes()));
    }

    IndexBuffer::~IndexBuffer()
    {
        // TODO use GL::DeleteBuffers - https://docs.gl/es3/glDeleteBuffers
    }

    IndexBuffer::IndexBuffer(IndexBuffer&& temp) noexcept : element_type(temp.element_type), count(temp.count), indices_handle(temp.indices_handle)
    {
        temp.element_type   = IndexElement::None;
        temp.count          = 0;
        temp.indices_handle = 0;
    }

    IndexBuffer& IndexBuffer::operator=(IndexBuffer&& temp) noexcept
    {
        std::swap(element_type, temp.element_type);
        std::swap(count, temp.count);
        std::swap(indices_handle, temp.indices_handle);

        return *this;
    }

    // TODO remove [[maybe_unused]]
    void IndexBuffer::Use([[maybe_unused]] bool bind) const
    {
        // TODO use GL::BindBuffer - https://docs.gl/es3/glBindBuffer
    }
}
