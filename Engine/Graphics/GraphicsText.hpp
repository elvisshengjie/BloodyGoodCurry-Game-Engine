// Graphics/GraphicsText.hpp
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <map>
#include <string>

namespace gfx {

    struct Character {
        unsigned int TextureID;
        glm::ivec2   Size;
        glm::ivec2   Bearing;
        unsigned int Advance;
    };

    class TextRenderer {
    public:
        void initialize(const char* fontPath, unsigned int width, unsigned int height);
        void setViewport(unsigned int width, unsigned int height);
        void RenderText(std::string text, float x, float y, float scale, glm::vec3 color);
        void cleanup();

    private:
        unsigned int shaderID = 0;
        unsigned int VAO = 0;
        unsigned int VBO = 0;
        std::map<char, Character> Characters;
    };

} // namespace gfx
