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
		static void renderRectangle(float posX, float posY, float rot, float scaleX, float scaleY, float r, float g, float b, float a);
		static void renderRectangle(float posX, float posY, float rot, float scale);
		static void renderCircle(float posX, float posY, float radius, float r, float g, float b, float a);
		static void renderSprite(unsigned int tex, float posX, float posY, float rot, float scaleX, float scaleY, float r, float g, float b, float a);
		static void cleanup();

	private:
		static void initSpritePipeline();

		static unsigned int VAO_rect, VBO_rect;
		static unsigned int VAO_circle, VBO_circle;
		static int circleVertexCount;
		static unsigned int VAO_bg, VBO_bg, bgTexture;
		static unsigned int bgShader;
		static unsigned int objectShader;

		static unsigned int VAO_sprite, VBO_sprite, EBO_sprite;
		static unsigned int spriteShader;
	};

} // namespace gfx

#endif
