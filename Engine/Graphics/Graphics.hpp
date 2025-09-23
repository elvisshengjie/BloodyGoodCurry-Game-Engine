#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <glad/glad.h>
#include "../Resource_Manager/Resource_Manager.h"

namespace gfx {

    class Graphics {
    public:
        static unsigned int loadTexture(const char* path);
        static void initialize();
        static void renderBackground();

        // Rect: non-uniform scale + color
        static void renderRectangle(float posX, float posY, float rot,
            float scaleX, float scaleY,
            float r, float g, float b, float a);

        // Back-compat wrapper (uniform scale, white)
        static void renderRectangle(float posX, float posY, float rot, float scale);

        // NEW: Circle from components (position, radius, color)
        static void renderCircle(float posX, float posY, float radius,
            float r, float g, float b, float a);

        static void cleanup();

    private:
        // rectangle
        static unsigned int VAO_rect, VBO_rect;
        // circle (unit circle fan)
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
