#include "Graphics.hpp"
#include <vector>
#include <cmath>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdexcept>

namespace gfx {

    constexpr float PI = 3.14159265359f;

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
    unsigned int Graphics::VAO_sprite = 0;
    unsigned int Graphics::VBO_sprite = 0;
    unsigned int Graphics::EBO_sprite = 0;
    unsigned int Graphics::spriteShader = 0;

    static int segments = 50;

    // Cache rectangle local-space geometric center (pivot).
    // Computed in initialize() from current rect vertex data.
    static float sRectPivotX = 0.0f;
    static float sRectPivotY = 0.0f;

    static inline void GL_THROW_IF_ERROR(const char* where) {
        GLenum e = glGetError();
        if (e != GL_NO_ERROR) throw std::runtime_error(std::string(where) + "|gl_error=" + std::to_string((int)e));
    }

    static unsigned int compileShader(const char* source, GLenum type) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        int success; char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
            throw std::runtime_error(std::string(type == GL_VERTEX_SHADER ? "compile_vs" : "compile_fs") + "|" + infoLog);
        }
        GL_THROW_IF_ERROR("compileShader");
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
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            glDeleteProgram(program);
            throw std::runtime_error(std::string("link_program|") + infoLog);
        }
        glDeleteShader(vertex);
        glDeleteShader(fragment);

        glValidateProgram(program);
        int validated = 0;
        glGetProgramiv(program, GL_VALIDATE_STATUS, &validated);
        if (!validated) {
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            glDeleteProgram(program);
            throw std::runtime_error(std::string("validate_program|") + infoLog);
        }
        GL_THROW_IF_ERROR("createShaderProgram");
        return program;
    }

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
            glBindTexture(GL_TEXTURE_2D, 0);
            if (textureID) glDeleteTextures(1, &textureID);
            throw std::runtime_error(std::string("texture_load|failed|") + path);
        }
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_THROW_IF_ERROR("loadTexture");
        return textureID;
    }

    void Graphics::initialize() {
        float rectVertices[] = {
          -0.3f, -0.4f, 0.0f,  1.0f, 0.0f, 0.0f,
           0.3f, -0.4f, 0.0f,  0.0f, 1.0f, 0.0f,
           0.3f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
          -0.3f,  0.0f, 0.0f,  1.0f, 1.0f, 0.0f
        };
        unsigned int rectIndices[] = { 0,1,2, 2,3,0 };
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

        // Compute local-space geometric center (pivot)
        sRectPivotX = (rectVertices[0] + rectVertices[6] + rectVertices[12] + rectVertices[18]) * 0.25f;
        sRectPivotY = (rectVertices[1] + rectVertices[7] + rectVertices[13] + rectVertices[19]) * 0.25f;

        std::vector<float> circleVertices;
        circleVertices.reserve((segments + 2) * 6);
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

        // --- Load background texture ---
        Resource_Manager::loadAll("../../assets/Textures");

        // Use the string ID directly
        bgTexture = Resource_Manager::resources_map["house"].handle;
        if (!bgTexture) throw std::runtime_error("bg_texture|missing|house");

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

        GL_THROW_IF_ERROR("initialize_end");
    }

    void Graphics::renderBackground() {
        glUseProgram(bgShader);
        glActiveTexture(GL_TEXTURE0);
        int loc = glGetUniformLocation(bgShader, "backgroundTex");
        if (loc < 0) throw std::runtime_error("renderBackground|uniform_missing|backgroundTex");
        glUniform1i(loc, 0);
        glBindTexture(GL_TEXTURE_2D, bgTexture);
        glBindVertexArray(VAO_bg);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        GL_THROW_IF_ERROR("renderBackground");
    }

    void Graphics::renderRectangle(float posX, float posY, float rot, float scaleX, float scaleY, float r, float g, float b, float a) {
        glUseProgram(objectShader);

        // Rotate around the rectangle's geometric center (pivot).
        // Because scale happens first (rightmost), rotate around the "scaled pivot".
        const float pivot_sx = sRectPivotX * scaleX;
        const float pivot_sy = sRectPivotY * scaleY;

        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(posX, posY, 0.0f));
        model = glm::translate(model, glm::vec3(pivot_sx, pivot_sy, 0.0f));
        model = glm::rotate(model, rot, glm::vec3(0, 0, 1));
        model = glm::translate(model, glm::vec3(-pivot_sx, -pivot_sy, 0.0f));
        model = glm::scale(model, glm::vec3(scaleX, scaleY, 1.0f));

        glUniformMatrix4fv(glGetUniformLocation(objectShader, "uMVP"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform4f(glGetUniformLocation(objectShader, "uColor"), r, g, b, a);
        glBindVertexArray(VAO_rect);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);
        GL_THROW_IF_ERROR("renderRectangle");
    }

    void Graphics::renderRectangle(float posX, float posY, float rot, float scale) {
        renderRectangle(posX, posY, rot, scale, scale, 1.f, 1.f, 1.f, 1.f);
    }

    void Graphics::renderCircle(float posX, float posY, float radius, float r, float g, float b, float a) {
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
        GL_THROW_IF_ERROR("renderCircle");
    }

    // Draw whole texture (backward-compatible)
    void Graphics::renderSprite(unsigned int tex, float posX, float posY, float rot, float scaleX, float scaleY, float r, float g, float b, float a) {
        glUseProgram(spriteShader);
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(posX, posY, 0.0f));
        model = glm::rotate(model, rot, glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scaleX, scaleY, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(spriteShader, "uMVP"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform4f(glGetUniformLocation(spriteShader, "uTint"), r, g, b, a);

        // Whole-texture UVs
        glUniform2f(glGetUniformLocation(spriteShader, "uUVOffset"), 0.0f, 0.0f);
        glUniform2f(glGetUniformLocation(spriteShader, "uUVScale"), 1.0f, 1.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(glGetUniformLocation(spriteShader, "uTex"), 0);
        glBindVertexArray(VAO_sprite);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        GL_THROW_IF_ERROR("renderSprite");
    }

    // Draw a single sub-rect frame from a sprite sheet laid out in cols x rows.
    void Graphics::renderSpriteFrame(unsigned int tex,
        float posX, float posY, float rot, float scaleX, float scaleY,
        int frameIndex, int cols, int rows,
        float r, float g, float b, float a)
    {
        glUseProgram(spriteShader);

        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(posX, posY, 0.0f));
        model = glm::rotate(model, rot, glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scaleX, scaleY, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(spriteShader, "uMVP"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform4f(glGetUniformLocation(spriteShader, "uTint"), r, g, b, a);

        if (cols <= 0) cols = 1;
        if (rows <= 0) rows = 1;

        const float sx = 1.0f / static_cast<float>(cols);
        const float sy = 1.0f / static_cast<float>(rows);
        const int c = frameIndex % cols;
        const int rIdx = frameIndex / cols;

        // stbi flips Y => (0,0) is bottom-left; single-row sheets work with offY = rIdx*sy
        const float offX = c * sx;
        const float offY = rIdx * sy;

        glUniform2f(glGetUniformLocation(spriteShader, "uUVOffset"), offX, offY);
        glUniform2f(glGetUniformLocation(spriteShader, "uUVScale"), sx, sy);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(glGetUniformLocation(spriteShader, "uTex"), 0);

        glBindVertexArray(VAO_sprite);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        GL_THROW_IF_ERROR("renderSpriteFrame");
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
        glDeleteVertexArrays(1, &VAO_sprite);
        glDeleteBuffers(1, &VBO_sprite);
        glDeleteBuffers(1, &EBO_sprite);
        glDeleteProgram(spriteShader);
    }

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

        // NOTE: added uUVOffset/uUVScale to support sub-UV drawing
        const char* vs =
            "#version 330 core\n"
            "layout(location=0) in vec3 aPos;\n"
            "layout(location=1) in vec2 aUV;\n"
            "uniform mat4 uMVP;\n"
            "uniform vec2 uUVOffset;\n"
            "uniform vec2 uUVScale;\n"
            "out vec2 vUV;\n"
            "void main(){\n"
            "  gl_Position = uMVP * vec4(aPos,1.0);\n"
            "  vUV = aUV * uUVScale + uUVOffset;\n"
            "}\n";
        const char* fs =
            "#version 330 core\n"
            "in vec2 vUV;\n"
            "out vec4 FragColor;\n"
            "uniform sampler2D uTex;\n"
            "uniform vec4 uTint;\n"
            "void main(){ FragColor = texture(uTex, vUV) * uTint; }\n";
        spriteShader = createShaderProgram(vs, fs);
        glBindVertexArray(0);
        GL_THROW_IF_ERROR("initSpritePipeline");
    }

    // Test hooks to intentionally break graphics for crash verification
    void Graphics::testCrash(int which) {
        if (which == 1) { bgShader = 0; }
        else if (which == 2) { VAO_bg = 0; }
        else if (which == 3) { spriteShader = 0; }
        else if (which == 4) { objectShader = 0; }
        else if (which == 5) { if (bgTexture) { glDeleteTextures(1, &bgTexture); bgTexture = 0; } }
    }

} // namespace gfx
