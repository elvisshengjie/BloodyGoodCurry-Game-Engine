// Sandbox/MyGame/Game.cpp
#include "../../Engine/Graphics/Window.hpp"
#include "Game.hpp"

#include "Config/WindowConfig.h"   // 用来读取 window.json

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

namespace {
    // ======== 轻量 2D 矩阵工具（列主序，与 OpenGL 一致）========
    struct Mat4 { float m[16]{}; };

    Mat4 Identity() {
        Mat4 r{}; r.m[0] = 1; r.m[5] = 1; r.m[10] = 1; r.m[15] = 1; return r;
    }
    Mat4 Ortho(float l, float r, float b, float t, float zn = -1.f, float zf = 1.f) {
        Mat4 M{};
        M.m[0] = 2.f / (r - l);
        M.m[5] = 2.f / (t - b);
        M.m[10] = -2.f / (zf - zn);
        M.m[12] = -(r + l) / (r - l);
        M.m[13] = -(t + b) / (t - b);
        M.m[14] = -(zf + zn) / (zf - zn);
        M.m[15] = 1.f;
        return M;
    }
    Mat4 Mul(const Mat4& A, const Mat4& B) {
        Mat4 R{};
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                R.m[c * 4 + r] =
                A.m[0 * 4 + r] * B.m[c * 4 + 0] +
                A.m[1 * 4 + r] * B.m[c * 4 + 1] +
                A.m[2 * 4 + r] * B.m[c * 4 + 2] +
                A.m[3 * 4 + r] * B.m[c * 4 + 3];
        return R;
    }
    Mat4 Translate(float x, float y) {
        Mat4 T = Identity(); T.m[12] = x; T.m[13] = y; return T;
    }
    Mat4 Scale(float sx, float sy) {
        Mat4 S{}; S.m[0] = sx; S.m[5] = sy; S.m[10] = 1.f; S.m[15] = 1.f; return S;
    }
    Mat4 RotateZ(float rad) {
        Mat4 R = Identity();
        const float c = std::cos(rad), s = std::sin(rad);
        R.m[0] = c; R.m[4] = -s; R.m[1] = s; R.m[5] = c; return R;
    }

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
        glAttachShader(prog, vs); glAttachShader(prog, fs);
        glLinkProgram(prog);
        GLint ok = 0; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) {
            GLint len = 0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
            std::string log(len, '\0');
            glGetProgramInfoLog(prog, len, nullptr, log.data());
            std::cerr << "[Program link error]\n" << log << std::endl;
        }
        glDetachShader(prog, vs); glDetachShader(prog, fs);
        glDeleteShader(vs); glDeleteShader(fs);
        return prog;
    }
    struct QuadGL {
        GLuint vao = 0, vbo = 0, ebo = 0;
        void create() {
            const float verts[8] = { -0.5f,-0.5f,  0.5f,-0.5f,  0.5f,0.5f,  -0.5f,0.5f };
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
    float DegToRad(float d) { return d * 3.14159265358979323846f / 180.f; }
} // anonymous

// ======== MyGame 持久状态 ========
static gfx::Window* gWin = nullptr;

// 读取自 JSON 的固定窗口尺寸（与你“之前”的用法一致）
static int gScreenW = 800;
static int gScreenH = 600;

// GL 资源
static GLuint gProg = 0;
static GLint  gUMVP = -1;
static GLint  gUColor = -1;
static QuadGL gQuad;

// 2D 变换状态（旋转/缩放）
static float gPosX = 0.f, gPosY = 0.f;
static float gRot = 0.f;
static float gScale = 1.f;
static constexpr float kBaseSize = 120.f;

// 音频用的边沿触发键表
static bool gKeyEdge[10] = { false };

namespace mygame {

    void initializeAudio() {
        std::cout << "Initializing audio system..." << std::endl;
        if (!SoundManager::getInstance().initialize()) {
            std::cerr << "Failed to initialize sound system!" << std::endl;
            return;
        }
        SoundManager::getInstance().setMasterVolume(0.7f);

        // 预加载音效
        SoundManager::getInstance().loadSound("coin", "badge-coin-win-14675.mp3", false);
        SoundManager::getInstance().loadSound("footsteps", "footsteps-male.mp3", true);
        SoundManager::getInstance().loadSound("level_win", "level-win.mp3", false);
        SoundManager::getInstance().loadSound("lose", "losing-horn.mp3", false);
        SoundManager::getInstance().loadSound("click", "mouse-click.mp3", false);
        SoundManager::getInstance().loadSound("win", "win.mp3", false);
    }
    void cleanupAudio() {
        SoundManager::getInstance().shutdown();
    }

    void init(gfx::Window& win) {
        gWin = &win;

        // —— 读取 JSON，固定使用 cfg 宽高（和你之前一模一样）——
        WindowConfig cfg = LoadWindowConfig("../../Data_Files/window.json");
        gScreenW = cfg.width;
        gScreenH = cfg.height;

        initializeAudio();

        // 着色器 + 几何体
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

        // 初始位置：用 cfg 居中（固定不变）
        gPosX = static_cast<float>(gScreenW) * 0.5f;
        gPosY = static_cast<float>(gScreenH) * 0.5f;
        gRot = 0.f; gScale = 1.f;

        std::cout << "\n=== Controls ===\n"
            << "1: coin | 2: toggle footsteps | 3: level win | 4: lose | 5: click | 6: win\n"
            << "M: toggle master volume | S: stop all | ESC handled by window\n"
            << "Q/E: rotate CCW/CW | Z/X: scale down/up | SHIFT accelerate | R reset\n"
            << "=======================================\n";
    }

    void update(float dt) {
        // ---- 音频（边沿） ----
        auto keyDown = [&](int key, int idx) {
            if (gWin->isKeyPressed(key) && !gKeyEdge[idx]) { gKeyEdge[idx] = true; return true; }
            if (!gWin->isKeyPressed(key)) gKeyEdge[idx] = false;
            return false;
            };

        if (keyDown(GLFW_KEY_1, 1)) SoundManager::getInstance().playSound("coin", 0.8f);
        if (keyDown(GLFW_KEY_2, 2)) {
            if (SoundManager::getInstance().isSoundPlaying("footsteps"))
                SoundManager::getInstance().stopSound("footsteps");
            else
                SoundManager::getInstance().playSound("footsteps", 0.6f);
        }
        if (keyDown(GLFW_KEY_3, 3)) SoundManager::getInstance().playSound("level_win", 0.9f);
        if (keyDown(GLFW_KEY_4, 4)) SoundManager::getInstance().playSound("lose", 0.8f);
        if (keyDown(GLFW_KEY_5, 5)) SoundManager::getInstance().playSound("click", 0.7f);
        if (keyDown(GLFW_KEY_6, 6)) SoundManager::getInstance().playSound("win", 0.9f);
        if (keyDown(GLFW_KEY_M, 7)) {
            static float vol = 0.7f;
            vol = (vol > 0.5f) ? 0.2f : 0.7f;
            SoundManager::getInstance().setMasterVolume(vol);
            std::cout << "Master volume: " << vol << "\n";
        }
        if (keyDown(GLFW_KEY_S, 8)) SoundManager::getInstance().stopAllSounds();

        // ---- 2D 变换（长按，帧率无关）----
        const float rotSpeed = DegToRad(90.f);
        const float scaleRate = 1.5f;
        const bool shift = gWin->isKeyPressed(GLFW_KEY_LEFT_SHIFT) || gWin->isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
        const float accel = shift ? 3.f : 1.f;

        if (gWin->isKeyPressed(GLFW_KEY_Q)) gRot += rotSpeed * dt * accel;
        if (gWin->isKeyPressed(GLFW_KEY_E)) gRot -= rotSpeed * dt * accel;
        if (gRot > 3.14159265f)  gRot -= 2.f * 3.14159265f;
        if (gRot < -3.14159265f) gRot += 2.f * 3.14159265f;

        if (gWin->isKeyPressed(GLFW_KEY_X)) gScale *= (1.f + scaleRate * dt * accel);
        if (gWin->isKeyPressed(GLFW_KEY_Z)) gScale *= (1.f - scaleRate * dt * accel);
        gScale = std::clamp(gScale, 0.25f, 4.0f);

        if (gWin->isKeyPressed(GLFW_KEY_R)) { gRot = 0.f; gScale = 1.f; }

        // 中心固定为 cfg 的中心（与之前一致，不随窗口改变）
        gPosX = static_cast<float>(gScreenW) * 0.5f;
        gPosY = static_cast<float>(gScreenH) * 0.5f;
    }

    void draw() {
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

    void shutdown() {
        cleanupAudio();
        if (gProg) glDeleteProgram(gProg);
        gQuad.destroy();
        gProg = 0; gUMVP = gUColor = -1;
        gWin = nullptr;
    }

} // namespace mygame
