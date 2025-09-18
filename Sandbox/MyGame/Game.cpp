// Sandbox/MyGame/Game.cpp
#include "../../Engine/Graphics/Window.hpp"
#include "Game.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <iostream>

// ---- small math helpers for 2D transforms (column-major, OpenGL style) ----
namespace {
    struct Mat4 {
        float m[16];
    };

    Mat4 Identity() {
        Mat4 r{};
        r.m[0] = 1; r.m[5] = 1; r.m[10] = 1; r.m[15] = 1;
        return r;
    }

    Mat4 Ortho(float l, float r, float b, float t, float zn = -1.0f, float zf = 1.0f) {
        Mat4 M{};
        M.m[0] = 2.0f / (r - l);
        M.m[5] = 2.0f / (t - b);
        M.m[10] = -2.0f / (zf - zn);
        M.m[12] = -(r + l) / (r - l);
        M.m[13] = -(t + b) / (t - b);
        M.m[14] = -(zf + zn) / (zf - zn);
        M.m[15] = 1.0f;
        return M;
    }

    Mat4 Mul(const Mat4& A, const Mat4& B) {
        Mat4 R{};
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                R.m[c * 4 + r] =
                    A.m[0 * 4 + r] * B.m[c * 4 + 0] +
                    A.m[1 * 4 + r] * B.m[c * 4 + 1] +
                    A.m[2 * 4 + r] * B.m[c * 4 + 2] +
                    A.m[3 * 4 + r] * B.m[c * 4 + 3];
            }
        }
        return R;
    }

    Mat4 Translate(float x, float y) {
        Mat4 T = Identity();
        T.m[12] = x;
        T.m[13] = y;
        return T;
    }

    Mat4 Scale(float sx, float sy) {
        Mat4 S{};
        S.m[0] = sx; S.m[5] = sy; S.m[10] = 1.0f; S.m[15] = 1.0f;
        return S;
    }

    Mat4 RotateZ(float rad) {
        Mat4 R = Identity();
        const float c = std::cos(rad);
        const float s = std::sin(rad);
        R.m[0] = c; R.m[4] = -s;
        R.m[1] = s; R.m[5] = c;
        return R;
    }

    float DegToRad(float deg) { return deg * 3.14159265358979323846f / 180.0f; }

    // ---- minimal GL helpers: compile shaders, create quad ----
    GLuint Compile(GLenum type, const char* src) {
        GLuint sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        GLint ok = 0; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            GLint len = 0; glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
            std::string log(len, '\0');
            glGetShaderInfoLog(sh, len, nullptr, log.data());
            std::cerr << "[Shader compile error]\n" << log << std::endl;
        }
        return sh;
    }

    GLuint Link(GLuint vs, GLuint fs) {
        GLuint prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);
        GLint ok = 0; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) {
            GLint len = 0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
            std::string log(len, '\0');
            glGetProgramInfoLog(prog, len, nullptr, log.data());
            std::cerr << "[Program link error]\n" << log << std::endl;
        }
        glDetachShader(prog, vs);
        glDetachShader(prog, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
        return prog;
    }

    struct QuadGL {
        GLuint vao = 0, vbo = 0, ebo = 0;
        void create() {
            // A unit quad centered at origin (pivot at center).
            const float verts[8] = {
                -0.5f, -0.5f,
                 0.5f, -0.5f,
                 0.5f,  0.5f,
                -0.5f,  0.5f
            };
            const unsigned short idx[6] = { 0,1,2, 2,3,0 };

            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);
            glBindVertexArray(vao);

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

            glBindVertexArray(0);
        }
        void destroy() {
            if (ebo) glDeleteBuffers(1, &ebo);
            if (vbo) glDeleteBuffers(1, &vbo);
            if (vao) glDeleteVertexArrays(1, &vao);
            vao = vbo = ebo = 0;
        }
    };
} // anonymous namespace

using Clock = std::chrono::steady_clock;
using SecondsF = std::chrono::duration<float>;

namespace mygame {

    void initializeAudio()
    {
        std::cout << "Initializing audio system..." << std::endl;
        if (!SoundManager::getInstance().initialize()) {
            std::cerr << "Failed to initialize sound system!" << std::endl;
            return;
        }
        SoundManager::getInstance().setMasterVolume(0.7f);
        std::cout << "Loading audio files..." << std::endl;
        if (SoundManager::getInstance().loadSound("coin", "badge-coin-win-14675.mp3", false))
            std::cout << "Loaded: badge-coin-win-14675.mp3 as 'coin'\n";
        if (SoundManager::getInstance().loadSound("footsteps", "footsteps-male.mp3", true))
            std::cout << "Loaded: footsteps-male.mp3 as 'footsteps' (looping)\n";
        if (SoundManager::getInstance().loadSound("level_win", "level-win.mp3", false))
            std::cout << "Loaded: level-win.mp3 as 'level_win'\n";
        if (SoundManager::getInstance().loadSound("lose", "losing-horn.mp3", false))
            std::cout << "Loaded: losing-horn.mp3 as 'lose'\n";
        if (SoundManager::getInstance().loadSound("click", "mouse-click.mp3", false))
            std::cout << "Loaded: mouse-click.mp3 as 'click'\n";
        if (SoundManager::getInstance().loadSound("win", "win.mp3", false))
            std::cout << "Loaded: win.mp3 as 'win'\n";
        std::cout << "Audio system initialized successfully!" << std::endl;
    }

    void cleanupAudio() {
        std::cout << "Cleaning up audio system..." << std::endl;
        SoundManager::getInstance().shutdown();
    }

    void run()
    {
        std::cout << "Starting MyGame with Sound + 2D Transform Demo..." << std::endl;

        // ---- audio ----
        initializeAudio();

        // ---- window ----
        WindowConfig cfg = LoadWindowConfig("../../Data_Files/window.json");
        gfx::Window win(cfg.width, cfg.height, cfg.title.c_str());

        std::cout << "\n=== Audio Controls ===\n"
            << "1: coin | 2: toggle footsteps | 3: level win | 4: lose | 5: click | 6: win\n"
            << "M: toggle master volume | S: stop all | ESC: quit\n";
        std::cout << "=== 2D Transform Controls ===\n"
            << "Q/E: rotate CCW/CW | Z/X: scale down/up | SHIFT: accelerate | R: reset\n"
            << "=================================\n";

        // ---- track key edges for audio toggles only ----
        static bool keysPressed[10] = { false };

        // ---- GL: minimal pipeline for drawing a centered quad we can rotate/scale ----
        const char* kVS = R"(#version 330 core
            layout(location=0) in vec2 aPos;
            uniform mat4 uMVP;
            void main() { gl_Position = uMVP * vec4(aPos, 0.0, 1.0); }
        )";
        const char* kFS = R"(#version 330 core
            out vec4 FragColor;
            uniform vec3 uColor;
            void main() { FragColor = vec4(uColor, 1.0); }
        )";
        GLuint vs = Compile(GL_VERTEX_SHADER, kVS);
        GLuint fs = Compile(GL_FRAGMENT_SHADER, kFS);
        GLuint prog = Link(vs, fs);
        GLint uMVP = glGetUniformLocation(prog, "uMVP");
        GLint uColor = glGetUniformLocation(prog, "uColor");

        QuadGL quad; quad.create();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // ---- transform state we control at runtime ----
        float posX = cfg.width * 0.5f;   // screen center
        float posY = cfg.height * 0.5f;
        float rotationRad = 0.0f;         // rotation around center
        float scale = 1.0f;               // uniform scale
        const float baseSize = 120.0f;    // base size of the quad in pixels

        // ---- timing ----
        using Clock = std::chrono::steady_clock;
        using SecondsF = std::chrono::duration<float>;
        auto t_prev = Clock::now();

        // ---- main loop ----
        while (!win.shouldClose()) {
            win.pollEvents();

            // dt (seconds)
            const auto t_now = Clock::now();
            const float dt = std::chrono::duration_cast<SecondsF>(t_now - t_prev).count();
            t_prev = t_now;

            // ----- AUDIO hotkeys (edge-triggered) -----
            if (win.isKeyPressed(GLFW_KEY_1) && !keysPressed[1]) {
                SoundManager::getInstance().playSound("coin", 0.8f);
                std::cout << "Playing coin sound!\n";
                keysPressed[1] = true;
            }
            else if (!win.isKeyPressed(GLFW_KEY_1)) keysPressed[1] = false;

            if (win.isKeyPressed(GLFW_KEY_2) && !keysPressed[2]) {
                if (SoundManager::getInstance().isSoundPlaying("footsteps")) {
                    SoundManager::getInstance().stopSound("footsteps");
                    std::cout << "Stopped footsteps\n";
                }
                else {
                    SoundManager::getInstance().playSound("footsteps", 0.6f);
                    std::cout << "Started footsteps (looping)\n";
                }
                keysPressed[2] = true;
            }
            else if (!win.isKeyPressed(GLFW_KEY_2)) keysPressed[2] = false;

            if (win.isKeyPressed(GLFW_KEY_3) && !keysPressed[3]) {
                SoundManager::getInstance().playSound("level_win", 0.9f);
                std::cout << "Playing level win sound!\n";
                keysPressed[3] = true;
            }
            else if (!win.isKeyPressed(GLFW_KEY_3)) keysPressed[3] = false;

            if (win.isKeyPressed(GLFW_KEY_4) && !keysPressed[4]) {
                SoundManager::getInstance().playSound("lose", 0.8f);
                std::cout << "Playing losing horn!\n";
                keysPressed[4] = true;
            }
            else if (!win.isKeyPressed(GLFW_KEY_4)) keysPressed[4] = false;

            if (win.isKeyPressed(GLFW_KEY_5) && !keysPressed[5]) {
                SoundManager::getInstance().playSound("click", 0.7f);
                std::cout << "Playing mouse click!\n";
                keysPressed[5] = true;
            }
            else if (!win.isKeyPressed(GLFW_KEY_5)) keysPressed[5] = false;

            if (win.isKeyPressed(GLFW_KEY_6) && !keysPressed[6]) {
                SoundManager::getInstance().playSound("win", 0.9f);
                std::cout << "Playing win sound!\n";
                keysPressed[6] = true;
            }
            else if (!win.isKeyPressed(GLFW_KEY_6)) keysPressed[6] = false;

            if (win.isKeyPressed(GLFW_KEY_M) && !keysPressed[7]) {
                static float currentVolume = 0.7f;
                currentVolume = (currentVolume > 0.5f) ? 0.2f : 0.7f;
                SoundManager::getInstance().setMasterVolume(currentVolume);
                std::cout << "Master volume set to: " << currentVolume << "\n";
                keysPressed[7] = true;
            }
            else if (!win.isKeyPressed(GLFW_KEY_M)) keysPressed[7] = false;

            if (win.isKeyPressed(GLFW_KEY_S) && !keysPressed[8]) {
                SoundManager::getInstance().stopAllSounds();
                std::cout << "Stopped all sounds!\n";
                keysPressed[8] = true;
            }
            else if (!win.isKeyPressed(GLFW_KEY_S)) keysPressed[8] = false;

            // ----- 2D TRANSFORM controls (level-triggered, frame-rate independent) -----
            const float rotSpeed = DegToRad(90.0f); // 90 degrees per second
            const float scaleRate = 1.5f;            // 150% per second
            const bool shiftHeld = win.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || win.isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
            const float accel = shiftHeld ? 3.0f : 1.0f;

            // Rotate: Q -> CCW, E -> CW
            if (win.isKeyPressed(GLFW_KEY_Q)) rotationRad += rotSpeed * dt * accel;
            if (win.isKeyPressed(GLFW_KEY_E)) rotationRad -= rotSpeed * dt * accel;

            // Keep rotation inside [-pi, pi] (optional cosmetic)
            if (rotationRad > 3.14159265f) rotationRad -= 2.0f * 3.14159265f;
            if (rotationRad < -3.14159265f) rotationRad += 2.0f * 3.14159265f;

            // Scale: X up, Z down (uniform)
            if (win.isKeyPressed(GLFW_KEY_X)) scale *= (1.0f + scaleRate * dt * accel);
            if (win.isKeyPressed(GLFW_KEY_Z)) scale *= (1.0f - scaleRate * dt * accel);
            scale = std::clamp(scale, 0.25f, 4.0f);

            // Reset
            if (win.isKeyPressed(GLFW_KEY_R)) {
                rotationRad = 0.0f;
                scale = 1.0f;
            }

            // ----- RENDER -----
            win.beginFrame();
            // Clear (your Window may already clear; safe to do again if desired)
            // glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
            // glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(prog);
            glBindVertexArray(quad.vao);

            // MVP = Ortho * (Translate * Rotate * Scale)
            const Mat4 P = Ortho(0.0f, (float)cfg.width, 0.0f, (float)cfg.height);
            const Mat4 M = Mul(Translate(posX, posY), Mul(RotateZ(rotationRad), Scale(baseSize * scale, baseSize * scale)));
            const Mat4 MVP = Mul(P, M);

            glUniformMatrix4fv(uMVP, 1, GL_FALSE, MVP.m);
            glUniform3f(uColor, 0.95f, 0.75f, 0.25f); // golden-ish rectangle

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);
            glUseProgram(0);

            win.endFrame();
            win.swapBuffers();
        }

        // ---- cleanup ----
        cleanupAudio();
        // GL resources
        // (If your Window destroys the GL context after this, deleting is recommended but optional.)
        glDeleteProgram(prog);
        quad.destroy();

        std::cout << "Game ended." << std::endl;
    }

} // namespace mygame
