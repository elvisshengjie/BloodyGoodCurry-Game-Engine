// Sandbox/MyGame/Game.cpp
#include "../../Engine/Graphics/Window.hpp"
#include "Managers/SoundManager.h"
#include "Messaging_System/Messager_Bus.hpp"
#include "Audio_Tester.h"
#include "Game.hpp"

// use fixed screen size from JSON
#include "Config/WindowConfig.h"

// math & GL helpers
#include "MathUtils.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

#include <Graphics/Graphics.hpp>

namespace mygame
{


    // ===== Persistent state =====
    static gfx::Window* gWin = nullptr;

    static int   gScreenW = 800;
    static int   gScreenH = 600;

    static GLuint gProg = 0;
    static GLint  gUMVP = -1;
    static GLint  gUColor = -1;
    static QuadGL gQuad;

    static float gPosX = 0.f, gPosY = 0.f;
    static float gRot = 0.f;
    static float gScale = 1.f;
    static constexpr float kBaseSize = 120.f;

    static float rectPosX = 0.f;
    static float rectPosY = 0.f;
    static float rectRot = 0.f;
    static float rectScale = 1.f;

    // audio state
    static std::array<bool, 10> gKeyEdge{};
    static MessageBus busInstance;

    // ------------------------------------------------------------
    // Init: called once by Core, receives the created Window
    // ------------------------------------------------------------
    void init(gfx::Window& win)
    {
        gWin = &win;

        // Load fixed size from JSON
        WindowConfig cfg = LoadWindowConfig("../../Data_Files/window.json");
        gScreenW = cfg.width;
        gScreenH = cfg.height;

        // Audio bootstrap
        initializeAudio();
        startAudio(busInstance);

        // --- OpenGL setup ---
        const char* kVS = R"(#version 330 core
            layout(location=0) in vec2 aPos;
            uniform mat4 uMVP;
            void main(){ gl_Position = uMVP * vec4(aPos,0.0,1.0); }
        )";
        const char* kFS = R"(#version 330 core
            out vec4 FragColor;
            uniform vec3 uColor;
            void main(){ FragColor = vec4(uColor,1.0); }
        )";
        GLuint vs = Compile(GL_VERTEX_SHADER, kVS);
        GLuint fs = Compile(GL_FRAGMENT_SHADER, kFS);
        gProg = Link(vs, fs);
        gUMVP = glGetUniformLocation(gProg, "uMVP");
        gUColor = glGetUniformLocation(gProg, "uColor");

        gQuad.create();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Initialize Graphics system
        gfx::Graphics::initialize();

        // start centered
        gPosX = gScreenW * 0.5f;
        gPosY = gScreenH * 0.5f;
        gRot = 0.f;
        gScale = 1.f;

        std::cout << "\n=== Controls ===\n"
            << "1: coin | 2: toggle footsteps | 3: level win | 4: lose | 5: click | 6: win\n"
            << "M: toggle master volume | S: stop all | ESC handled by window\n"
            << "Q/E: rotate CCW/CW | Z/X: scale down/up | SHIFT accelerate | R reset\n"
            << "=======================================\n";
    }

    // ------------------------------------------------------------
    // Update: called every frame
    // ------------------------------------------------------------
    void update(float dt)
    {
        handleAudioInput(*gWin, gKeyEdge, busInstance);

        const float rotSpeed = DegToRad(90.f);
        const float scaleRate = 1.5f;
        const bool  shift = gWin->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
            gWin->isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
        const float accel = shift ? 3.f : 1.f;

        // rotate the RECTANGLE (Q/E)
        if (gWin->isKeyPressed(GLFW_KEY_Q)) rectRot += rotSpeed * dt * accel;
        if (gWin->isKeyPressed(GLFW_KEY_E)) rectRot -= rotSpeed * dt * accel;

        // keep angle reasonable
        if (rectRot > 3.14159265f)  rectRot -= 6.28318530f;
        if (rectRot < -3.14159265f) rectRot += 6.28318530f;

        // scale the RECTANGLE (Z/X)
        if (gWin->isKeyPressed(GLFW_KEY_X)) rectScale *= (1.f + scaleRate * dt * accel);
        if (gWin->isKeyPressed(GLFW_KEY_Z)) rectScale *= (1.f - scaleRate * dt * accel);
        rectScale = std::clamp(rectScale, 0.25f, 4.0f);

        // reset (R)
        if (gWin->isKeyPressed(GLFW_KEY_R)) { rectRot = 0.f; rectScale = 1.f; }

        // the old QuadGL demo can keep using gRot/gScale if you want,
        // but they no longer control the Graphics rectangle.
    }


    // ------------------------------------------------------------
    // Draw: called every frame
    // ------------------------------------------------------------
    void draw()
    {
        glUseProgram(gProg);
        glBindVertexArray(gQuad.vao);

        const float W = static_cast<float>(gScreenW);
        const float H = static_cast<float>(gScreenH);

        const Mat4 P = Ortho(0.f, W, 0.f, H);
        const Mat4 M = Mul(Translate(gPosX, gPosY),
            Mul(RotateZ(gRot), Scale(kBaseSize * gScale, kBaseSize * gScale)));
        const Mat4 MVP = Mul(P, M);

        glUniformMatrix4fv(gUMVP, 1, GL_FALSE, MVP.m);
        glUniform3f(gUColor, 0.95f, 0.75f, 0.25f);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

        glBindVertexArray(0);
        glUseProgram(0);

        // Draw extra graphics (your background + shapes)
        gfx::Graphics::renderBackground();
        // draw rotating/scaling rectangle
        gfx::Graphics::renderRectangle(rectPosX, rectPosY, rectRot, rectScale);

        // draw circle (no rotation/scale)
        gfx::Graphics::renderCircle();
    }

    // ------------------------------------------------------------
    // Shutdown: called once after loop
    // ------------------------------------------------------------
    void shutdown()
    {
        gfx::Graphics::cleanup();
        cleanupAudio();

        if (gProg) glDeleteProgram(gProg);
        gQuad.destroy();

        gProg = 0;
        gUMVP = gUColor = -1;
        gWin = nullptr;

        std::cout << "Game ended." << std::endl;
    }

} // namespace mygame
