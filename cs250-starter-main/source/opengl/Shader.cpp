/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "Shader.hpp"


#include "GL.hpp"
#include "assets/Path.hpp"
#include "opengl/Environment.hpp"
#include "util/Logger.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <gsl/gsl>
#include <span>
#include <sstream>
#include <vector>

namespace
{
    [[nodiscard]] inline gsl::czstring get_glsl_version_macro() noexcept
    {
        const auto gl_version = opengl::current_version();
        if constexpr (not opengl::IsWebGL)
        {
            switch (gl_version)
            {
                default: return "#version 110\n";
                case 210: return "#version 120\n";
                case 300: return "#version 130\n";
                case 310: return "#version 140\n";
                case 320: return "#version 150\n";
                case 330: return "#version 330\n";
                case 400: return "#version 400\n";
                case 410: return "#version 410\n";
                case 420: return "#version 420\n";
                case 430: return "#version 430\n";
                case 440: return "#version 440\n";
                case 450: return "#version 450\n";
                case 460: return "#version 460\n";
            }
        }
        else
        {
            // OpoenGL ES / WebGL Require setting the precisions so we are adding them here
            // This way we don't have to worry about forgetting it when writing shaders
            switch (gl_version)
            {
                default: return "#version 100\nprecision highp float;\nprecision highp int;\n";
                case 300: return "#version 300 es\nprecision highp float;\nprecision highp int;\n";
                case 310: return "#version 310 es\nprecision highp float;\nprecision highp int;\n";
                case 320: return "#version 320 es\nprecision highp float;\nprecision highp int;\n";
            }
        }
    }

    [[nodiscard]] constexpr size_t get_offset_after_version(const std::string_view glsl_text) noexcept
    {
        const size_t first_newline_position = glsl_text.find('\n');
        if (first_newline_position != std::string_view::npos)
        {
            const std::string_view first_text_line = glsl_text.substr(0, first_newline_position);
            if (first_text_line.find("#version") != std::string_view::npos)
            {
                return first_newline_position + 1;
            }
        }
        return 0;
    }

    void print_glsl_text(std::span<gsl::czstring> source)
    {
        using CountInt     = decltype(std::count(source[0], source[0] + std::strlen(source[0]), '\n'));
        CountInt num_lines = 0;
        for (const auto& text : source)
        {
            num_lines += std::count(text, text + strlen(text), '\n');
        }
        const int          max_linenumber_width = static_cast<int>(std::to_string(num_lines).size());
        CountInt           line_number          = 1;
        std::string        line;
        std::ostringstream sout;
        for (const auto& text : source)
        {
            std::istringstream source_stream(text);
            while (std::getline(source_stream, line))
            {
                sout << std::setw(max_linenumber_width) << std::right << line_number << "| " << line << '\n';
                ++line_number;
            }
        }
        util::log_debug("\n{}", sout.str());
    }

    [[nodiscard]] bool Compile(GLuint& shader, opengl::Shader::Type type, std::string_view glsl_text, std::string& error_log)
    {
        if (shader <= 0)
        {
            shader = GL::CreateShader(type);
            if (shader == 0)
            {
                error_log = "Unable to create shader\n";
                return false;
            }
        }
        // Use the OpenGL's current version glsl
        // If the glsl text specifies a version then we will skip over it
        GLchar const* source[]{ get_glsl_version_macro(), glsl_text.data() + get_offset_after_version(glsl_text) };
        GL::ShaderSource(shader, 2, source, nullptr);
        GL::CompileShader(shader);

        GLint is_compiled = 0;
        GL::GetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
        if (is_compiled == GL_FALSE)
        {
            GLint log_length = 0;
            GL::GetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
            error_log.resize(static_cast<std::string::size_type>(log_length) + 1);
            GL::GetShaderInfoLog(shader, log_length, nullptr, error_log.data());
            GL::DeleteShader(shader);
            shader = 0;
            print_glsl_text(source);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool Compile(GLuint& shader, opengl::Shader::Type type, std::filesystem::path file_path, std::string& error_log)
    {
        if (!std::filesystem::exists(file_path))
        {
            // try prepending the asset directory path
            file_path = assets::get_base_path() / file_path;
            if (!std::filesystem::exists(file_path))
            {
                error_log = "Cannot find " + file_path.string() + "\n";
                return false;
            }
        }
        std::ifstream ifs(file_path, std::ios::in);
        if (!ifs)
        {
            error_log = "Cannot open " + file_path.string() + "\n";
            return false;
        }
        std::string glsl_text;
        glsl_text.reserve(gsl::narrow<std::size_t>(std::filesystem::file_size(file_path)));
        std::copy((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>(), std::back_insert_iterator(glsl_text));
        return Compile(shader, type, std::string_view(glsl_text), error_log);
    }

    opengl::Shader::Type shader_type_from_extension(const std::filesystem::path& file_path) noexcept
    {
        switch (file_path.string().back())
        {
            // tesc extension
            case 'c': return opengl::Shader::TESSELLATION_CONTROL;
            // tese extension
            case 'e': return opengl::Shader::TESSELLATION_EVALUATION;
            // geom extension
            case 'm': return opengl::Shader::GEOMETRY;
            // frag extension
            case 'g': return opengl::Shader::FRAGMENT;
            // comp extension
            case 'p': return opengl::Shader::COMPUTE;
            default:
                // else vert extension is assumed
                return opengl::Shader::VERTEX;
        }
    }
}

namespace opengl
{

    Shader::Shader(std::string_view the_shader_name, const std::initializer_list<std::filesystem::path>& shader_paths)
        : Shader(the_shader_name, std::span(std::begin(shader_paths), std::end(shader_paths)))
    {
    }

    Shader::Shader(std::string_view the_shader_name, const std::span<const std::filesystem::path>& shader_paths) : program_handle(0), shader_name(the_shader_name), uniforms()
    {
        try
        {
            // Compile all shaders and attach them to the program
            std::vector<GLuint>::size_type index = 0;
            std::vector<GLuint>            shader(shader_paths.size());
            for (const auto& shader_path : shader_paths)
            {
                std::string error;
                const auto  shader_type = shader_type_from_extension(shader_path);
                const bool  is_compiled = Compile(shader[index], shader_type, std::filesystem::path(shader_path), error);
                // if any shader is failed to compile -> delete all shaders and program
                if (!is_compiled)
                {
                    for (auto& shdr : shader)
                    {
                        if (shdr > 0)
                        {
                            GL::DeleteShader(shdr);
                        }
                    }
                    throw std::runtime_error(error);
                }
                index++;
            }
            link_program(shader);
#if defined(DEVELOPER_VERSION)
            print_active_attributes();
            print_active_uniforms();
#endif
        }
        catch (...)
        {
            delete_program();
            throw;
        }
    }

    Shader::Shader(std::string_view the_shader_name, std::string_view vertex_shader_source, std::string_view fragment_shader_source) : program_handle(0), shader_name(the_shader_name), uniforms()
    {
        std::vector<GLuint>  shader(2);
        const std::array     sources = { vertex_shader_source, fragment_shader_source };
        constexpr std::array types   = { Shader::Type::VERTEX, Shader::Type::FRAGMENT };
        std::string          error;
        for (unsigned index = 0; index < 2; ++index)
        {
            const bool is_compiled = Compile(shader[index], types[index], sources[index], error);
            // if any shader is failed to compile -> delete all shaders and program
            if (!is_compiled)
            {
                for (auto shdr : shader)
                {
                    if (shdr > 0)
                    {
                        GL::DeleteShader(shdr);
                    }
                }
                throw std::runtime_error(error);
            }
        }
        link_program(shader);
#if defined(DEVELOPER_VERSION)
        print_active_attributes();
        print_active_uniforms();
#endif
    }

    Shader::~Shader()
    {
        delete_program();
    }

    Shader::Shader(Shader&& temp) noexcept : program_handle{ temp.program_handle }, shader_name{ std::move(temp.shader_name) }, uniforms{ std::move(temp.uniforms) }
    {
        temp.program_handle = 0;
    }

    Shader& Shader::operator=(Shader&& temp) noexcept
    {
        std::swap(program_handle, temp.program_handle);
        std::swap(shader_name, temp.shader_name);
        std::swap(uniforms, temp.uniforms);
        return *this;
    }

    void Shader::Use(bool bind) const noexcept
    {
        GL::UseProgram(bind ? program_handle : 0);
    }

    bool Shader::IsValidWithVertexArrayObject(opengl::Handle vertex_array_object_handle) const
    {
        if (program_handle == 0)
        {
            return false;
        }
        Use();
        GL::BindVertexArray(vertex_array_object_handle);
        // Check validate
        GL::ValidateProgram(program_handle);
        GLint is_validate = 0;
        GL::GetProgramiv(program_handle, GL_VALIDATE_STATUS, &is_validate);
        GL::BindVertexArray(0);
        Use(false);
        if (is_validate == GL_FALSE)
        {
            GLint log_length = 0;
            GL::GetProgramiv(program_handle, GL_INFO_LOG_LENGTH, &log_length);
            std::string error;
            error.resize(static_cast<unsigned>(log_length) + 1);
            GL::GetProgramInfoLog(program_handle, log_length, nullptr, error.data());
            util::log_error("{}", error);
            return false;
        }
        return true;
    }

    void Shader::SendUniform(std::string_view name, bool value) const
    {
        GL::Uniform1i(get_uniform_location(name), static_cast<int>(value));
    }

    void Shader::SendUniform(std::string_view name, glm::bvec2 value) const
    {
        GL::Uniform2i(get_uniform_location(name), value.x, value.y);
    }

    void Shader::SendUniform(std::string_view name, glm::bvec3 value) const
    {
        GL::Uniform3i(get_uniform_location(name), value.x, value.y, value.z);
    }

    void Shader::SendUniform(std::string_view name, glm::bvec4 value) const
    {
        GL::Uniform4i(get_uniform_location(name), value.x, value.y, value.z, value.w);
    }

    void Shader::SendUniform(std::string_view name, int value) const
    {
        GL::Uniform1i(get_uniform_location(name), value);
    }

    void Shader::SendUniform(std::string_view name, glm::ivec2 value) const
    {
        GL::Uniform2i(get_uniform_location(name), value.x, value.y);
    }

    void Shader::SendUniform(std::string_view name, glm::ivec3 value) const
    {
        GL::Uniform3i(get_uniform_location(name), value.x, value.y, value.z);
    }

    void Shader::SendUniform(std::string_view name, glm::ivec4 value) const
    {
        GL::Uniform4i(get_uniform_location(name), value.x, value.y, value.z, value.w);
    }

    void Shader::SendUniform(std::string_view name, unsigned value) const
    {
        GL::Uniform1ui(get_uniform_location(name), value);
    }

    void Shader::SendUniform(std::string_view name, glm::uvec2 value) const
    {
        GL::Uniform2ui(get_uniform_location(name), value.x, value.y);
    }

    void Shader::SendUniform(std::string_view name, glm::uvec3 value) const
    {
        GL::Uniform3ui(get_uniform_location(name), value.x, value.y, value.z);
    }

    void Shader::SendUniform(std::string_view name, glm::uvec4 value) const
    {
        GL::Uniform4ui(get_uniform_location(name), value.x, value.y, value.z, value.w);
    }

    void Shader::SendUniform(std::string_view name, float value) const
    {
        GL::Uniform1f(get_uniform_location(name), value);
    }

    void Shader::SendUniform(std::string_view name, glm::vec2 value) const
    {
        GL::Uniform2f(get_uniform_location(name), value.x, value.y);
    }

    void Shader::SendUniform(std::string_view name, glm::vec3 value) const
    {
        GL::Uniform3f(get_uniform_location(name), value.x, value.y, value.z);
    }

    void Shader::SendUniform(std::string_view name, glm::vec4 value) const
    {
        GL::Uniform4f(get_uniform_location(name), value.x, value.y, value.z, value.w);
    }

    void Shader::SendUniform(std::string_view name, glm::mat2 mat) const
    {
        GL::UniformMatrix2fv(get_uniform_location(name), 1, (matrixStyle == MatrixStyle::RowOrder) ? GL_TRUE : GL_FALSE, &mat[0][0]);
    }

    void Shader::SendUniform(std::string_view name, const glm::mat3& mat) const
    {
        GL::UniformMatrix3fv(get_uniform_location(name), 1, (matrixStyle == MatrixStyle::RowOrder) ? GL_TRUE : GL_FALSE, &mat[0][0]);
    }

    void Shader::SendUniform(std::string_view name, const glm::mat4& mat) const
    {
        GL::UniformMatrix4fv(get_uniform_location(name), 1, (matrixStyle == MatrixStyle::RowOrder) ? GL_TRUE : GL_FALSE, &mat[0][0]);
    }

    void Shader::SendUniform(std::string_view name, const glm::mat2x3& mat) const
    {
        GL::UniformMatrix2x3fv(get_uniform_location(name), 1, (matrixStyle == MatrixStyle::RowOrder) ? GL_TRUE : GL_FALSE, &mat[0][0]);
    }

    void Shader::SendUniform(std::string_view name, const glm::mat2x4& mat) const
    {
        GL::UniformMatrix2x4fv(get_uniform_location(name), 1, (matrixStyle == MatrixStyle::RowOrder) ? GL_TRUE : GL_FALSE, &mat[0][0]);
    }

    void Shader::SendUniform(std::string_view name, const glm::mat3x2& mat) const
    {
        GL::UniformMatrix3x2fv(get_uniform_location(name), 1, (matrixStyle == MatrixStyle::RowOrder) ? GL_TRUE : GL_FALSE, &mat[0][0]);
    }

    void Shader::SendUniform(std::string_view name, const glm::mat3x4& mat) const
    {
        GL::UniformMatrix3x4fv(get_uniform_location(name), 1, (matrixStyle == MatrixStyle::RowOrder) ? GL_TRUE : GL_FALSE, &mat[0][0]);
    }

    void Shader::SendUniform(std::string_view name, const glm::mat4x2& mat) const
    {
        GL::UniformMatrix4x2fv(get_uniform_location(name), 1, (matrixStyle == MatrixStyle::RowOrder) ? GL_TRUE : GL_FALSE, &mat[0][0]);
    }

    void Shader::SendUniform(std::string_view name, const glm::mat4x3& mat) const
    {
        GL::UniformMatrix4x3fv(get_uniform_location(name), 1, (matrixStyle == MatrixStyle::RowOrder) ? GL_TRUE : GL_FALSE, &mat[0][0]);
    }

    void Shader::link_program(const std::vector<unsigned int>& shader)
    {
        program_handle = GL::CreateProgram();
        if (program_handle == 0)
        {
            throw std::runtime_error("Unable to create program\n");
        }

        for (const auto shader_handle : shader)
        {
            GL::AttachShader(program_handle, shader_handle);
        }

        // Link shaders to the program
        GL::LinkProgram(program_handle);
        for (auto& shdr : shader)
        {
            GL::DeleteShader(shdr);
        }
        GLint is_linked = 0;
        GL::GetProgramiv(program_handle, GL_LINK_STATUS, &is_linked);
        if (is_linked == GL_FALSE)
        {
            GLint log_length = 0;
            GL::GetProgramiv(program_handle, GL_INFO_LOG_LENGTH, &log_length);
            std::string error;
            error.resize(static_cast<unsigned>(log_length) + 1);
            GL::GetProgramInfoLog(program_handle, log_length, nullptr, error.data());
            throw std::runtime_error(error);
        }
    }

    int Shader::get_uniform_location(std::string_view uniform_name) const noexcept
    {
        const auto iter_location = uniforms.find(uniform_name);
        if (iter_location == uniforms.end())
        {
            GLint location = GL::GetUniformLocation(program_handle, uniform_name.data());
            if (location < 0)
            {
                util::log_verbose("Uniform variable({}) does not exist", uniform_name);
                location = -1;
            }
            uniforms[std::string(uniform_name)] = location;
            return location;
        }
        return iter_location->second;
    }

    void Shader::delete_program() noexcept
    {
        GL::DeleteProgram(program_handle);
        program_handle = 0;
    }

    void Shader::print_active_uniforms() const
    {
        GLint max_length;
        GL::GetProgramiv(program_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_length);
        std::string name;
        name.resize(static_cast<unsigned>(max_length) + 1);
        GLint num_uniforms;
        GL::GetProgramiv(program_handle, GL_ACTIVE_UNIFORMS, &num_uniforms);
        std::ostringstream sout;
        sout << "----------------------------------------------------------------------" << '\r' << shader_name << " uniforms\n";
        sout << "Location\t|\tName\n";
        const GLuint uniform_count = gsl::narrow<GLuint>(num_uniforms);
        for (GLuint i = 0; i < uniform_count; ++i)
        {
            GLsizei written;
            GLint   size;
            GLenum  type;
            name.clear();
            name.resize(static_cast<unsigned>(max_length) + 1);
            GL::GetActiveUniform(program_handle, i, max_length, &written, &size, &type, name.data());
            name.resize(static_cast<std::string::size_type>(written));
            const auto loc = get_uniform_location(name);
            sout << loc << "\t\t" << name << '\n';
        }
        sout << "----------------------------------------------------------------------\n";
        util::log_verbose("\n{}", sout.str());
    }

    void Shader::print_active_attributes() const
    {
        GLint max_length, num_attribs;
        GL::GetProgramiv(program_handle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_length);
        GL::GetProgramiv(program_handle, GL_ACTIVE_ATTRIBUTES, &num_attribs);
        std::string name;
        name.resize(static_cast<unsigned>(max_length) + 1);
        std::ostringstream sout;
        sout << "----------------------------------------------------------------------" << '\r' << shader_name << " vertex attributes\n";
        sout << "Index\t|\tName\n";

        const GLuint attribs_count = gsl::narrow<GLuint>(num_attribs);
        for (GLuint i = 0; i < attribs_count; ++i)
        {
            GLsizei written;
            GLint   size;
            GLenum  type;
            name.clear();
            name.resize(static_cast<unsigned>(max_length) + 1);
            GL::GetActiveAttrib(program_handle, i, max_length, &written, &size, &type, name.data());
            name.resize(static_cast<std::string::size_type>(written));
            GLint loc = GL::GetAttribLocation(program_handle, name.c_str());
            sout << loc << "\t\t" << name << '\n';
        }
        sout << "----------------------------------------------------------------------\n";
        util::log_verbose("\n{}", sout.str());
    }
}
