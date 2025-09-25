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

#include "Component/TransformComponent.h"
#include "Component/RenderComponent.h"
#include "Component/CircleRenderComponent.h"

namespace mygame
{
    // ===== Persistent state =====
    static gfx::Window* gWin = nullptr;

    static int   gScreenW = 800;
    static int   gScreenH = 600;



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


    Framework::GOC* sRectObj = nullptr;
    static std::vector<Framework::GOC*> sLevelObjs;
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
   
        RegisterComponent(TransformComponent);
        RegisterComponent(RenderComponent);
        RegisterComponent(CircleRenderComponent);

        // 3) Create objects from JSON
        //sTestObj = FACTORY->Create("../../Data_Files/test.json");
        //sTestObj2 = FACTORY->Create("../../Data_Files/test2.json");
        //sCircleObj = FACTORY->Create("../../Data_Files/circle.json");
        sLevelObjs = sFactory->CreateLevel("../../Data_Files/level.json");
   
   /*     if (auto* tr = sTestObj->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent)) {
            std::cout << "[Check] TransformComponent: x=" << tr->x << " y=" << tr->y << " rot=" << tr->rot << "\n";
        }
        else {
            std::cout << "[Check] TransformComponent missing!\n";
        }

    */

        // cache base size for sTestObj if it has a RenderComponent
        //if (sTestObj) {
        //    if (auto* rc = sTestObj->GetComponentType<RenderComponent>(ComponentTypeId::CT_RenderComponent)) {
        //        gRectBaseW = rc->w; gRectBaseH = rc->h; gRectScale = 1.0f;
        //    }
        //}

        // Load fixed size from JSON (for window)
        WindowConfig cfg = LoadWindowConfig("../../Data_Files/window.json");
        gScreenW = cfg.width;
        gScreenH = cfg.height;

        // Audio bootstrap
        initializeAudio();
        startAudio(busInstance);

        // --- No more demo-quad shader/VAO setup ---
        // Keep blending enabled for ECS shapes with alpha
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Initialize Graphics system (VAOs, shaders for ECS objects/background)
        gfx::Graphics::initialize();

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



        // Optional lifecycle hotkeys: U/T/I
     /*   static bool uPrev = false, tPrev = false, iPrev = false;
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
        }*/
        /*  uPrev = u; tPrev = t; iPrev = i;*/

        const float rotSpeed = DegToRad(90.f);
        const float scaleRate = 1.5f;
        const bool  shift = gWin->isKeyPressed(GLFW_KEY_LEFT_SHIFT) || gWin->isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
        const float accel = shift ? 3.f : 1.f;

        // === Drive sTestObj's Transform & Render via components ===
        for (auto* obj : sLevelObjs) {
            if (obj->GetObjectName() == "rect") {
                sRectObj = obj;
                break;
            }
        }
        if (sRectObj) {
            auto* tr = sRectObj->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            auto* rc = sRectObj->GetComponentType<Framework::RenderComponent>(
                Framework::ComponentTypeId::CT_RenderComponent);

            // rotation (Q/E)
            if (tr) {
                if (gWin->isKeyPressed(GLFW_KEY_Q)) tr->rot += rotSpeed * dt * accel;
                if (gWin->isKeyPressed(GLFW_KEY_E)) tr->rot -= rotSpeed * dt * accel;
                if (tr->rot > 3.14159265f) tr->rot -= 6.28318530f;
                if (tr->rot < -3.14159265f) tr->rot += 6.28318530f;
                if (gWin->isKeyPressed(GLFW_KEY_R)) tr->rot = 0.f;
            }

            // scale (Z/X) affects RenderComponent w/h
            if (rc) {
                if (gWin->isKeyPressed(GLFW_KEY_X)) gRectScale *= (1.f + scaleRate * dt * accel);
                if (gWin->isKeyPressed(GLFW_KEY_Z)) gRectScale *= (1.f - scaleRate * dt * accel);

                gRectScale = std::clamp(gRectScale, 0.25f, 4.0f);
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
       

        // --- Background ---
        gfx::Graphics::renderBackground();

        // === ECS-driven drawing: rectangles ===
        for (auto& [id, obj] : Framework::FACTORY->Objects()) {
            auto* tr = obj->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            auto* rc = obj->GetComponentType<Framework::RenderComponent>(
                Framework::ComponentTypeId::CT_RenderComponent);
            if (!tr || !rc) continue;

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
   /*     if (sTestObj) { FACTORY->Destroy(sTestObj);   sTestObj = nullptr; }
        if (sTestObj2) { FACTORY->Destroy(sTestObj2);  sTestObj2 = nullptr; }
        if (sCircleObj) { FACTORY->Destroy(sCircleObj); sCircleObj = nullptr; }*/
        if (sFactory) { sFactory->Update(0.0f); sFactory.reset(); }

        // --- Removed: if (gProg) glDeleteProgram(gProg); and gQuad.destroy(); ---

        gWin = nullptr;

        std::cout << "Game ended." << std::endl;
    }


} // namespace mygame
