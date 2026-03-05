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

    struct IndexElement
    {
        enum Type : unsigned
        {
            UInt   = GL_UNSIGNED_INT,
            UShort = GL_UNSIGNED_SHORT,
            UByte  = GL_UNSIGNED_BYTE,
            None   = 0
        };
    };

    class [[nodiscard]] IndexBuffer
    {
        IndexElement::Type element_type   = IndexElement::None;
        GLsizei            count          = 0;
        Handle           indices_handle = 0;

    public:
        IndexBuffer() = default;
        IndexBuffer(std::span<const unsigned int> indices);
        IndexBuffer(std::span<const unsigned short> indices);
        IndexBuffer(std::span<const unsigned char> indices);
        ~IndexBuffer();

        IndexBuffer(const IndexBuffer&)            = delete;
        IndexBuffer& operator=(const IndexBuffer&) = delete;
        IndexBuffer(IndexBuffer&& temp) noexcept;
        IndexBuffer& operator=(IndexBuffer&& temp) noexcept;

        void Use(bool bind = true) const;

        [[nodiscard]] Handle GetHandle() const noexcept
        {
            return indices_handle;
        }

        [[nodiscard]] GLsizei GetCount() const noexcept
        {
            return count;
        }

        [[nodiscard]] IndexElement::Type GetElementType() const noexcept
        {
            return element_type;
        }
    };

}
