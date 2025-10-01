#include "Graphics.hpp"
#include <vector>
#include <cmath>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace gfx {

    constexpr float PI = 3.14159265359f;

    // --- Static GL handles ---
    unsigned int Graphics::VAO_rect = 0;
    unsigned int Graphics::VBO_rect = 0;
    unsigned int Graphics::VAO_circle = 0;
    unsigned int Graphics::VBO_circle = 0;
    int          Graphics::circleVertexCount = 0;
    unsigned int Graphics::VAO_bg = 0;
    unsigned int Graphics::VBO_bg = 0;
    unsigned int Graphics::bgTexture = 0;
    unsigned int Graphics::bgShader = 0;
    unsigned int Graphics::objectShader = 0;
    unsigned int Graphics::VAO_sprite = 0;
    unsigned int Graphics::VBO_sprite = 0;
    unsigned int Graphics::EBO_sprite = 0;
    unsigned int Graphics::spriteShader = 0;

    // Circle tessellation segments
    static int segments = 50;

    // --- NEW (minimal change): cache the rectangle's local-space geometric center (pivot) ---
    // These are computed once in initialize() from your current rect vertex data (no behavior change except rotation pivot).
    static float sRectPivotX = 0.0f;
    static float sRectPivotY = 0.0f;

    // ---------------- Shader helpers ----------------
    static unsigned int compileShader(const char* source, GLenum type) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        int success; char infoLog[512];
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
        int success; char infoLog[512];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
        }
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return program;
    }

    // ---------------- Texture ----------------
    unsigned int Graphics::loadTexture(const char* path) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true);
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
        glBindTexture(GL_TEXTURE_2D, 0);
        return textureID;
    }

    // ---------------- Init ----------------
    void Graphics::initialize() {
        // NOTE: Keep your original rectangle positions and per-vertex colors EXACTLY AS THEY WERE.
        // We will compute the local pivot from these values so only the rotation center changes.
        float rectVertices[] = {
            //  x      y      z      r     g     b
            -0.3f, -0.4f, 0.0f,   1.0f, 0.0f, 0.0f,  // 0: bottom-left  (red)
             0.3f, -0.4f, 0.0f,   0.0f, 1.0f, 0.0f,  // 1: bottom-right (green)
             0.3f,  0.0f, 0.0f,   0.0f, 0.0f, 1.0f,  // 2: top-right    (blue)
            -0.3f,  0.0f, 0.0f,   1.0f, 1.0f, 0.0f   // 3: top-left     (yellow)
        };
        unsigned int rectIndices[] = { 0,1,2, 2,3,0 };

        // --- Compute local-space geometric center (pivot) from current vertices (minimal change) ---
        // Stride = 6 floats per vertex: [x,y,z,r,g,b]
        sRectPivotX = (rectVertices[0] + rectVertices[6] + rectVertices[12] + rectVertices[18]) * 0.25f;
        sRectPivotY = (rectVertices[1] + rectVertices[7] + rectVertices[13] + rectVertices[19]) * 0.25f;
        // With your data above this results in (0.0f, -0.2f)

        unsigned int EBO_rect;
        glGenVertexArrays(1, &VAO_rect);
        glGenBuffers(1, &VBO_rect);
        glGenBuffers(1, &EBO_rect);

        glBindVertexArray(VAO_rect);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_rect);
        glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_rect);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIndices), rectIndices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // ----- Circle: unit circle fan centered at origin -----
        std::vector<float> circleVertices;
        circleVertices.reserve((segments + 2) * 6);
        // center vertex
        circleVertices.insert(circleVertices.end(), { 0.f, 0.f, 0.f, 0.f, 0.f, 1.f });
        for (int i = 0; i <= segments; ++i) {
            float angle = (2.0f * PI * i) / segments;
            float x = std::cos(angle);
            float y = std::sin(angle);
            circleVertices.insert(circleVertices.end(), { x, y, 0.f, 0.f, 0.f, 1.f });
        }
        circleVertexCount = segments + 2;

        glGenVertexArrays(1, &VAO_circle);
        glGenBuffers(1, &VBO_circle);
        glBindVertexArray(VAO_circle);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_circle);
        glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // ----- Fullscreen background quad in NDC -----
        float bgVertices[] = {
          -1.0f,  1.0f,  0.0f, 1.0f,
          -1.0f, -1.0f,  0.0f, 0.0f,
           1.0f, -1.0f,  1.0f, 0.0f,
          -1.0f,  1.0f,  0.0f, 1.0f,
           1.0f, -1.0f,  1.0f, 0.0f,
           1.0f,  1.0f,  1.0f, 1.0f
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

        // --- Load background texture via your Resource_Manager (unchanged) ---
        Resource_Manager::loadAll("../../assets/Textures");
        bgTexture = Resource_Manager::resources_map["house"].handle;

        // ----- Shaders -----
        const char* bgVertexSrc =
            "#version 330 core\n"
            "layout (location = 0) in vec2 aPos;\n"
            "layout (location = 1) in vec2 aTexCoord;\n"
            "out vec2 TexCoord;\n"
            "void main(){gl_Position=vec4(aPos,0.0,1.0);TexCoord=aTexCoord;}\n";
        const char* bgFragmentSrc =
            "#version 330 core\n"
            "out vec4 FragColor;\n"
            "in vec2 TexCoord;\n"
            "uniform sampler2D backgroundTex;\n"
            "void main(){FragColor=texture(backgroundTex,TexCoord);} \n";
        bgShader = createShaderProgram(bgVertexSrc, bgFragmentSrc);

        const char* objVertexSrc =
            "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "layout (location = 1) in vec3 aColor;\n"
            "out vec3 vColor;\n"
            "uniform mat4 uMVP;\n"
            "void main(){gl_Position=uMVP*vec4(aPos,1.0);vColor=aColor;}\n";
        const char* objFragmentSrc =
            "#version 330 core\n"
            "in vec3 vColor;\n"
            "out vec4 FragColor;\n"
            "uniform vec4 uColor;\n"
            "void main(){FragColor=vec4(vColor,1.0)*uColor;}\n";
        objectShader = createShaderProgram(objVertexSrc, objFragmentSrc);

        initSpritePipeline();

        glBindVertexArray(0);
        glUseProgram(0);
    }

    // ---------------- Background ----------------
    void Graphics::renderBackground() {
        glUseProgram(bgShader);
        glBindTexture(GL_TEXTURE_2D, bgTexture);
        glBindVertexArray(VAO_bg);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    // ---------------- Rectangle ----------------
    void Graphics::renderRectangle(float posX, float posY, float rot,
        float scaleX, float scaleY,
        float r, float g, float b, float a) {
        glUseProgram(objectShader);

        // Minimal-change pivot fix:
        // - Keep SCALE behavior the same as before (about the local origin).
        // - Rotate around the rectangle's GEOMETRIC CENTER.
        //
        // Because your vertices are not centered at (0,0),
        // we translate by the "scaled pivot" before rotation, and translate back after,
        // so only the rotation center changes.
        //
        // Final order (glm appends on the right):  M = T(pos) * T(pivot_s) * R * T(-pivot_s) * S
        // Rightmost S acts first (same as your original T*R*S wrt S and R order),
        // but rotation is now about the scaled center pivot_s.
        const float pivot_sx = sRectPivotX * scaleX;  // scaled pivot X
        const float pivot_sy = sRectPivotY * scaleY;  // scaled pivot Y

        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(posX, posY, 0.0f));                  // world translation
        model = glm::translate(model, glm::vec3(pivot_sx, pivot_sy, 0.0f));          // move scaled center to origin (pre-rotation)
        model = glm::rotate(model, rot, glm::vec3(0, 0, 1));                         // rotate around center
        model = glm::translate(model, glm::vec3(-pivot_sx, -pivot_sy, 0.0f));        // move back
        model = glm::scale(model, glm::vec3(scaleX, scaleY, 1.0f));                  // same scaling behavior as before

        glUniformMatrix4fv(glGetUniformLocation(objectShader, "uMVP"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform4f(glGetUniformLocation(objectShader, "uColor"), r, g, b, a);

        glBindVertexArray(VAO_rect);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void Graphics::renderRectangle(float posX, float posY, float rot, float scale) {
        renderRectangle(posX, posY, rot, scale, scale, 1.f, 1.f, 1.f, 1.f);
    }

    // ---------------- Circle ----------------
    void Graphics::renderCircle(float posX, float posY, float radius,
        float r, float g, float b, float a) {
        glUseProgram(objectShader);
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(posX, posY, 0.0f));
        model = glm::scale(model, glm::vec3(radius, radius, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(objectShader, "uMVP"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform4f(glGetUniformLocation(objectShader, "uColor"), r, g, b, a);
        glBindVertexArray(VAO_circle);
        glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    // ---------------- Sprite ----------------
    void Graphics::renderSprite(unsigned int tex, float posX, float posY, float rot,
        float scaleX, float scaleY,
        float r, float g, float b, float a) {
        glUseProgram(spriteShader);
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(posX, posY, 0.0f));
        model = glm::rotate(model, rot, glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scaleX, scaleY, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(spriteShader, "uMVP"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform4f(glGetUniformLocation(spriteShader, "uTint"), r, g, b, a);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(glGetUniformLocation(spriteShader, "uTex"), 0);
        glBindVertexArray(VAO_sprite);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    // ---------------- Cleanup ----------------
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
        glDeleteVertexArrays(1, &VAO_sprite);
        glDeleteBuffers(1, &VBO_sprite);
        glDeleteBuffers(1, &EBO_sprite);
        glDeleteProgram(spriteShader);
    }

    // ---------------- Sprite pipeline ----------------
    void Graphics::initSpritePipeline() {
        float spriteVerts[] = {
          -0.5f, -0.5f, 0.0f, 0.f, 0.f,
           0.5f, -0.5f, 0.0f, 1.f, 0.f,
           0.5f,  0.5f, 0.0f, 1.f, 1.f,
          -0.5f,  0.5f, 0.0f, 0.f, 1.f
        };
        unsigned int idx[] = { 0,1,2, 2,3,0 };
        glGenVertexArrays(1, &VAO_sprite);
        glGenBuffers(1, &VBO_sprite);
        glGenBuffers(1, &EBO_sprite);
        glBindVertexArray(VAO_sprite);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_sprite);
        glBufferData(GL_ARRAY_BUFFER, sizeof(spriteVerts), spriteVerts, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_sprite);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        const char* vs =
            "#version 330 core\n"
            "layout(location=0) in vec3 aPos;\n"
            "layout(location=1) in vec2 aUV;\n"
            "uniform mat4 uMVP;\n"
            "out vec2 vUV;\n"
            "void main(){gl_Position=uMVP*vec4(aPos,1.0);vUV=aUV;}\n";
        const char* fs =
            "#version 330 core\n"
            "in vec2 vUV;\n"
            "out vec4 FragColor;\n"
            "uniform sampler2D uTex;\n"
            "uniform vec4 uTint;\n"
            "void main(){FragColor=texture(uTex,vUV)*uTint;}\n";
        spriteShader = createShaderProgram(vs, fs);
        glBindVertexArray(0);
    }

} // namespace gfx
