#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <glad/glad.h>

namespace gfx {

    class Graphics {
    public:
        static void initialize();
        static void renderBackground();
        static void renderRectangle(float posX, float posY, float rot, float scale);
        static void renderCircle();
        static void cleanup();

    private:
        // rectangle
        static unsigned int VAO_rect, VBO_rect;
        // circle
        static unsigned int VAO_circle, VBO_circle;
        static int circleVertexCount;
        // background
        static unsigned int VAO_bg, VBO_bg, bgTexture;
        static unsigned int bgShader;
        // object shader
        static unsigned int objectShader;
    };

} // namespace gfx

#endif
