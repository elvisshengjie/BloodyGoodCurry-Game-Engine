#pragma once
#include <glad/glad.h>

// A minimal 4x4 matrix (column-major, like OpenGL expects)
struct Mat4 {
    float m[16]{};
};

// ===== Matrix construction =====

// Identity matrix
Mat4 Identity();

// Orthographic projection matrix
Mat4 Ortho(float left, float right, float bottom, float top,
    float zNear = -1.0f, float zFar = 1.0f);

// Multiply two 4x4 matrices (R = A * B)
Mat4 Mul(const Mat4& A, const Mat4& B);

// Translation matrix (2D translation, z = 0)
Mat4 Translate(float x, float y);

// Scaling matrix (2D scale, z = 0)
Mat4 Scale(float sx, float sy);

// Rotation matrix around Z axis
Mat4 RotateZ(float radians);

// Convert degrees to radians
float DegToRad(float degrees);

// ===== OpenGL helpers =====

// Compile a GLSL shader of given type (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
// Logs errors to stderr if compilation fails.
GLuint Compile(GLenum type, const char* src);

// Link a shader program from a vertex and fragment shader.
// Deletes the shaders after linking. Logs errors to stderr if linking fails.
GLuint Link(GLuint vs, GLuint fs);

// ===== Simple Quad Mesh =====
// A unit quad centered at the origin, pivot at center.
struct QuadGL {
    GLuint vao = 0, vbo = 0, ebo = 0;

    // Create the VAO/VBO/EBO and upload quad vertex/index data
    void create();

    // Destroy GL buffers and VAO
    void destroy();
};
