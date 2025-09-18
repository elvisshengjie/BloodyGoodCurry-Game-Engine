#ifndef GRAPHICSTEXT_HPP
#define GRAPHICSTEXT_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <map>
#include <string>

namespace gfx {

    struct Character {
        unsigned int TextureID;
        glm::ivec2 Size;
        glm::ivec2 Bearing;
        unsigned int Advance;
    };

    class TextRenderer {
    public:
        void initialize(const char* fontPath, unsigned int width, unsigned int height);
        void RenderText(std::string text, float x, float y, float scale, glm::vec3 color);
        void cleanup();

    private:
        std::map<char, Character> Characters;
        unsigned int VAO, VBO;
        unsigned int shaderID;
    };

}

#endif
