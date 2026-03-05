/**
 * \file
 * \author Rudy Castan
 * \date 2026 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

#pragma once

#include "Handle.hpp"
#include "IndexBuffer.hpp"
#include "VertexBuffer.hpp"
#include <GL/glew.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace opengl
{

    namespace Attribute
    {
        /**
         * \brief Compact vertex attribute descriptor for efficient OpenGL vertex specification
         *
         * Type provides a space-efficient way to describe vertex attributes by packing
         * all the necessary OpenGL vertex attribute information into a single 32-bit value.
         *
         * Bit Layout:
         * - Bits 15-0:  OpenGL component type (GL_FLOAT, GL_UNSIGNED_BYTE, etc.)
         * - Bits 18-16: Component count (1-4 components per attribute)
         * - Bits 23-19: Attribute size in bytes (1-16 bytes)
         * - Bit 24:     Normalization flag for integer-to-float conversion
         * - Bit 25:     Integer attribute flag (glVertexAttribIPointer vs glVertexAttribPointer)
         * - Bits 31-26: Instancing divisor (0-63) for per-instance attributes
         */
        struct Type
        {
            uint16_t GLType         : 16; // Bits 15-0   (16 bits): OpenGL component type
            uint8_t  ComponentCount : 3;  // Bits 18-16   (3 bits): Component count (1, 2, 3, 4)
            uint8_t  SizeBytes      : 5;  // Bits 23-19   (5 bits): Attribute size in bytes
            bool     Normalize      : 1;  // Bit  24      (1 bit) : Normalized flag
            bool     IntAttribute   : 1;  // Bit  25      (1 bit) : Integer attribute flag
            uint8_t  Divisor        : 6;  // Bits 26-31   (6 bits): Divisor for instancing

            constexpr auto operator<=>(const Type&) const noexcept = default;

            /**
             * \brief Set the instancing divisor for per-instance attributes
             * \param divisor_value Divisor for instanced rendering (0 = per-vertex, >0 = per-instance)
             * \return Reference to this Type for method chaining
             */
            constexpr Type& WithDivisor(uint8_t divisor_value) noexcept
            {
                Divisor = divisor_value & 0x3F; // only 6 bits
                return *this;
            }
        };

        static_assert(sizeof(Type) == sizeof(uint32_t));
    }

    /**
     * \brief Layout specification for vertex attributes within a buffer
     *
     * BufferLayout describes how vertex attributes are organized within a single
     * buffer object, including their types, order, and optional starting offset.
     */
    struct BufferLayout
    {
        /** \brief Byte offset from buffer start where attribute data begins */
        uint32_t BufferStartingByteOffset = 0;

        /** \brief Ordered list of attribute types in this buffer layout */
        std::vector<Attribute::Type> Attributes{};

        constexpr BufferLayout() noexcept = default;

        constexpr BufferLayout(std::initializer_list<Attribute::Type> attributes) noexcept : BufferStartingByteOffset{ 0 }, Attributes{ attributes }
        {
        }

        constexpr BufferLayout(uint32_t starting_byte_offset, std::initializer_list<Attribute::Type> attributes) noexcept : BufferStartingByteOffset{ starting_byte_offset }, Attributes{ attributes }
        {
        }
    };

    struct Primitive
    {
        enum Type : GLenum
        {
            Points        = GL_POINTS,
            Lines         = GL_LINES,
            LineLoop      = GL_LINE_LOOP,
            LineStrip     = GL_LINE_STRIP,
            Triangles     = GL_TRIANGLES,
            TriangleStrip = GL_TRIANGLE_STRIP,
            TriangleFan   = GL_TRIANGLE_FAN,
            Patches       = GL_PATCHES,
        };
    };

    class [[nodiscard]] VertexArray
    {
        Handle                    vertex_array_handle = 0;
        std::vector<VertexBuffer> vertex_buffers;
        IndexBuffer               index_buffer;
        GLsizei                   num_indices       = 0;
        IndexElement::Type        indices_type      = IndexElement::None;
        Primitive::Type           primitive_pattern = Primitive::Triangles;
        GLsizei                   num_vertices      = 0;

    public:
        explicit VertexArray(Primitive::Type the_primitive_pattern = Primitive::Triangles);
        ~VertexArray();

        VertexArray(const VertexArray&)            = delete;
        VertexArray& operator=(const VertexArray&) = delete;
        VertexArray(VertexArray&& temp) noexcept;
        VertexArray& operator=(VertexArray&& temp) noexcept;

        void Use(bool bind = true) const;

        void AddVertexBuffer(VertexBuffer&& vertex_buffer, BufferLayout buffer_layout);
        void SetIndexBuffer(IndexBuffer&& the_indices);

        [[nodiscard]] Handle GetHandle() const noexcept
        {
            return vertex_array_handle;
        }

        [[nodiscard]] const auto& GetVertexBuffers() const
        {
            return vertex_buffers;
        }

        [[nodiscard]] const auto& GetIndexBuffer() const
        {
            return index_buffer;
        }

        [[nodiscard]] GLsizei GetIndicesCount() const noexcept
        {
            return num_indices;
        }

        [[nodiscard]] IndexElement::Type GetIndicesType() const noexcept
        {
            return indices_type;
        }

        [[nodiscard]] Primitive::Type GetPrimitivePattern() const
        {
            return primitive_pattern;
        }

        void SetPrimitivePattern(Primitive::Type the_primitive_pattern)
        {
            primitive_pattern = the_primitive_pattern;
        }

        [[nodiscard]] GLsizei GetVertexCount() const
        {
            return num_vertices;
        }

        void SetVertexCount(int vertex_count)
        {
            num_vertices = vertex_count;
        }
    };

    void DrawIndexed(const VertexArray& vertex_array) noexcept;
    void DrawVertices(const VertexArray& vertex_array) noexcept;

    namespace Attribute
    {
        namespace details
        {
            // Constants for encoding
            constexpr bool NORMALIZE    = true;
            constexpr bool NO_NORMALIZE = false;
            constexpr bool TO_INT       = true;  // Use glVertexAttribIPointer
            constexpr bool TO_FLOAT     = false; // Use glVertexAttribPointer
        }

        /**
         * \brief Predefined vertex attribute types for common data formats
         *
         * Naming Convention:
         * - Base types: Bool, Byte, Short, Int, UByte, UShort, UInt, Float
         * - Vector types: Type2, Type3, Type4 (e.g., Float2, Int3, UByte4)
         * - Conversions: TypeToFloat, TypeToNormalized (e.g., ByteToFloat, UByteToNormalized)
         *
         * Common Usage Patterns:
         * - Positions: Float2, Float3
         * - Colors: UByte4ToNormalized (compact), Float4 (precise)
         * - Normals: Float3, Byte3ToNormalized (compact)
         * - Texture Coordinates: Float2
         */

        constexpr Type None                = { 0, 0, 0, 0, 0, 0 };
        constexpr Type Bool                = { GL_UNSIGNED_BYTE, 1, 1 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Bool2               = { GL_UNSIGNED_BYTE, 2, 2 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Bool3               = { GL_UNSIGNED_BYTE, 3, 3 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Bool4               = { GL_UNSIGNED_BYTE, 4, 4 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Byte                = { GL_BYTE, 1, 1 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Byte2               = { GL_BYTE, 2, 2 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Byte2ToFloat        = { GL_BYTE, 2, 2 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Byte2ToNormalized   = { GL_BYTE, 2, 2 * sizeof(int8_t), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Byte3               = { GL_BYTE, 3, 3 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Byte3ToFloat        = { GL_BYTE, 3, 3 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Byte3ToNormalized   = { GL_BYTE, 3, 3 * sizeof(int8_t), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Byte4               = { GL_BYTE, 4, 4 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Byte4ToFloat        = { GL_BYTE, 4, 4 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Byte4ToNormalized   = { GL_BYTE, 4, 4 * sizeof(int8_t), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type ByteToFloat         = { GL_BYTE, 1, 1 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type ByteToNormalized    = { GL_BYTE, 1, 1 * sizeof(int8_t), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Float               = { GL_FLOAT, 1, 1 * sizeof(float), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Float2              = { GL_FLOAT, 2, 2 * sizeof(float), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Float3              = { GL_FLOAT, 3, 3 * sizeof(float), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Float4              = { GL_FLOAT, 4, 4 * sizeof(float), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Int                 = { GL_INT, 1, 1 * sizeof(int), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Int2                = { GL_INT, 2, 2 * sizeof(int), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Int2ToFloat         = { GL_INT, 2, 2 * sizeof(int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Int2ToNormalized    = { GL_INT, 2, 2 * sizeof(int), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Int3                = { GL_INT, 3, 3 * sizeof(int), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Int3ToFloat         = { GL_INT, 3, 3 * sizeof(int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Int3ToNormalized    = { GL_INT, 3, 3 * sizeof(int), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Int4                = { GL_INT, 4, 4 * sizeof(int), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Int4ToFloat         = { GL_INT, 4, 4 * sizeof(int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Int4ToNormalized    = { GL_INT, 4, 4 * sizeof(int), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type IntToFloat          = { GL_INT, 1, 1 * sizeof(int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type IntToNormalized     = { GL_INT, 1, 1 * sizeof(int), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Short               = { GL_SHORT, 1, 1 * sizeof(short), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Short2              = { GL_SHORT, 2, 2 * sizeof(short), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Short2ToFloat       = { GL_SHORT, 2, 2 * sizeof(short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Short2ToNormalized  = { GL_SHORT, 2, 2 * sizeof(short), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Short3              = { GL_SHORT, 3, 3 * sizeof(short), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Short3ToFloat       = { GL_SHORT, 3, 3 * sizeof(short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Short3ToNormalized  = { GL_SHORT, 3, 3 * sizeof(short), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Short4              = { GL_SHORT, 4, 4 * sizeof(short), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type Short4ToFloat       = { GL_SHORT, 4, 4 * sizeof(short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type Short4ToNormalized  = { GL_SHORT, 4, 4 * sizeof(short), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type ShortToFloat        = { GL_SHORT, 1, 1 * sizeof(short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type ShortToNormalized   = { GL_SHORT, 1, 1 * sizeof(short), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UByte               = { GL_UNSIGNED_BYTE, 1, 1 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type UByte2              = { GL_UNSIGNED_BYTE, 2, 2 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type UByte2ToFloat       = { GL_UNSIGNED_BYTE, 2, 2 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UByte2ToNormalized  = { GL_UNSIGNED_BYTE, 2, 2 * sizeof(uint8_t), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UByte3              = { GL_UNSIGNED_BYTE, 3, 3 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type UByte3ToFloat       = { GL_UNSIGNED_BYTE, 3, 3 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UByte3ToNormalized  = { GL_UNSIGNED_BYTE, 3, 3 * sizeof(uint8_t), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UByte4              = { GL_UNSIGNED_BYTE, 4, 4 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type UByte4ToFloat       = { GL_UNSIGNED_BYTE, 4, 4 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UByte4ToNormalized  = { GL_UNSIGNED_BYTE, 4, 4 * sizeof(uint8_t), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UByteToFloat        = { GL_UNSIGNED_BYTE, 1, 1 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UByteToNormalized   = { GL_UNSIGNED_BYTE, 1, 1 * sizeof(uint8_t), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UInt                = { GL_UNSIGNED_INT, 1, 1 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type UInt2               = { GL_UNSIGNED_INT, 2, 2 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type UInt2ToFloat        = { GL_UNSIGNED_INT, 2, 2 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UInt2ToNormalized   = { GL_UNSIGNED_INT, 2, 2 * sizeof(unsigned int), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UInt3               = { GL_UNSIGNED_INT, 3, 3 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type UInt3ToFloat        = { GL_UNSIGNED_INT, 3, 3 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UInt3ToNormalized   = { GL_UNSIGNED_INT, 3, 3 * sizeof(unsigned int), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UInt4               = { GL_UNSIGNED_INT, 4, 4 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type UInt4ToFloat        = { GL_UNSIGNED_INT, 4, 4 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UInt4ToNormalized   = { GL_UNSIGNED_INT, 4, 4 * sizeof(unsigned int), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UIntToFloat         = { GL_UNSIGNED_INT, 1, 1 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UIntToNormalized    = { GL_UNSIGNED_INT, 1, 1 * sizeof(unsigned int), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UShort              = { GL_UNSIGNED_SHORT, 1, 1 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type UShort2             = { GL_UNSIGNED_SHORT, 2, 2 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type UShort2ToFloat      = { GL_UNSIGNED_SHORT, 2, 2 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UShort2ToNormalized = { GL_UNSIGNED_SHORT, 2, 2 * sizeof(unsigned short), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UShort3             = { GL_UNSIGNED_SHORT, 3, 3 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type UShort3ToFloat      = { GL_UNSIGNED_SHORT, 3, 3 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UShort3ToNormalized = { GL_UNSIGNED_SHORT, 3, 3 * sizeof(unsigned short), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UShort4             = { GL_UNSIGNED_SHORT, 4, 4 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_INT, 0 };
        constexpr Type UShort4ToFloat      = { GL_UNSIGNED_SHORT, 4, 4 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UShort4ToNormalized = { GL_UNSIGNED_SHORT, 4, 4 * sizeof(unsigned short), details::NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UShortToFloat       = { GL_UNSIGNED_SHORT, 1, 1 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };
        constexpr Type UShortToNormalized  = { GL_UNSIGNED_SHORT, 1, 1 * sizeof(unsigned short), details::NORMALIZE, details::TO_FLOAT, 0 };
    }

}
