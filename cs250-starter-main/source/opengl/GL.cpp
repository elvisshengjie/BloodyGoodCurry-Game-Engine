/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "opengl/GL.hpp"
#include "util/Logger.hpp"

#include <GL/glew.h>
#include <cassert>
#include <sstream>
#include <string>


#if defined(DEVELOPER_VERSION)
#    include <source_location>
#    define VOID_SOURCE_LOCATION const std::source_location caller_location
#    define SOURCE_LOCATION      , VOID_SOURCE_LOCATION
#    define glCheck(expression)                                                                                                                                                                        \
        expression;                                                                                                                                                                                    \
        glCheckError(caller_location.file_name(), caller_location.line(), caller_location.function_name(), #expression)

namespace
{
    inline void glCheckError(const char* file, unsigned line, const char* function_name, const char* opengl_function)
    {
        GLenum errorCode = glGetError();

        if (errorCode == GL_NO_ERROR)
            return;

        const std::string  fileString  = file;
        std::string        error       = "Unknown error";
        std::string        description = "No description";
        std::ostringstream serr;

        serr << "OpenGL call " << opengl_function << " failed in " << fileString.substr(fileString.find_last_of("\\/") + 1) << "(" << line << ")."
             << "\nwithin Function:\n   " << function_name << "\nError description:\n   ";
        int loop_limit = 0;
        while (errorCode != GL_NO_ERROR && loop_limit < 3)
        {
            ++loop_limit;
            switch (errorCode)
            {
                case GL_INVALID_ENUM:
                    {
                        error       = "GL_INVALID_ENUM";
                        description = "An unacceptable value has been specified for an enumerated argument.";
                        break;
                    }

                case GL_INVALID_VALUE:
                    {
                        error       = "GL_INVALID_VALUE";
                        description = "A numeric argument is out of range.";
                        break;
                    }

                case GL_INVALID_OPERATION:
                    {
                        error       = "GL_INVALID_OPERATION";
                        description = "The specified operation is not allowed in the current state.";
                        break;
                    }

                case GL_STACK_OVERFLOW:
                    {
                        error       = "GL_STACK_OVERFLOW";
                        description = "This command would cause a stack overflow.";
                        break;
                    }

                case GL_STACK_UNDERFLOW:
                    {
                        error       = "GL_STACK_UNDERFLOW";
                        description = "This command would cause a stack underflow.";
                        break;
                    }

                case GL_OUT_OF_MEMORY:
                    {
                        error       = "GL_OUT_OF_MEMORY";
                        description = "There is not enough memory left to execute the command.";
                        break;
                    }

                default:
                    {
                        error += " " + std::to_string(errorCode);
                        break;
                    }
            }

            serr << error << "\n   " << description << "\n\n";
            errorCode = glGetError();
        }
        util::log_error("{}", serr.str());
        assert(false);
    }
}
#else
#    define SOURCE_LOCATION
#    define VOID_SOURCE_LOCATION void
#    define glCheck(expression)  expression
#endif


namespace GL
{
    const GLubyte* GetString(GLenum name SOURCE_LOCATION)
    {
        glCheck(const auto the_string = glGetString(name));
        return the_string;
    }

    GLint GetAttribLocation(GLuint program, const GLchar* name SOURCE_LOCATION)
    {
        glCheck(const auto location = glGetAttribLocation(program, name));
        return location;
    }

    GLint GetUniformLocation(GLuint program, const GLchar* name SOURCE_LOCATION)
    {
        glCheck(const auto location = glGetUniformLocation(program, name));
        return location;
    }

    GLuint CreateProgram(VOID_SOURCE_LOCATION)
    {
        glCheck(const auto program = glCreateProgram());
        return program;
    }

    GLuint CreateShader(GLenum shaderType SOURCE_LOCATION)
    {
        glCheck(const auto shader = glCreateShader(shaderType));
        return shader;
    }

    void ActiveTexture(GLenum texture SOURCE_LOCATION)
    {
        glCheck(glActiveTexture(texture));
    }

    void AttachShader(GLuint program, GLuint shader SOURCE_LOCATION)
    {
        glCheck(glAttachShader(program, shader));
    }

    void BindBuffer(GLenum target, GLuint buffer SOURCE_LOCATION)
    {
        glCheck(glBindBuffer(target, buffer));
    }

    void BindTexture(GLenum target, GLuint texture SOURCE_LOCATION)
    {
        glCheck(glBindTexture(target, texture));
    }

    void BlendEquation(GLenum mode SOURCE_LOCATION)
    {
        glCheck(glBlendEquation(mode));
    }

    void BlendFunc(GLenum sfactor, GLenum dfactor SOURCE_LOCATION)
    {
        glCheck(glBlendFunc(sfactor, dfactor));
    }

    void BufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage SOURCE_LOCATION)
    {
        glCheck(glBufferData(target, size, data, usage));
    }

    void BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data SOURCE_LOCATION)
    {
        glCheck(glBufferSubData(target, offset, size, data));
    }

    void Clear(GLbitfield mask SOURCE_LOCATION)
    {
        glCheck(glClear(mask));
    }

    void ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha SOURCE_LOCATION)
    {
        glCheck(glClearColor(red, green, blue, alpha));
    }

    void CompileShader(GLuint shader SOURCE_LOCATION)
    {
        glCheck(glCompileShader(shader));
    }

    void CullFace(GLenum mode SOURCE_LOCATION)
    {
        glCheck(glCullFace(mode));
    }

    void DeleteBuffers(GLsizei n, const GLuint* buffers SOURCE_LOCATION)
    {
        glCheck(glDeleteBuffers(n, buffers));
    }

    void DeleteProgram(GLuint program SOURCE_LOCATION)
    {
        glCheck(glDeleteProgram(program));
    }

    void DeleteShader(GLuint shader SOURCE_LOCATION)
    {
        glCheck(glDeleteShader(shader));
    }

    void DeleteTextures(GLsizei n, const GLuint* textures SOURCE_LOCATION)
    {
        glCheck(glDeleteTextures(n, textures));
    }

    void DepthMask(GLboolean flag SOURCE_LOCATION)
    {
        glCheck(glDepthMask(flag));
    }

    void Disable(GLenum cap SOURCE_LOCATION)
    {
        glCheck(glDisable(cap));
    }

    void DrawArrays(GLenum mode, GLint first, GLsizei count SOURCE_LOCATION)
    {
        glCheck(glDrawArrays(mode, first, count));
    }

    void DrawBuffers(GLsizei n, const GLenum* bufs SOURCE_LOCATION)
    {
        glCheck(glDrawBuffers(n, bufs));
    }

    void DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices SOURCE_LOCATION)
    {
        glCheck(glDrawElements(mode, count, type, indices));
    }

    void Enable(GLenum cap SOURCE_LOCATION)
    {
        glCheck(glEnable(cap));
    }

    void EnableVertexAttribArray(GLuint index SOURCE_LOCATION)
    {
        glCheck(glEnableVertexAttribArray(index));
    }

    void FrontFace(GLenum mode SOURCE_LOCATION)
    {
        glCheck(glFrontFace(mode));
    }

    void GenBuffers(GLsizei n, GLuint* buffers SOURCE_LOCATION)
    {
        glCheck(glGenBuffers(n, buffers));
    }

    void GenTextures(GLsizei n, GLuint* textures SOURCE_LOCATION)
    {
        glCheck(glGenTextures(n, textures));
    }

    void GetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name SOURCE_LOCATION)
    {
        glCheck(glGetActiveAttrib(program, index, bufSize, length, size, type, name));
    }

    void GetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name SOURCE_LOCATION)
    {
        glCheck(glGetActiveUniform(program, index, bufSize, length, size, type, name));
    }

    void GetBooleanv(GLenum pname, GLboolean* data SOURCE_LOCATION)
    {
        glCheck(glGetBooleanv(pname, data));
    }

    void GetIntegerv(GLenum pname, GLint* data SOURCE_LOCATION)
    {
        glCheck(glGetIntegerv(pname, data));
    }

    void GetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog SOURCE_LOCATION)
    {
        glCheck(glGetProgramInfoLog(program, maxLength, length, infoLog));
    }

    void GetProgramiv(GLuint program, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetProgramiv(program, pname, params));
    }

    void GetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog SOURCE_LOCATION)
    {
        glCheck(glGetShaderInfoLog(shader, maxLength, length, infoLog));
    }

    void GetShaderiv(GLuint shader, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetShaderiv(shader, pname, params));
    }

    void GetUniformfv(GLuint program, GLint location, GLfloat* params SOURCE_LOCATION)
    {
        glCheck(glGetUniformfv(program, location, params));
    }

    void GetUniformiv(GLuint program, GLint location, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetUniformiv(program, location, params));
    }

    void GetUniformuiv(GLuint program, GLint location, GLuint* params SOURCE_LOCATION)
    {
        glCheck(glGetUniformuiv(program, location, params));
    }

    void Hint(GLenum target, GLenum mode SOURCE_LOCATION)
    {
        glCheck(glHint(target, mode));
    }

    void LinkProgram(GLuint program SOURCE_LOCATION)
    {
        glCheck(glLinkProgram(program));
    }

    void PolygonOffset(GLfloat factor, GLfloat units SOURCE_LOCATION)
    {
        glCheck(glPolygonOffset(factor, units));
    }

    void ShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length SOURCE_LOCATION)
    {
        glCheck(glShaderSource(shader, count, string, length));
    }

    void TexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data SOURCE_LOCATION)
    {
        glCheck(glTexImage2D(target, level, internalFormat, width, height, border, format, type, data));
    }

    void TexParameterfv(GLenum target, GLenum pname, const GLfloat* params SOURCE_LOCATION)
    {
        glCheck(glTexParameterfv(target, pname, params));
    }

    void TexParameteri(GLenum target, GLenum pname, GLint param SOURCE_LOCATION)
    {
        glCheck(glTexParameteri(target, pname, param));
    }

    void TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels SOURCE_LOCATION)
    {
        glCheck(glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels));
    }

    void Uniform1f(GLint location, GLfloat v0 SOURCE_LOCATION)
    {
        glCheck(glUniform1f(location, v0));
    }

    void Uniform1i(GLint location, GLint v0 SOURCE_LOCATION)
    {
        glCheck(glUniform1i(location, v0));
    }

    void Uniform1ui(GLint location, GLuint v0 SOURCE_LOCATION)
    {
        glCheck(glUniform1ui(location, v0));
    }

    void Uniform2f(GLint location, GLfloat v0, GLfloat v1 SOURCE_LOCATION)
    {
        glCheck(glUniform2f(location, v0, v1));
    }

    void Uniform2fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniform2fv(location, count, value));
    }

    void Uniform2i(GLint location, GLint v0, GLint v1 SOURCE_LOCATION)
    {
        glCheck(glUniform2i(location, v0, v1));
    }

    void Uniform2ui(GLint location, GLuint v0, GLuint v1 SOURCE_LOCATION)
    {
        glCheck(glUniform2ui(location, v0, v1));
    }

    void Uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2 SOURCE_LOCATION)
    {
        glCheck(glUniform3f(location, v0, v1, v2));
    }

    void Uniform3fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniform3fv(location, count, value));
    }

    void Uniform3i(GLint location, GLint v0, GLint v1, GLint v2 SOURCE_LOCATION)
    {
        glCheck(glUniform3i(location, v0, v1, v2));
    }

    void Uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2 SOURCE_LOCATION)
    {
        glCheck(glUniform3ui(location, v0, v1, v2));
    }

    void Uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 SOURCE_LOCATION)
    {
        glCheck(glUniform4f(location, v0, v1, v2, v3));
    }

    void Uniform4fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniform4fv(location, count, value));
    }

    void Uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3 SOURCE_LOCATION)
    {
        glCheck(glUniform4i(location, v0, v1, v2, v3));
    }

    void Uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 SOURCE_LOCATION)
    {
        glCheck(glUniform4ui(location, v0, v1, v2, v3));
    }

    void UniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix2fv(location, count, transpose, value));
    }

    void UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix2x3fv(location, count, transpose, value));
    }

    void UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix2x4fv(location, count, transpose, value));
    }

    void UniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix3fv(location, count, transpose, value));
    }

    void UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix3x2fv(location, count, transpose, value));
    }

    void UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix3x4fv(location, count, transpose, value));
    }

    void UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix4fv(location, count, transpose, value));
    }

    void UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix4x2fv(location, count, transpose, value));
    }

    void UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix4x3fv(location, count, transpose, value));
    }

    void UseProgram(GLuint program SOURCE_LOCATION)
    {
        glCheck(glUseProgram(program));
    }

    void ValidateProgram(GLuint program SOURCE_LOCATION)
    {
        glCheck(glValidateProgram(program));
    }

    void VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer SOURCE_LOCATION)
    {
        glCheck(glVertexAttribPointer(index, size, type, normalized, stride, pointer));
    }

    void VertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer SOURCE_LOCATION)
    {
        glCheck(glVertexAttribIPointer(index, size, type, stride, pointer));
    }

    void VertexAttribDivisor(GLuint index, GLuint divisor SOURCE_LOCATION)
    {
        glCheck(glVertexAttribDivisor(index, divisor));
    }

    void Viewport(GLint x, GLint y, GLsizei width, GLsizei height SOURCE_LOCATION)
    {
        glCheck(glViewport(x, y, width, height));
    }

    GLenum CheckFramebufferStatus(GLenum target SOURCE_LOCATION)
    {
        glCheck(const GLenum status = glCheckFramebufferStatus(target));
        return status;
    }

    void BindFramebuffer(GLenum target, GLuint framebuffer SOURCE_LOCATION)
    {
        glCheck(glBindFramebuffer(target, framebuffer));
    }

    void BindVertexArray(GLuint array SOURCE_LOCATION)
    {
        glCheck(glBindVertexArray(array));
    }

    void DeleteFramebuffers(GLsizei n, GLuint* framebuffers SOURCE_LOCATION)
    {
        glCheck(glDeleteFramebuffers(n, framebuffers));
    }

    void DeleteVertexArrays(GLsizei n, const GLuint* arrays SOURCE_LOCATION)
    {
        glCheck(glDeleteVertexArrays(n, arrays));
    }

    void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level SOURCE_LOCATION)
    {
        glCheck(glFramebufferTexture2D(target, attachment, textarget, texture, level));
    }

    void GenFramebuffers(GLsizei n, GLuint* framebuffers SOURCE_LOCATION)
    {
        glCheck(glGenFramebuffers(n, framebuffers));
    }

    void GenVertexArrays(GLsizei n, GLuint* arrays SOURCE_LOCATION)
    {
        glCheck(glGenVertexArrays(n, arrays));
    }

    void TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height SOURCE_LOCATION)
    {
        glCheck(glTexStorage2D(target, levels, internalformat, width, height));
    }

}
