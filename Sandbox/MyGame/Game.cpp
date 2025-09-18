// Sandbox/MyGame/Game.cpp
#include "../../Engine/Graphics/Window.hpp"
#include "Managers/SoundManager.h"
#include "Messaging_System/Messager_Bus.hpp"
#include "Audio_Tester.h"
#include "Game.hpp"

// use fixed screen size from JSON (same as your previous approach)
#include "Config/WindowConfig.h"

// math & GL helpers you split out
#include "MathUtils.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>


namespace mygame
{
    // ===== Persistent state =====
    static gfx::Window* gWin = nullptr;

    static int   gScreenW = 800;  // from window.json
    static int   gScreenH = 600;

    static GLuint gProg = 0;
    static GLint  gUMVP = -1;
    static GLint  gUColor = -1;
    static QuadGL gQuad;

    static float gPosX = 0.f, gPosY = 0.f;
    static float gRot = 0.f;
    static float gScale = 1.f;
    static constexpr float kBaseSize = 120.f;

    // audio state
    static std::array<bool, 10> gKeyEdge{}; // edge-trigger keys for audio
    static MessageBus busInstance;   // owned below

    // ---- Your previous audio helpers ----
    void initializeAudio();
    void cleanupAudio();

    // ------------------------------------------------------------
    // Init: called once by Core, receives the created Window
    // ------------------------------------------------------------
    void init(gfx::Window& win)
    {
        gWin = &win;

        // Load fixed size from JSON (same as before)
        WindowConfig cfg = LoadWindowConfig("../../Data_Files/window.json");
        gScreenW = cfg.width;
        gScreenH = cfg.height;

        // Audio bootstrap (new MessageBus flow)
        initializeAudio();
        startAudio(busInstance);

        // GL pipeline
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

        // Initialize Graphics
        gfx::Graphics::initialize();

        // Track key states to prevent multiple triggers
        static std::array<bool, 10> keysPressed = { false }, lastkeysPressed = { false };

        auto t_prev = Clock::now();
        while (!win.shouldClose()) {
            win.pollEvents();

            const auto t_now = Clock::now();
            const float dt = std::chrono::duration_cast<SecondsF>(t_now - t_prev).count();
            t_prev = t_now;

            handleAudioInput(win, keysPressed);

            // ---- Render ----
            win.beginFrame();

            gfx::Graphics::renderBackground();
            gfx::Graphics::renderRectangle();
            gfx::Graphics::renderCircle();

            win.endFrame();
            win.swapBuffers();

            lastkeysPressed = keysPressed;
        }

        // Cleanup
        gfx::Graphics::cleanup();
        cleanupAudio();
        std::cout << "Game ended." << std::endl;
    }

        // start centered (fixed, from JSON)
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
    // Update: called every frame by Core (no rendering here)
    // ------------------------------------------------------------
    void update(float dt)
    {
        handleAudioInput(*gWin, gKeyEdge, busInstance);
        // 2) 2D transform (hold-to-repeat, frame-rate independent)
        const float rotSpeed = DegToRad(90.f);
        const float scaleRate = 1.5f;
        const bool  shift = gWin->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
            gWin->isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
        const float accel = shift ? 3.f : 1.f;

        if (gWin->isKeyPressed(GLFW_KEY_Q)) gRot += rotSpeed * dt * accel;
        if (gWin->isKeyPressed(GLFW_KEY_E)) gRot -= rotSpeed * dt * accel;

        if (gRot > 3.14159265f) gRot -= 6.28318530f;
        if (gRot < -3.14159265f) gRot += 6.28318530f;

        if (gWin->isKeyPressed(GLFW_KEY_X)) gScale *= (1.f + scaleRate * dt * accel);
        if (gWin->isKeyPressed(GLFW_KEY_Z)) gScale *= (1.f - scaleRate * dt * accel);
        gScale = std::clamp(gScale, 0.25f, 4.0f);

        if (gWin->isKeyPressed(GLFW_KEY_R)) { gRot = 0.f; gScale = 1.f; }

        // keep centered using the fixed JSON size (same as before)
        gPosX = gScreenW * 0.5f;
        gPosY = gScreenH * 0.5f;
    }

    // ------------------------------------------------------------
    // draw: called every frame by Core between beginFrame/endFrame
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
    }

    // ------------------------------------------------------------
    // shutdown: called once by Core after the loop
    // ------------------------------------------------------------
    void shutdown()
    {
        cleanupAudio();

        if (gProg) glDeleteProgram(gProg);
        gQuad.destroy();

        gProg = 0;
        gUMVP = gUColor = -1;
        gWin = nullptr;
    }

} // namespace mygame
