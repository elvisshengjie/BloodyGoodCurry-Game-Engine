/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

 #pragma once

 #pragma once
 
 #include "Handle.hpp"
 #include <GL/glew.h>
 #include <filesystem>
 #include <glm/mat2x2.hpp> // mat2, dmat2
 #include <glm/mat3x3.hpp> // mat3, dmat3
 #include <glm/mat4x4.hpp> // mat4, dmat4
 #include <glm/vec2.hpp>   // vec2, bvec2, dvec2, ivec2 and uvec2
 #include <glm/vec3.hpp>   // vec3, bvec3, dvec3, ivec3 and uvec3
 #include <glm/vec4.hpp>   // vec4, bvec4, dvec4, ivec4 and uvec4
 #include <initializer_list>
 #include <map>
 #include <span>
 #include <string>
 #include <string_view>
 #include <vector>
 
 namespace opengl
 {
 
     enum class MatrixStyle
     {
         ColumnOrder,
         OpenGLStyle = ColumnOrder,
         RowOrder,
         DirectXStyle = RowOrder
     };
 
     class [[nodiscard]] Shader
     {
     public:
         enum Type : GLenum
         {
             VERTEX                  = GL_VERTEX_SHADER,
             FRAGMENT                = GL_FRAGMENT_SHADER,
             GEOMETRY                = GL_GEOMETRY_SHADER,
             TESSELLATION_CONTROL    = GL_TESS_CONTROL_SHADER,
             TESSELLATION_EVALUATION = GL_TESS_EVALUATION_SHADER,
             COMPUTE                 = GL_COMPUTE_SHADER
         };
 
     public:
         Shader() = default;
         Shader(std::string_view the_shader_name, const std::initializer_list<std::filesystem::path>& shader_paths);
         Shader(std::string_view the_shader_name, const std::span<const std::filesystem::path>& shader_paths);
 
         Shader(std::string_view shader_name, std::string_view vertex_shader_source, std::string_view fragment_shader_source);
         ~Shader();
 
         Shader(const Shader&) = delete;
         Shader(Shader&&) noexcept;
         Shader& operator=(const Shader&) = delete;
         Shader& operator=(Shader&&) noexcept;
 
         void Use(bool bind = true) const noexcept;
 
         opengl::Handle GetHandle() const noexcept
         {
             return program_handle;
         }
 
         const std::string& GetName() const
         {
             return shader_name;
         }
 
         void SetMatrixStyle(MatrixStyle style) noexcept
         {
             matrixStyle = style;
         }
 
         bool IsValidWithVertexArrayObject(opengl::Handle vertex_array_object_handle) const;
 
         void SendUniform(std::string_view name, float value) const;
         void SendUniform(std::string_view name, glm::vec2 value) const;
         void SendUniform(std::string_view name, glm::vec3 value) const;
         void SendUniform(std::string_view name, glm::vec4 value) const;
         void SendUniform(std::string_view name, int value) const;
         void SendUniform(std::string_view name, glm::ivec2 value) const;
         void SendUniform(std::string_view name, glm::ivec3 value) const;
         void SendUniform(std::string_view name, glm::ivec4 value) const;
         void SendUniform(std::string_view name, unsigned value) const;
         void SendUniform(std::string_view name, glm::uvec2 value) const;
         void SendUniform(std::string_view name, glm::uvec3 value) const;
         void SendUniform(std::string_view name, glm::uvec4 value) const;
         void SendUniform(std::string_view name, bool value) const;
         void SendUniform(std::string_view name, glm::bvec2 value) const;
         void SendUniform(std::string_view name, glm::bvec3 value) const;
         void SendUniform(std::string_view name, glm::bvec4 value) const;
         void SendUniform(std::string_view name, glm::mat2 mat) const;
         void SendUniform(std::string_view name, const glm::mat3& mat) const;
         void SendUniform(std::string_view name, const glm::mat4& mat) const;
         void SendUniform(std::string_view name, const glm::mat2x3& mat) const;
         void SendUniform(std::string_view name, const glm::mat2x4& mat) const;
         void SendUniform(std::string_view name, const glm::mat3x2& mat) const;
         void SendUniform(std::string_view name, const glm::mat3x4& mat) const;
         void SendUniform(std::string_view name, const glm::mat4x2& mat) const;
         void SendUniform(std::string_view name, const glm::mat4x3& mat) const;
 
 
     private:
         opengl::Handle                                  program_handle = 0;
         std::string                                     shader_name{};
         mutable std::map<std::string, int, std::less<>> uniforms{};
         MatrixStyle                                     matrixStyle{ MatrixStyle::OpenGLStyle };
 
     private:
         void              link_program(const std::vector<unsigned int>& shader);
         [[nodiscard]] int get_uniform_location(std::string_view uniform_name) const noexcept;
         void              delete_program() noexcept;
         void              print_active_uniforms() const;
         void              print_active_attributes() const;
     };
 }
 