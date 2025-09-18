#include "Graphics.hpp"
#include <vector>
#include <cmath>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

constexpr float PI = 3.14159265359f;

namespace gfx {

    unsigned int Graphics::VAO_rect = 0;
    unsigned int Graphics::VBO_rect = 0;
    unsigned int Graphics::VAO_circle = 0;
    unsigned int Graphics::VBO_circle = 0;
    int Graphics::circleVertexCount = 0;
    unsigned int Graphics::VAO_bg = 0;
    unsigned int Graphics::VBO_bg = 0;
    unsigned int Graphics::bgTexture = 0;
    unsigned int Graphics::bgShader = 0;
    unsigned int Graphics::objectShader = 0;

    struct Circle {
        float x, y, r;
    };
    static std::vector<Circle> circles; // store circle info
    static int segments = 50;

    // -------- Shader helpers --------
    static unsigned int compileShader(const char* source, GLenum type) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        int success;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
        }
        return shader;
    }

    static unsigned int createShaderProgram(const char* vSource, const char* fSource) {
        unsigned int vertex = compileShader(vSource, GL_VERTEX_SHADER);
        unsigned int fragment = compileShader(fSource, GL_FRAGMENT_SHADER);

        unsigned int program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);

        int success;
        char infoLog[512];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);

        return program;
    }

    // -------- Texture loader --------
    static unsigned int loadTexture(const char* path) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width, height, nrChannels;
        //stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else {
            std::cerr << "Failed to load texture: " << path << std::endl;
        }
        stbi_image_free(data);

        return textureID;
    }

    void Graphics::initialize() {
        // -------- Smaller Rectangle --------
        float rectVertices[] = {
            -0.3f, -0.4f, 0.0f,  0.0f, 0.0f, 1.0f,
             0.3f, -0.4f, 0.0f,  0.0f, 0.0f, 1.0f,
             0.3f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,

            -0.3f, -0.4f, 0.0f,  0.0f, 0.0f, 1.0f,
             0.3f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
            -0.3f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f
        };

        glGenVertexArrays(1, &VAO_rect);
        glGenBuffers(1, &VBO_rect);
        glBindVertexArray(VAO_rect);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_rect);
        glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // -------- Multiple Circles --------
        circles = {
            { -0.7f,  0.6f, 0.1f },
            {  0.0f,  0.6f, 0.1f },
            {  0.7f,  0.6f, 0.1f }
        };

        std::vector<float> circleVertices;
        for (auto& c : circles) {
            circleVertices.push_back(c.x);
            circleVertices.push_back(c.y);
            circleVertices.push_back(0.0f);
            circleVertices.push_back(0.0f);
            circleVertices.push_back(0.0f);
            circleVertices.push_back(1.0f);

            for (int i = 0; i <= segments; i++) {
                float angle = (2.0f * PI * i) / segments;
                float x = c.x + c.r * cos(angle);
                float y = c.y + c.r * sin(angle);

                circleVertices.push_back(x);
                circleVertices.push_back(y);
                circleVertices.push_back(0.0f);

                circleVertices.push_back(0.0f);
                circleVertices.push_back(0.0f);
                circleVertices.push_back(1.0f);
            }
        }

        circleVertexCount = (segments + 2);

        glGenVertexArrays(1, &VAO_circle);
        glGenBuffers(1, &VBO_circle);
        glBindVertexArray(VAO_circle);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_circle);
        glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // -------- Background Quad --------
        float bgVertices[] = {
            -1.0f,  1.0f,  0.0f, 0.0f,
            -1.0f, -1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 1.0f,

            -1.0f,  1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 1.0f,
             1.0f,  1.0f,  1.0f, 0.0f
        };

        glGenVertexArrays(1, &VAO_bg);
        glGenBuffers(1, &VBO_bg);
        glBindVertexArray(VAO_bg);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_bg);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bgVertices), bgVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        bgTexture = loadTexture("../../assets/house.jpg");

        const char* bgVertexSrc =
            "#version 330 core\n"
            "layout (location = 0) in vec2 aPos;\n"
            "layout (location = 1) in vec2 aTexCoord;\n"
            "out vec2 TexCoord;\n"
            "void main() {\n"
            "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
            "    TexCoord = aTexCoord;\n"
            "}\n";

        const char* bgFragmentSrc =
            "#version 330 core\n"
            "out vec4 FragColor;\n"
            "in vec2 TexCoord;\n"
            "uniform sampler2D backgroundTex;\n"
            "void main() {\n"
            "    FragColor = texture(backgroundTex, TexCoord);\n"
            "}\n";

        bgShader = createShaderProgram(bgVertexSrc, bgFragmentSrc);

        const char* objVertexSrc =
            "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "layout (location = 1) in vec3 aColor;\n"
            "out vec3 ourColor;\n"
            "void main() {\n"
            "    gl_Position = vec4(aPos, 1.0);\n"
            "    ourColor = aColor;\n"
            "}\n";

        const char* objFragmentSrc =
            "#version 330 core\n"
            "in vec3 ourColor;\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            "    FragColor = vec4(ourColor, 1.0);\n"
            "}\n";

        objectShader = createShaderProgram(objVertexSrc, objFragmentSrc);
    }

    void Graphics::renderBackground() {
        glUseProgram(bgShader);
        glBindTexture(GL_TEXTURE_2D, bgTexture);
        glBindVertexArray(VAO_bg);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void Graphics::renderRectangle() {
        glUseProgram(objectShader);
        glBindVertexArray(VAO_rect);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glUseProgram(0);
    }
    void Graphics::renderCircle() {
        glUseProgram(objectShader);
        glBindVertexArray(VAO_circle);

        int verticesPerCircle = segments + 2; // 1 center + segments + 1 to close
        for (size_t i = 0; i < circles.size(); i++) {
            glDrawArrays(GL_TRIANGLE_FAN, static_cast<GLint>(i * verticesPerCircle), verticesPerCircle);
        }

        glBindVertexArray(0);
        glUseProgram(0);
    }


    void Graphics::cleanup() {
        glDeleteVertexArrays(1, &VAO_rect);
        glDeleteBuffers(1, &VBO_rect);
        glDeleteVertexArrays(1, &VAO_circle);
        glDeleteBuffers(1, &VBO_circle);
        glDeleteVertexArrays(1, &VAO_bg);
        glDeleteBuffers(1, &VBO_bg);
        glDeleteTextures(1, &bgTexture);
        glDeleteProgram(bgShader);
        glDeleteProgram(objectShader);
    }

} // namespace gfx
