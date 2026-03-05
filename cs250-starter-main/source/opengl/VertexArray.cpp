/**
 * \file
 * \author Rudy Castan
 * \author TODO Your Name
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

#include "VertexArray.hpp"

#include "GL.hpp"
#include <GL/glew.h>
#include <gsl/gsl>

namespace opengl
{

    VertexArray::VertexArray(Primitive::Type the_primitive_pattern)
    {
        primitive_pattern = the_primitive_pattern;
        /* TODO
            GL::GenVertexArrays - https://docs.gl/es3/glGenVertexArrays
        */
    }

    VertexArray::~VertexArray()
    {
        // TODO GL::DeleteVertexArrays - https://docs.gl/es3/glDeleteVertexArrays
    }

    VertexArray::VertexArray(VertexArray&& temp) noexcept
        : vertex_array_handle(temp.vertex_array_handle), vertex_buffers(std::move(temp.vertex_buffers)), index_buffer(std::move(temp.index_buffer)), num_indices(temp.num_indices),
          indices_type(temp.indices_type), primitive_pattern(temp.primitive_pattern), num_vertices(temp.num_vertices)
    {
        temp.vertex_array_handle = 0;
        temp.num_indices         = 0;
        temp.indices_type        = IndexElement::None;
        temp.num_vertices        = 0;
    }

    VertexArray& VertexArray::operator=(VertexArray&& temp) noexcept
    {
        std::swap(vertex_array_handle, temp.vertex_array_handle);
        std::swap(vertex_buffers, temp.vertex_buffers);
        std::swap(index_buffer, temp.index_buffer);
        std::swap(num_indices, temp.num_indices);
        std::swap(indices_type, temp.indices_type);
        std::swap(primitive_pattern, temp.primitive_pattern);
        std::swap(num_vertices, temp.num_vertices);

        return *this;
    }

    // TODO remove [[maybe_unused]]
    void VertexArray::Use([[maybe_unused]] bool bind) const
    {
        // TODO GL::BindVertexArray - https://docs.gl/es3/glBindVertexArray
    }

    // TODO remove [[maybe_unused]]
    void VertexArray::AddVertexBuffer(VertexBuffer&& vertex_buffer, [[maybe_unused]] BufferLayout buffer_layout)
    {
        /* TODO
            1. Use (bind) this Vertex Array
            2. Use (bind) the Vertex Buffer

            3. Calculate stride: sum up SizeBytes for all attributes in buffer_layout.Attributes

            4. Initialize offset to buffer_layout.BufferStartingByteOffset

            5. Initialize attribute_index to 0

            6. For each attribute in buffer_layout.Attributes:
               - Skip if attribute is Attribute::None
               - GL::EnableVertexAttribArray - https://docs.gl/es3/glEnableVertexAttribArray
               - Extract: GLType, ComponentCount, Normalize, IntAttribute, Divisor from attribute
               - If IntAttribute is true:
                   GL::VertexAttribIPointer - https://docs.gl/es3/glVertexAttribPointer
               - Else:
                   GL::VertexAttribPointer - https://docs.gl/es3/glVertexAttribPointer
               - GL::VertexAttribDivisor - https://docs.gl/es3/glVertexAttribDivisor
               - Increment attribute_index
               - Add attribute.SizeBytes to offset

            7. Use(false) to unbind this Vertex Array
            8. vertex_buffer.Use(false) to unbind the Vertex Buffer
        */
        vertex_buffers.emplace_back(std::move(vertex_buffer));
    }

    void VertexArray::SetIndexBuffer(IndexBuffer&& the_indices)
    {
        num_indices  = the_indices.GetCount();
        indices_type = the_indices.GetElementType();
        /* TODO
                Use (bind) this Vertex Array
                Use (bind) the index buffer
                Do not Use (unbind) this vertex array
                Do not Use (unbind) the index buffer
        */
        index_buffer = std::move(the_indices);
    }

    // TODO remove [[maybe_unused]]
    void DrawIndexed([[maybe_unused]] const VertexArray& vertex_array) noexcept
    {
        // TODO GL::DrawElements - https://docs.gl/es3/glDrawElements
    }

    // TODO remove [[maybe_unused]]
    void DrawVertices([[maybe_unused]] const VertexArray& vertex_array) noexcept
    {
        // TODO GL::DrawArrays - https://docs.gl/es3/glDrawArrays
    }
}
