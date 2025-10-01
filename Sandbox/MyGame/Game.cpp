// Sandbox/MyGame/Game.cpp
#include "../../Engine/Graphics/Window.hpp"
#include "Audio/SoundManager.h"
#include "Messaging_System/Messager_Bus.hpp"
#include "Audio_Tester.h"
#include "Game.hpp"
#include "Graphics/Graphics.hpp"

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
#include "Component/SpriteComponent.h"
#include "Composition/PrefabManager.h"
#include "Physics/Dynamics/RigidBodyComponent.h"

#include "Debug/ImGuiLayer.h"
#include "imgui.h"
#include "Debug/Spawn.h"
#include "Physics/Collision/Collision.h"

#include <filesystem>

namespace mygame
{
    using std::filesystem::absolute; using std::filesystem::exists;
    

    // ===== Persistent state =====
    static gfx::Window* gWin = nullptr;

    static int   gScreenW = 800;
    static int   gScreenH = 600;

    // audio state
    static std::array<bool, 10> gKeyEdge{};
    static MessageBus busInstance;

    // component system
    static std::unique_ptr<Framework::GameObjectFactory> sFactory;
    static Framework::GOC* sTestObj = nullptr;   // owned by the factory
    static Framework::GOC* sTestObj2 = nullptr;  // owned by the factory
    static Framework::GOC* sCircleObj = nullptr; // owned by the factory

    // scale control for sTestObj's RenderComponent (Z/X & R keys)
    static float gRectScale = 1.0f;
    static float gRectBaseW = 1.0f, gRectBaseH = 1.0f;

    // --- NEW: texture for sprite rendering of the rectangle ---
    static unsigned int gPlayerTex = 0;

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
        RegisterComponent(SpriteComponent);
        RegisterComponent(RigidBodyComponent);


        //3)Create Master copy
        LoadPrefabs();
        auto p = std::string("../../Data_Files/player.json");
        std::cout << "[Prefab] Player path = " << absolute(p) << "  exists=" << exists(p) << "\n";

        // 4) Create objects from JSON
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

        // --- NEW: load PNG to render instead of flat-colored rectangle ---
        Resource_Manager::load("player_png", "../../assets/Textures/player.png");
        gPlayerTex = Resource_Manager::resources_map["player_png"].handle;


        std::cout << "\n=== Controls ===\n"
            << "1: coin | 2: toggle footsteps | 3: level win | 4: lose | 5: click | 6: win\n"
            << "M: toggle master volume | S: stop all | ESC handled by window\n"
            << "Q/E: rotate selected object | Z/X: scale down/up | SHIFT accelerate | R reset\n"
            << "=======================================\n";

        // Clone 10 Rects in a horizontal line
        //const int    count = 10;
        //const float  startX = 0.1f;
        //const float  gapX = 0.07f;   // normalized screen units (your renderer uses 0..1)
        //const float  y = 0.2f;

        //for (int i = 0; i < count; ++i) {
        //    auto* obj = ClonePrefab("Rect");
        //    if (!obj) { std::cout << "[Prefab] Missing Rect master!\n"; break; }

        //    // position each clone
        //    if (auto* tr = obj->GetComponentType<Framework::TransformComponent>(
        //        Framework::ComponentTypeId::CT_TransformComponent)) {
        //        tr->x = startX + i * gapX;
        //        tr->y = y;
        //        tr->rot = 0.f;
        //    }
        //}


        // Clone 6 Circles in a 2x3 grid
        /*const int   crows = 2, ccols = 3;
        const float cstartX = 0.2f, cstartY = 0.5f;
        const float cgapX = 0.15f, cgapY = 0.12f;

        for (int r = 0; r < crows; ++r) {
            for (int c = 0; c < ccols; ++c) {
                auto* obj = ClonePrefab("Circle");
                if (!obj) { std::cout << "[Prefab] Missing Circle master!\n"; continue; }

                if (auto* tr = obj->GetComponentType<Framework::TransformComponent>(
                    Framework::ComponentTypeId::CT_TransformComponent)) {
                    tr->x = cstartX + c * cgapX;
                    tr->y = cstartY + r * cgapY;
                    tr->rot = 0.f;
                }
            }
        }*/

        //Initialize ImGui
        ImGuiLayerConfig cFg;
       cFg.glsl_version = "#version 330";
       cFg.dockspace = true;
       cFg.gamepad = false;
       ImGuiLayer::Initialize(win, cFg);


    }

    // ------------------------------------------------------------
    // Update: called every frame
    // ------------------------------------------------------------
    void update(float dt)
    {
        using namespace Framework;
        AABB ahitbox(0, 0, 0, 0);
        AABB bhitbox(0, 0, 0, 0);
        handleAudioInput(*gWin, gKeyEdge, busInstance);

        // sweep factory once per frame (handles deferred destroys)
        if (sFactory) sFactory->Update(dt);


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
            auto* rbc = sRectObj->GetComponentType<Framework::RigidBodyComponent>(
                Framework::ComponentTypeId::CT_RigidBodyComponent);
            ahitbox = AABB(tr->x, tr->y, rbc->width, rbc->height);
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

            // Testing movement
            if (rbc && tr) {
                if (gWin->isKeyPressed(GLFW_KEY_D)) tr->x += rbc->velX * dt;
                if (gWin->isKeyPressed(GLFW_KEY_A)) tr->x -= rbc->velX * dt;
                if (gWin->isKeyPressed(GLFW_KEY_W)) tr->y += rbc->velY * dt;
                if (gWin->isKeyPressed(GLFW_KEY_S)) tr->y -= rbc->velY * dt;
            }
        }
        for (auto* obj2 : sLevelObjs) {
            if (obj2->GetObjectName() == "Player") {
                sTestObj = obj2;
                break;
            }
        }
        if (sTestObj) {
            auto* tr2 = sRectObj->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            auto* rc2 = sRectObj->GetComponentType<Framework::RenderComponent>(
                Framework::ComponentTypeId::CT_RenderComponent);
            auto* rbc2 = sRectObj->GetComponentType<Framework::RigidBodyComponent>(
                Framework::ComponentTypeId::CT_RigidBodyComponent);
            bhitbox = AABB(tr2->x, tr2->y, rbc2->width, rbc2->height);
        }
        if (Collision::CheckCollisionRectToRect(ahitbox, bhitbox))
            std::cout << "Collision detected!" << std::endl;
    }
    // ------------------------------------------------------------
    // Draw: called every frame
    // ------------------------------------------------------------
    void draw()
    {
       

        // --- Background ---
        gfx::Graphics::renderBackground();

        //player
        for (auto& [id, obj] : Framework::FACTORY->Objects()) {
            auto* tr = obj->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            if (!tr) continue;

            // Sprites
            if (auto* sp = obj->GetComponentType<Framework::SpriteComponent>(
                Framework::ComponentTypeId::CT_SpriteComponent)) {

                // Size/tint can come from RenderComponent (reuse it if present)
                float sx = 1.f, sy = 1.f;
                float r = 1.f, g = 1.f, b = 1.f, a = 1.f;

                if (auto* rc = obj->GetComponentType<Framework::RenderComponent>(
                    Framework::ComponentTypeId::CT_RenderComponent)) {
                    sx = rc->w; sy = rc->h;
                    r = rc->r; g = rc->g; b = rc->b; a = rc->a;
                }

                // ensure we have a GL texture id
                unsigned tex = sp->texture_id;
                if (!tex && !sp->texture_key.empty()) {
                    tex = Resource_Manager::getTexture(sp->texture_key);
                    sp->texture_id = tex; // cache it
                }
                if (tex) {
                    gfx::Graphics::renderSprite(tex, tr->x, tr->y, tr->rot, sx, sy, r, g, b, a);
                }
            }

            
        }

        // === ECS-driven drawing: rectangles ===
        for (auto& [id, obj] : Framework::FACTORY->Objects()) {
            auto* tr = obj->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            auto* rc = obj->GetComponentType<Framework::RenderComponent>(
                Framework::ComponentTypeId::CT_RenderComponent);
            if (!tr || !rc) continue;
            // if object have sprite skip
            if (obj->GetComponentType<Framework::SpriteComponent>(
                Framework::ComponentTypeId::CT_SpriteComponent)) {
                continue;
            }
            // --- UPDATED: render the rectangle as a PNG sprite ---
            gfx::Graphics::renderRectangle(
                tr->x, tr->y, tr->rot,
                rc->w, rc->h,
                1.f, 1.f, 1.f, 1.f
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
        mygame::DrawSpawnPanel();
        ImGui::ShowDemoWindow();
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
        Framework::UnloadPrefabs();
        // --- Removed: if (gProg) glDeleteProgram(gProg); and gQuad.destroy(); ---

        gWin = nullptr;

        std::cout << "Game ended." << std::endl;
        ImGuiLayer::Shutdown();
    }

} // namespace mygame
