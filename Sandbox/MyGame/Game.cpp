// Sandbox/MyGame/Game.cpp
#include "../../Engine/Graphics/Window.hpp"
#include "Audio/SoundManager.h"
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
#include <Serialization/JsonSerialization.h>
#include "Factory/Factory.h"
#include "Component/TestComponent.h"
#include "Component/TransformComponent.h"
#include "Component/RenderComponent.h"
#include "Component/CircleRenderComponent.h"

namespace mygame
{
    // ===== Persistent state =====
    static gfx::Window* gWin = nullptr;

    static int   gScreenW = 800;
    static int   gScreenH = 600;

    // Demo quad (legacy small sample)
    static GLuint gProg = 0;
    static GLint  gUMVP = -1;
    static GLint  gUColor = -1;
    static QuadGL gQuad;

    static float gPosX = 0.f, gPosY = 0.f; // for demo quad only
    static float gRot = 0.f;
    static float gScale = 1.f;
    static constexpr float kBaseSize = 120.f;

    // audio state
    static std::array<bool, 10> gKeyEdge{};
    static MessageBus busInstance;

    // component system
    static std::unique_ptr<Framework::GameObjectFactory> sFactory;
    static Framework::GOC* sTestObj = nullptr;  // owned by the factory
    static Framework::GOC* sTestObj2 = nullptr;  // owned by the factory
    static Framework::GOC* sCircleObj = nullptr;  // owned by the factory

    // scale control for sTestObj's RenderComponent (Z/X & R keys)
    static float gRectScale = 1.0f;
    static float gRectBaseW = 1.0f, gRectBaseH = 1.0f;

    // ------------------------------------------------------------
    // Init: called once by Core, receives the created Window
    // ------------------------------------------------------------
    void init(gfx::Window& win)
    {
        gWin = &win;
        using namespace Framework;

        // 1) Create the factory (sets FACTORY)
        sFactory = std::make_unique<GameObjectFactory>();

        // 2) Register components (FACTORY must exist first!)
        RegisterComponent(TestComponent);
        RegisterComponent(TransformComponent);
        RegisterComponent(RenderComponent);
        RegisterComponent(CircleRenderComponent);

        // 3) Create objects from JSON
        sTestObj = FACTORY->Create("../../Data_Files/test.json");
        sTestObj2 = FACTORY->Create("../../Data_Files/test2.json");
        sCircleObj = FACTORY->Create("../../Data_Files/circle.json");

        // Debug checks
        if (auto* tc = sTestObj->GetComponentType<TestComponent>(ComponentTypeId::CT_TestComponent)) {
            std::cout << "[Check] TestComponent: " << tc->name << " hp=" << tc->hp << "\n";
        }
        else {
            std::cout << "[Check] TestComponent missing!\n";
        }

        if (auto* tr = sTestObj->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent)) {
            std::cout << "[Check] TransformComponent: x=" << tr->x << " y=" << tr->y << " rot=" << tr->rot << "\n";
        }
        else {
            std::cout << "[Check] TransformComponent missing!\n";
        }

        if (!sTestObj2) {
            std::cerr << "[Test2] Failed to create GOC from test2.json\n";
        }
        else {
            auto* tc2 = sTestObj2->GetComponentType<TestComponent>(ComponentTypeId::CT_TestComponent);
            if (!tc2) std::cerr << "TestComponent not found (registry/JSON mismatch?)\n";
            else      std::cout << "[JSON] name=" << tc2->name << ", hp=" << tc2->hp << "\n";
        }

        // Print all with TestComponent
        for (auto& [id, obj] : FACTORY->Objects()) {
            if (auto* c = obj->GetComponentType<TestComponent>(ComponentTypeId::CT_TestComponent)) {
                std::cout << "[GOC " << id << "] name=" << c->name << " hp=" << c->hp << "\n";
            }
        }

        // cache base size for sTestObj if it has a RenderComponent
        if (sTestObj) {
            if (auto* rc = sTestObj->GetComponentType<RenderComponent>(ComponentTypeId::CT_RenderComponent)) {
                gRectBaseW = rc->w; gRectBaseH = rc->h; gRectScale = 1.0f;
            }
        }

        // Load fixed size from JSON (for window)
        WindowConfig cfg = LoadWindowConfig("../../Data_Files/window.json");
        gScreenW = cfg.width;
        gScreenH = cfg.height;

        // Audio bootstrap
        initializeAudio();
        startAudio(busInstance);

        // --- OpenGL setup for the demo quad ---
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

        // Initialize Graphics system (VAOs, shaders for objects/background)
        gfx::Graphics::initialize();

        // start centered for the demo quad
        gPosX = gScreenW * 0.5f;
        gPosY = gScreenH * 0.5f;
        gRot = 0.f;
        gScale = 1.f;

        std::cout << "\n=== Controls ===\n"
            << "1: coin | 2: toggle footsteps | 3: level win | 4: lose | 5: click | 6: win\n"
            << "M: toggle master volume | S: stop all | ESC handled by window\n"
            << "Q/E: rotate selected object | Z/X: scale down/up | SHIFT accelerate | R reset\n"
            << "=======================================\n";
    }

    // ------------------------------------------------------------
    // Update: called every frame
    // ------------------------------------------------------------
    void update(float dt)
    {
        using namespace Framework;

        handleAudioInput(*gWin, gKeyEdge, busInstance);

        // sweep factory once per frame (handles deferred destroys)
        if (sFactory) sFactory->Update(dt);

        // Example: press Y to send a Ping to the TestComponent
        static bool yDownPrev = false;
        bool yDown = gWin->isKeyPressed(GLFW_KEY_Y);
        if (yDown && !yDownPrev && sTestObj) {
            PingMessage ping{ 7 };
            sTestObj->SendMessage(ping);
        }
        yDownPrev = yDown;

        // Optional lifecycle hotkeys: U/T/I
        static bool uPrev = false, tPrev = false, iPrev = false;
        bool u = gWin->isKeyPressed(GLFW_KEY_U);
        bool t = gWin->isKeyPressed(GLFW_KEY_T);
        bool i = gWin->isKeyPressed(GLFW_KEY_I);

        if (u && !uPrev && sTestObj) {
            FACTORY->Destroy(sTestObj);
            sTestObj = nullptr;
            std::cout << "[Test] Marked GOC for deletion\n";
        }
        if (t && !tPrev) {
            if (sTestObj) { FACTORY->Destroy(sTestObj); sTestObj = nullptr; }
            if (sFactory) sFactory->Update(0.0f);
            sTestObj = FACTORY->Create("../../Data_Files/test.json");
            if (auto* rc = sTestObj->GetComponentType<RenderComponent>(ComponentTypeId::CT_RenderComponent)) {
                gRectBaseW = rc->w; gRectBaseH = rc->h; gRectScale = 1.0f;
            }
            std::cout << "[Test] Reloaded JSON GOC\n";
        }
        if (i && !iPrev && sFactory) {
            sFactory->Update(0.0f);
            std::cout << "[Test] Forced sweep\n";
        }
        uPrev = u; tPrev = t; iPrev = i;

        const float rotSpeed = DegToRad(90.f);
        const float scaleRate = 1.5f;
        const bool  shift = gWin->isKeyPressed(GLFW_KEY_LEFT_SHIFT) || gWin->isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
        const float accel = shift ? 3.f : 1.f;

        // === Drive sTestObj's Transform & Render via components ===
        if (sTestObj) {
            // rotation (Q/E)
            if (auto* tr = sTestObj->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent)) {
                if (gWin->isKeyPressed(GLFW_KEY_Q)) tr->rot += rotSpeed * dt * accel;
                if (gWin->isKeyPressed(GLFW_KEY_E)) tr->rot -= rotSpeed * dt * accel;
                // wrap
                if (tr->rot > 3.14159265f) tr->rot -= 6.28318530f;
                if (tr->rot < -3.14159265f) tr->rot += 6.28318530f;
                // reset rotation on R
                if (gWin->isKeyPressed(GLFW_KEY_R)) tr->rot = 0.f;
            }
            // scale (Z/X) affects RenderComponent w/h
            if (auto* rc = sTestObj->GetComponentType<RenderComponent>(ComponentTypeId::CT_RenderComponent)) {
                if (gWin->isKeyPressed(GLFW_KEY_X)) gRectScale *= (1.f + scaleRate * dt * accel);
                if (gWin->isKeyPressed(GLFW_KEY_Z)) gRectScale *= (1.f - scaleRate * dt * accel);
                // clamp
                gRectScale = std::clamp(gRectScale, 0.25f, 4.0f);
                // reset on R
                if (gWin->isKeyPressed(GLFW_KEY_R)) gRectScale = 1.f;

                rc->w = gRectBaseW * gRectScale;
                rc->h = gRectBaseH * gRectScale;
            }
        }
    }

    // ------------------------------------------------------------
    // Draw: called every frame
    // ------------------------------------------------------------
    void draw()
    {
        // --- Old demo quad (kept; independent of ECS, uses pixel-space ortho) ---
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

        // --- Background ---
        gfx::Graphics::renderBackground();

        // === ECS-driven drawing: rectangles ===
        for (auto& [id, obj] : Framework::FACTORY->Objects()) {
            auto* tr = obj->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            auto* rc = obj->GetComponentType<Framework::RenderComponent>(
                Framework::ComponentTypeId::CT_RenderComponent);
            if (!tr || !rc) continue;

            // Positions/sizes treated as NDC-based here
            gfx::Graphics::renderRectangle(
                tr->x, tr->y, tr->rot,
                rc->w, rc->h,
                rc->r, rc->g, rc->b, rc->a
            );
        }

        // === ECS-driven drawing: circles ===
        for (auto& [id, obj] : Framework::FACTORY->Objects()) {
            auto* tr = obj->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            auto* cc = obj->GetComponentType<Framework::CircleRenderComponent>(
                Framework::ComponentTypeId::CT_CircleRenderComponent);
            if (!tr || !cc) continue;

            gfx::Graphics::renderCircle(
                tr->x, tr->y, cc->radius,
                cc->r, cc->g, cc->b, cc->a
            );
        }
    }

    // ------------------------------------------------------------
    // Shutdown: called once after loop
    // ------------------------------------------------------------
    void shutdown()
    {
        std::cout << "Cleaning up sound..." << std::endl;
        cleanupAudio();

        // Unload all graphics
        std::cout << "Cleaning up graphics..." << std::endl;
        Resource_Manager::unloadAll(Resource_Manager::Graphics);

        using namespace Framework;
        // Destroy test objects and the factory cleanly
        if (sTestObj) { FACTORY->Destroy(sTestObj);   sTestObj = nullptr; }
        if (sTestObj2) { FACTORY->Destroy(sTestObj2);  sTestObj2 = nullptr; }
        if (sCircleObj) { FACTORY->Destroy(sCircleObj); sCircleObj = nullptr; }
        if (sFactory) { sFactory->Update(0.0f); sFactory.reset(); }

        if (gProg) glDeleteProgram(gProg);
        gQuad.destroy();

        gProg = 0;
        gUMVP = gUColor = -1;
        gWin = nullptr;

        std::cout << "Game ended." << std::endl;
    }

} // namespace mygame
