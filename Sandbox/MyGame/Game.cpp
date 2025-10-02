/*********************************************************************************************
 \file      RigidBodyComponent.h
 \par       SofaSpuds
 \author   

 \brief     

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

// Sandbox/MyGame/Game.cpp
#include "../../Engine/Graphics/Window.hpp"
#include "Audio/SoundManager.h"
#include "Messaging_System/Messager_Bus.hpp"
#include "Audio_Tester.h"
#include "Game.hpp"
#include "Graphics/Graphics.hpp"
#include "Graphics/GraphicsText.hpp"

// use fixed screen size from JSON
#include "Config/WindowConfig.h"

// math & GL helpers
#include "MathUtils.hpp"

// --- Platform helpers to locate executable directory (keep before glad/glfw)
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

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
#include "Debug/Perf.h"
#include "Input/Input.h"

#include <filesystem>

// Crash logging
#include "Debug/CrashLogger.hpp"

namespace mygame
{
    // Safe check that a pointer still belongs to the factory
    static bool IsAlive(Framework::GOC* obj) {
        if (!obj || !Framework::FACTORY) return false;
        for (auto& [id, ptr] : Framework::FACTORY->Objects())
            if (ptr == obj) return true;
        return false;
    }

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

    // scale control for the Player's RenderComponent (Z/X & R keys)
    static float gRectScale = 1.0f;
    static float gRectBaseW = 0.5f, gRectBaseH = 0.5f;
    static bool  gCaptured = false;

    // Optional demo texture
    static unsigned int gPlayerTex = 0;

    // --- Two text renderers (two instances) using Roboto only ---
    static gfx::TextRenderer gTextTitle;   // title
    static gfx::TextRenderer gTextHint;    // control hint
    static bool gTextReadyTitle = false;
    static bool gTextReadyHint = false;

    // Level objects
    static Framework::GOC* sRectObj = nullptr;
    static std::vector<Framework::GOC*> sLevelObjs;

    // --- Helpers to find fonts robustly (Roboto only, no absolute paths, no Arial) ---
    static std::filesystem::path GetExeDir() {
        namespace fs = std::filesystem;
#if defined(_WIN32)
        char buf[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, buf, MAX_PATH);
        return fs::path(buf).parent_path();
#elif defined(__APPLE__)
        char buf[2048];
        uint32_t sz = sizeof(buf);
        if (_NSGetExecutablePath(buf, &sz) == 0) return fs::path(buf).parent_path();
        std::string s; s.resize(sz);
        if (_NSGetExecutablePath(s.data(), &sz) == 0) return fs::path(s).parent_path();
        return fs::current_path();
#else
        char buf[4096] = {};
        ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (n > 0) { buf[n] = 0; return fs::path(buf).parent_path(); }
        return fs::current_path();
#endif
    }

    // Try to resolve Roboto from project layout only.
    // Priority: Roboto-Black.ttf → Roboto-Regular.ttf → any Roboto*.ttf under assets/Fonts up the tree.
    static std::string FindRoboto() {
        namespace fs = std::filesystem;

        // 1) Direct project-relative candidates (common build/run roots)
        const char* rels[] = {
            "assets/Fonts/Roboto-Black.ttf",
            "assets/Fonts/Roboto-Regular.ttf",
            "assets/Fonts/Roboto-VariableFont_wdth,wght.ttf",
            "assets/Fonts/Roboto-Italic-VariableFont_wdth,wght.ttf",

            "../assets/Fonts/Roboto-Black.ttf",
            "../assets/Fonts/Roboto-Regular.ttf",
            "../assets/Fonts/Roboto-VariableFont_wdth,wght.ttf",
            "../assets/Fonts/Roboto-Italic-VariableFont_wdth,wght.ttf",

            "../../assets/Fonts/Roboto-Black.ttf",
            "../../assets/Fonts/Roboto-Regular.ttf",
            "../../assets/Fonts/Roboto-VariableFont_wdth,wght.ttf",
            "../../assets/Fonts/Roboto-Italic-VariableFont_wdth,wght.ttf",

            "../../../assets/Fonts/Roboto-Black.ttf",
            "../../../assets/Fonts/Roboto-Regular.ttf",
            "../../../assets/Fonts/Roboto-VariableFont_wdth,wght.ttf",
            "../../../assets/Fonts/Roboto-Italic-VariableFont_wdth,wght.ttf"
        };
        for (auto r : rels) if (fs::exists(r)) return std::string(r);

        // 2) Walk upward from CWD and EXE dir for assets/Fonts
        std::vector<fs::path> roots{ fs::current_path(), GetExeDir() };

        auto try_pick = [&](const fs::path& fontsDir) -> std::string {
            fs::path rb = fontsDir / "Roboto-Black.ttf";
            if (fs::exists(rb)) return rb.string();
            fs::path rr = fontsDir / "Roboto-Regular.ttf";
            if (fs::exists(rr)) return rr.string();
            if (fs::exists(fontsDir)) {
                for (auto& e : fs::directory_iterator(fontsDir)) {
                    if (!e.is_regular_file()) continue;
                    auto name = e.path().filename().string();
                    if (name.rfind("Roboto", 0) == 0 && e.path().extension() == ".ttf")
                        return e.path().string();
                }
            }
            return {};
            };

        for (const auto& root : roots) {
            auto p = root;
            for (int up = 0; up < 7 && !p.empty(); ++up) {
                auto base = p / "assets" / "Fonts";
                if (auto picked = try_pick(base); !picked.empty()) return picked;
                p = p.parent_path();
            }
        }

        return {};
    }

    // Input helper
    static Framework::InputManager gInput(nullptr);

    using clock = std::chrono::high_resolution_clock;

    // ================== Sprite sheet lightweight animator (Player only) ==================
    enum class AnimState { Idle, Run };
    static AnimState sAnimState = AnimState::Idle;

    static unsigned int gTexIdle = 0;
    static unsigned int gTexRun = 0;

    static int   gIdleCols = 5, gIdleRows = 1, gIdleFrames = 5;
    static int   gRunCols = 8, gRunRows = 1, gRunFrames = 8;

    static float gIdleFPS = 6.f;
    static float gRunFPS = 10.f;

    static int   gFrame = 0;
    static float gFrameClock = 0.f;

    static inline float     CurrentFPS() { return (sAnimState == AnimState::Run) ? gRunFPS : gIdleFPS; }
    static inline int       CurrentFrames() { return (sAnimState == AnimState::Run) ? gRunFrames : gIdleFrames; }
    static inline int       CurrentCols() { return (sAnimState == AnimState::Run) ? gRunCols : gIdleCols; }
    static inline int       CurrentRows() { return (sAnimState == AnimState::Run) ? gRunRows : gIdleRows; }
    static inline unsigned  CurrentTex() { return (sAnimState == AnimState::Run) ? gTexRun : gTexIdle; }
    static inline void      ResetAnim() { gFrame = 0; gFrameClock = 0.f; }

    // ------------------------------------------------------------
    // Init
    // ------------------------------------------------------------
    void init(gfx::Window& win)
    {
        gWin = &win;
        using namespace Framework;
        gInput = Framework::InputManager(gWin->raw());

        // Crash logger
        g_crashLogger = new CrashLogger(std::string("../../logs"),
            std::string("crash.log"),
            std::string("ENGINE/CRASH"));
        std::cout << "[CrashLog] " << g_crashLogger->LogPath() << "\n";

        InstallTerminateHandler();
        InstallSignalHandlers();

        // Factory & components
        sFactory = std::make_unique<GameObjectFactory>();
        RegisterComponent(TransformComponent);
        RegisterComponent(RenderComponent);
        RegisterComponent(CircleRenderComponent);
        RegisterComponent(SpriteComponent);
        RegisterComponent(RigidBodyComponent);

        // Prefabs & level
        LoadPrefabs();
        auto p = std::string("../../Data_Files/player.json");
        std::cout << "[Prefab] Player path = " << absolute(p) << "  exists=" << exists(p) << "\n";
        sLevelObjs = sFactory->CreateLevel("../../Data_Files/level.json");

        // Cache player's base size once
        for (auto* obj : sLevelObjs) {
            if (obj && obj->GetObjectName() == "Player") {
                sRectObj = obj;
                if (auto* rc = sRectObj->GetComponentType<Framework::RenderComponent>(
                    Framework::ComponentTypeId::CT_RenderComponent)) {
                    gRectBaseW = rc->w;  gRectBaseH = rc->h; gRectScale = 1.f; gCaptured = true;
                }
                break;
            }
        }

        // Window size
        WindowConfig cfg = LoadWindowConfig("../../Data_Files/window.json");
        gScreenW = cfg.width; gScreenH = cfg.height;

        // Audio
        initializeAudio();
        startAudio(busInstance);

        // GL state
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Graphics
        gfx::Graphics::initialize();

        // --- Text init (two instances) with Roboto only ---
        std::cout << "[CWD] " << std::filesystem::current_path() << "\n";
        std::cout << "[EXE] " << GetExeDir() << "\n";
        if (auto fontPath = FindRoboto(); !fontPath.empty()) {
            std::cout << "[Text] Using font: " << fontPath << "\n";
            gTextTitle.initialize(fontPath.c_str(), gScreenW, gScreenH);
            gTextHint.initialize(fontPath.c_str(), gScreenW, gScreenH);
            gTextReadyTitle = true;
            gTextReadyHint = true;
        }
        else {
            std::cout << "[Text] Roboto not found. Text will be skipped.\n";
            gTextReadyTitle = gTextReadyHint = false;
        }

        // Demo textures
        Resource_Manager::load("player_png", "../../assets/Textures/player.png");
        gPlayerTex = Resource_Manager::resources_map["player_png"].handle;

        // Sprite sheets
        Resource_Manager::load("ming_idle", "../../assets/Textures/Idle Sprite .png");
        Resource_Manager::load("ming_run", "../../assets/Textures/Running Sprite .png");
        gTexIdle = Resource_Manager::resources_map["ming_idle"].handle;
        gTexRun = Resource_Manager::resources_map["ming_run"].handle;
        sAnimState = AnimState::Idle;
        ResetAnim();

        std::cout << "\n=== Controls ===\n"
            << "WASD: Move | Q/E: Rotate | Z/X: Scale | R: Reset\n"
            << "A/D held => Run animation, otherwise Idle\n"
            << "F1: Toggle Performance Overlay (FPS & timings)\n"
            << "=======================================\n";

        // ImGui
        ImGuiLayerConfig cFg;
        cFg.glsl_version = "#version 330";
        cFg.dockspace = true;
        cFg.gamepad = false;
        ImGuiLayer::Initialize(win, cFg);
    }

    // ------------------------------------------------------------
    // Update
    // ------------------------------------------------------------
    void update(float dt)
    {
        TryGuard::Run([&] {
            using namespace Framework;
            gInput.Update();

            // Perf overlay toggle + frame begin
            Framework::PerfFrameStart(dt, gWin->isKeyPressed(GLFW_KEY_F1));

            auto t0 = clock::now(); // start timing Update

            // sweep factory once per frame (handles deferred destroys)
            if (sFactory) sFactory->Update(dt);

            const float rotSpeed = DegToRad(90.f);
            const float scaleRate = 1.5f;
            const bool  shift = gWin->isKeyPressed(GLFW_KEY_LEFT_SHIFT) || gWin->isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
            const float accel = shift ? 3.f : 1.f;

            // find the "Player"
            sRectObj = nullptr;
            for (auto* obj : sLevelObjs) {
                if (obj && obj->GetObjectName() == "Player") { sRectObj = obj; break; }
            }

            AABB ahitbox(0, 0, 0, 0), bhitbox(0, 0, 0, 0);

            if (sRectObj) {
                auto* tr = sRectObj->GetComponentType<Framework::TransformComponent>(
                    Framework::ComponentTypeId::CT_TransformComponent);
                auto* rc = sRectObj->GetComponentType<Framework::RenderComponent>(
                    Framework::ComponentTypeId::CT_RenderComponent);
                auto* rbc = sRectObj->GetComponentType<Framework::RigidBodyComponent>(
                    Framework::ComponentTypeId::CT_RigidBodyComponent);

                if (tr && rbc) ahitbox = AABB(tr->x, tr->y, rbc->width, rbc->height);

                // rotation (Q/E)
                if (tr) {
                    if (gWin->isKeyPressed(GLFW_KEY_Q)) tr->rot += rotSpeed * dt * accel;
                    if (gWin->isKeyPressed(GLFW_KEY_E)) tr->rot -= rotSpeed * dt * accel;
                    if (tr->rot > 3.14159265f)  tr->rot -= 6.28318530f;
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
                // movement (WASD)
                if (rbc && tr) {
                    if (gWin->isKeyPressed(GLFW_KEY_D)) tr->x += rbc->velX * dt;
                    if (gWin->isKeyPressed(GLFW_KEY_A)) tr->x -= rbc->velX * dt;
                    if (gWin->isKeyPressed(GLFW_KEY_W)) tr->y += rbc->velY * dt;
                    if (gWin->isKeyPressed(GLFW_KEY_S)) tr->y -= rbc->velY * dt;
                }

                // animation state
                const bool wantRun =
                    gWin->isKeyPressed(GLFW_KEY_A) || gWin->isKeyPressed(GLFW_KEY_D) ||
                    gWin->isKeyPressed(GLFW_KEY_W) || gWin->isKeyPressed(GLFW_KEY_S) ||
                    gWin->isKeyPressed(GLFW_KEY_LEFT) || gWin->isKeyPressed(GLFW_KEY_RIGHT) ||
                    gWin->isKeyPressed(GLFW_KEY_UP) || gWin->isKeyPressed(GLFW_KEY_DOWN);
                AnimState newState = wantRun ? AnimState::Run : AnimState::Idle;
                if (newState != sAnimState) { sAnimState = newState; ResetAnim(); }

                // advance frame clock
                gFrameClock += dt * CurrentFPS();
                while (gFrameClock >= 1.f) { gFrameClock -= 1.f; gFrame = (gFrame + 1) % CurrentFrames(); }
            }

            // find a rect for collision test
            for (auto* obj2 : sLevelObjs) {
                if (obj2 && obj2->GetObjectName() == "rect") { sTestObj = obj2; break; }
            }
            if (IsAlive(sTestObj)) {
                auto* tr2 = sTestObj->GetComponentType<Framework::TransformComponent>(
                    Framework::ComponentTypeId::CT_TransformComponent);
                auto* rbc2 = sTestObj->GetComponentType<Framework::RigidBodyComponent>(
                    Framework::ComponentTypeId::CT_RigidBodyComponent);
                if (tr2 && rbc2) bhitbox = AABB(tr2->x, tr2->y, rbc2->width, rbc2->height);
            }
            else {
                sTestObj = nullptr;
            }

            if (Collision::CheckCollisionRectToRect(ahitbox, bhitbox))
                std::cout << "Collision detected!" << std::endl;

            // Simple input tests
            if (gInput.IsKeyPressed(GLFW_KEY_SPACE))  std::cout << "Spacebar pressed!" << std::endl;
            if (gInput.IsKeyHeld(GLFW_KEY_SPACE))     std::cout << "Spacebar held!" << std::endl;
            if (gInput.IsKeyReleased(GLFW_KEY_SPACE)) std::cout << "Spacebar released!" << std::endl;

            if (gInput.IsMousePressed(GLFW_MOUSE_BUTTON_LEFT))  std::cout << "LMB pressed!" << std::endl;
            if (gInput.IsMouseHeld(GLFW_MOUSE_BUTTON_LEFT))     std::cout << "LMB held!" << std::endl;
            if (gInput.IsMouseReleased(GLFW_MOUSE_BUTTON_LEFT)) std::cout << "LMB released!" << std::endl;

            handleAudioInput(*gWin, gKeyEdge, busInstance);

            const double updateMs = std::chrono::duration<double, std::milli>(clock::now() - t0).count();
            Framework::setUpdate(updateMs);
            }, "mygame::update");
    }

    // ------------------------------------------------------------
    // Draw
    // ------------------------------------------------------------
    void draw()
    {
        TryGuard::Run([&] {
            // -------- Render (non-ImGui) timing --------
            auto t0 = clock::now();

            // Background
            gfx::Graphics::renderBackground();

            // Sprites
            for (auto& [id, obj] : Framework::FACTORY->Objects()) {
                auto* tr = obj->GetComponentType<Framework::TransformComponent>(
                    Framework::ComponentTypeId::CT_TransformComponent);
                if (!tr) continue;

                if (auto* sp = obj->GetComponentType<Framework::SpriteComponent>(
                    Framework::ComponentTypeId::CT_SpriteComponent)) {

                    // Default size/tint; can be overridden by RenderComponent if present
                    float sx = 1.f, sy = 1.f;
                    float r = 1.f, g = 1.f, b = 1.f, a = 1.f;

                    if (auto* rc = obj->GetComponentType<Framework::RenderComponent>(
                        Framework::ComponentTypeId::CT_RenderComponent)) {
                        sx = rc->w; sy = rc->h; r = rc->r; g = rc->g; b = rc->b; a = rc->a;
                    }

                    // Player uses sheet animation
                    if (obj->GetObjectName() == "Player" && gTexIdle && gTexRun) {
                        gfx::Graphics::renderSpriteFrame(
                            CurrentTex(), tr->x, tr->y, tr->rot,
                            sx, sy,
                            gFrame, CurrentCols(), CurrentRows(),
                            r, g, b, a
                        );
                        continue;
                    }

                    // Other sprites: whole texture
                    unsigned tex = sp->texture_id;
                    if (!tex && !sp->texture_key.empty()) {
                        tex = Resource_Manager::getTexture(sp->texture_key);
                        sp->texture_id = tex;
                    }
                    if (tex) {
                        gfx::Graphics::renderSprite(tex, tr->x, tr->y, tr->rot, sx, sy, r, g, b, a);
                    }
                }
            }

            // Rectangles (no sprite)
            for (auto& [id, obj] : Framework::FACTORY->Objects()) {
                auto* tr = obj->GetComponentType<Framework::TransformComponent>(
                    Framework::ComponentTypeId::CT_TransformComponent);
                auto* rc = obj->GetComponentType<Framework::RenderComponent>(
                    Framework::ComponentTypeId::CT_RenderComponent);
                if (!tr || !rc) continue;
                if (obj->GetComponentType<Framework::SpriteComponent>(
                    Framework::ComponentTypeId::CT_SpriteComponent)) continue;

                gfx::Graphics::renderRectangle(tr->x, tr->y, tr->rot, rc->w, rc->h, rc->r, rc->g, rc->b, rc->a);
            }

            // Circles
            for (auto& [id, obj] : Framework::FACTORY->Objects()) {
                auto* tr = obj->GetComponentType<Framework::TransformComponent>(
                    Framework::ComponentTypeId::CT_TransformComponent);
                auto* cc = obj->GetComponentType<Framework::CircleRenderComponent>(
                    Framework::ComponentTypeId::CT_CircleRenderComponent);
                if (!tr || !cc) continue;

                gfx::Graphics::renderCircle(tr->x, tr->y, cc->radius, cc->r, cc->g, cc->b, cc->a);
            }

            // Two separate text instances, different positions
            if (gTextReadyTitle) {
                // Title (top-left)
                gTextTitle.RenderText(
                    "Bloody Good Curry",
                    32.0f,
                    static_cast<float>(gScreenH) - 64.0f,
                    1.05f,
                    glm::vec3(1.0f, 1.0f, 1.0f)
                );
            }
            if (gTextReadyHint) {
                // Keep it clearly away from the edges and from the title
                // scale 0.75 makes it smaller than the title
                gTextHint.RenderText(
                    "Press WASD to run",
                    32.0f,
                    40.0f,
                    0.75f,
                    glm::vec3(0.95f, 0.85f, 0.10f)
                );
            }

            // Record render timing
            const double renderMs = std::chrono::duration<double, std::milli>(clock::now() - t0).count();
            Framework::setRender(renderMs);

            // -------- ImGui timing --------
            t0 = clock::now();

            // Spawn panel etc.
            mygame::DrawSpawnPanel();
          

            // Crash test panel
            if (ImGui::Begin("Crash Tests")) {
                if (ImGui::Button("Crash BG shader"))     gfx::Graphics::testCrash(1);
                if (ImGui::Button("Crash BG VAO"))        gfx::Graphics::testCrash(2);
                if (ImGui::Button("Crash Sprite shader")) gfx::Graphics::testCrash(3);
                if (ImGui::Button("Crash Object shader")) gfx::Graphics::testCrash(4);
                if (ImGui::Button("Delete BG texture"))   gfx::Graphics::testCrash(5);
            }
            ImGui::End();

            // Performance overlay
            Framework::DrawPerformanceWindow();

            // finish ImGui timing (value shown next frame)
            const double imguiMs = std::chrono::duration<double, std::milli>(clock::now() - t0).count();
            Framework::setImGui(imguiMs);
            }, "mygame::draw");
    }

    // ------------------------------------------------------------
    // Shutdown
    // ------------------------------------------------------------
    void shutdown()
    {
        std::cout << "Cleaning up sound..." << std::endl;
        cleanupAudio();

        std::cout << "Cleaning up graphics..." << std::endl;
        gfx::Graphics::cleanup();
        Resource_Manager::unloadAll(Resource_Manager::Graphics);

        // Cleanup text renderers
        gTextTitle.cleanup();
        gTextHint.cleanup();
        gTextReadyTitle = gTextReadyHint = false;

        using namespace Framework;
        if (sFactory) { sFactory->Update(0.0f); sFactory.reset(); }
        Framework::UnloadPrefabs();

        ImGuiLayer::Shutdown();
        if (ImGui::GetCurrentContext()) ImGui::DestroyContext();

        gWin = nullptr;

        std::cout << "Game ended." << std::endl;

        // Crash logger cleanup
        if (g_crashLogger) { delete g_crashLogger; g_crashLogger = nullptr; }
    }

} // namespace mygame
