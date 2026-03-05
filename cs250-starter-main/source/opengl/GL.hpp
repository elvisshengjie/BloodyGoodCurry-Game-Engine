/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once
#include <cstddef> // for ptrdiff_t

typedef unsigned int   GLenum;
typedef unsigned int   GLbitfield;
typedef unsigned int   GLuint;
typedef int            GLint;
typedef int            GLsizei;
typedef unsigned char  GLboolean;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned long  GLulong;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef double         GLclampd;
typedef void           GLvoid;
typedef char           GLchar;
typedef ptrdiff_t      GLintptr;
typedef ptrdiff_t      GLsizeiptr;


#if defined(DEVELOPER_VERSION)
#    include <source_location>
#    define VOID_SOURCE_LOCATION const std::source_location caller_location = std::source_location::current()
#    define SOURCE_LOCATION      , VOID_SOURCE_LOCATION
#else
#    define SOURCE_LOCATION
#    define VOID_SOURCE_LOCATION void
#endif


namespace GL
{
    // Opengl Version 2.0
    const GLubyte* GetString(GLenum name SOURCE_LOCATION);
    GLint          GetAttribLocation(GLuint program, const GLchar* name SOURCE_LOCATION);
    GLint          GetUniformLocation(GLuint program, const GLchar* name SOURCE_LOCATION);
    GLuint         CreateProgram(VOID_SOURCE_LOCATION);
    GLuint         CreateShader(GLenum shaderType SOURCE_LOCATION);
    void           ActiveTexture(GLenum texture SOURCE_LOCATION);
    void           AttachShader(GLuint program, GLuint shader SOURCE_LOCATION);
    void           BindBuffer(GLenum target, GLuint buffer SOURCE_LOCATION);
    void           BindTexture(GLenum target, GLuint texture SOURCE_LOCATION);
    void           BlendEquation(GLenum mode SOURCE_LOCATION);
    void           BlendFunc(GLenum sfactor, GLenum dfactor SOURCE_LOCATION);
    void           BufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage SOURCE_LOCATION);
    void           BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data SOURCE_LOCATION);
    void           Clear(GLbitfield mask SOURCE_LOCATION);
    void           ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha SOURCE_LOCATION);
    void           CompileShader(GLuint shader SOURCE_LOCATION);
    void           CullFace(GLenum mode SOURCE_LOCATION);
    void           DeleteBuffers(GLsizei n, const GLuint* buffers SOURCE_LOCATION);
    void           DeleteProgram(GLuint program SOURCE_LOCATION);
    void           DeleteShader(GLuint shader SOURCE_LOCATION);
    void           DeleteTextures(GLsizei n, const GLuint* textures SOURCE_LOCATION);
    void           DepthMask(GLboolean flag SOURCE_LOCATION);
    void           Disable(GLenum cap SOURCE_LOCATION);
    void           DrawArrays(GLenum mode, GLint first, GLsizei count SOURCE_LOCATION);
    void           DrawBuffers(GLsizei n, const GLenum* bufs SOURCE_LOCATION);
    void           DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices SOURCE_LOCATION);
    void           Enable(GLenum cap SOURCE_LOCATION);
    void           EnableVertexAttribArray(GLuint index SOURCE_LOCATION);
    void           FrontFace(GLenum mode SOURCE_LOCATION);
    void           GenBuffers(GLsizei n, GLuint* buffers SOURCE_LOCATION);
    void           GenTextures(GLsizei n, GLuint* textures SOURCE_LOCATION);
    void           GetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name SOURCE_LOCATION);
    void           GetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name SOURCE_LOCATION);
    void           GetBooleanv(GLenum pname, GLboolean* data SOURCE_LOCATION);
    void           GetIntegerv(GLenum pname, GLint* data SOURCE_LOCATION);
    void           GetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog SOURCE_LOCATION);
    void           GetProgramiv(GLuint program, GLenum pname, GLint* params SOURCE_LOCATION);
    void           GetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog SOURCE_LOCATION);
    void           GetShaderiv(GLuint shader, GLenum pname, GLint* params SOURCE_LOCATION);
    void           GetUniformfv(GLuint program, GLint location, GLfloat* params SOURCE_LOCATION);
    void           GetUniformiv(GLuint program, GLint location, GLint* params SOURCE_LOCATION);
    void           GetUniformuiv(GLuint program, GLint location, GLuint* params SOURCE_LOCATION);
    void           Hint(GLenum target, GLenum mode SOURCE_LOCATION);
    void           LinkProgram(GLuint program SOURCE_LOCATION);
    void           PolygonOffset(GLfloat factor, GLfloat units SOURCE_LOCATION);
    void           ShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length SOURCE_LOCATION);
    void           TexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data SOURCE_LOCATION);
    void           TexParameterfv(GLenum target, GLenum pname, const GLfloat* params SOURCE_LOCATION);
    void           TexParameteri(GLenum target, GLenum pname, GLint param SOURCE_LOCATION);
    void           TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels SOURCE_LOCATION);
    void           Uniform1f(GLint location, GLfloat v0 SOURCE_LOCATION);
    void           Uniform1i(GLint location, GLint v0 SOURCE_LOCATION);
    void           Uniform1ui(GLint location, GLuint v0 SOURCE_LOCATION);
    void           Uniform2f(GLint location, GLfloat v0, GLfloat v1 SOURCE_LOCATION);
    void           Uniform2fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION);
    void           Uniform2i(GLint location, GLint v0, GLint v1 SOURCE_LOCATION);
    void           Uniform2ui(GLint location, GLuint v0, GLuint v1 SOURCE_LOCATION);
    void           Uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2 SOURCE_LOCATION);
    void           Uniform3fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION);
    void           Uniform3i(GLint location, GLint v0, GLint v1, GLint v2 SOURCE_LOCATION);
    void           Uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2 SOURCE_LOCATION);
    void           Uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 SOURCE_LOCATION);
    void           Uniform4fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION);
    void           Uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3 SOURCE_LOCATION);
    void           Uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 SOURCE_LOCATION);
    void           UniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UseProgram(GLuint program SOURCE_LOCATION);
    void           ValidateProgram(GLuint program SOURCE_LOCATION);
    void           VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer SOURCE_LOCATION);
    void           VertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer SOURCE_LOCATION);
    void           VertexAttribDivisor(GLuint index, GLuint divisor SOURCE_LOCATION);
    void           Viewport(GLint x, GLint y, GLsizei width, GLsizei height SOURCE_LOCATION);


    // Opengl Version 3.0
    GLenum CheckFramebufferStatus(GLenum target SOURCE_LOCATION);
    void   BindFramebuffer(GLenum target, GLuint framebuffer SOURCE_LOCATION);
    void   BindVertexArray(GLuint array SOURCE_LOCATION);
    void   DeleteFramebuffers(GLsizei n, GLuint* framebuffers SOURCE_LOCATION);
    void   DeleteVertexArrays(GLsizei n, const GLuint* arrays SOURCE_LOCATION);
    void   FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level SOURCE_LOCATION);
    void   GenFramebuffers(GLsizei n, GLuint* framebuffers SOURCE_LOCATION);
    void   GenVertexArrays(GLsizei n, GLuint* arrays SOURCE_LOCATION);


    // Opengl ES 3.0 or Opengl Version 4.2
    void TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height SOURCE_LOCATION);


}

#undef SOURCE_LOCATION
#undef VOID_SOURCE_LOCATION
