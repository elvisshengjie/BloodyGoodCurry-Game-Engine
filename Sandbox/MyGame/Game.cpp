/*********************************************************************************************
 \file      RigidBodyComponent.h
 \par       SofaSpuds
 \author   All TEAM MEMBERS

 \brief  A lightweight 2D rigid-body component for the engine’s component system.
         It stores the kinematic state (velX, velY), collider size (width, height),
         and basic flags such as isStatic / useGravity / damping. Each frame it
         updates the owner’s Transform (simple Euler integration) and exposes an
         AABB for collision tests in Physics/Collision. All fields are data-driven:
         they can be de-serialized from JSON in prefabs/levels (e.g., width, height,
         velX, velY, mass, damping, isStatic, useGravity). Designed for fast gameplay
         prototyping—no rotation or advanced forces yet; integrates with
         `Collision::CheckCollisionRectToRect` and is used by Game.cpp movement logic
 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "Graphics/Window.hpp"
#include "Systems/SystemManager.h"
#include "Systems/InputSystem.h"
#include "Systems/LogicSystem.h"
#include "Systems/PhysicSystem.h"
#include "Systems/RenderSystem.h"
#include "Systems/audioSystem.h"
#include "Debug/CrashLogger.hpp"
#include "Debug/Perf.h"
#include <GLFW/glfw3.h>

namespace mygame
{
    namespace
    {
        using clock = std::chrono::high_resolution_clock;

        Framework::SystemManager gSystems;
        Framework::InputSystem* gInputSystem = nullptr;
        Framework::LogicSystem* gLogicSystem = nullptr;
        Framework::PhysicSystem* gPhysicsSystem = nullptr;
        Framework::AudioSystem* gAudioSystem = nullptr;
        Framework::RenderSystem* gRenderSystem = nullptr;
    }

   // ------------------------------------------------------------
  // Init
  // ------------------------------------------------------------
    void init(gfx::Window& win)
    {
        gInputSystem = gSystems.RegisterSystem<Framework::InputSystem>(win);
        gLogicSystem = gSystems.RegisterSystem<Framework::LogicSystem>(win, *gInputSystem);
        gPhysicsSystem = gSystems.RegisterSystem<Framework::PhysicSystem>(*gLogicSystem);
        gAudioSystem = gSystems.RegisterSystem<Framework::AudioSystem>();
        gRenderSystem = gSystems.RegisterSystem<Framework::RenderSystem>(win, *gLogicSystem);

        //(void)gPhysicsSystem;
        //(void)gAudioSystem;
        //(void)gRenderSystem;

        gSystems.IntializeAll();
    }
    // ------------------------------------------------------------
   // Update
   // ------------------------------------------------------------
    void update(float dt)
    {
        TryGuard::Run([&] {
            const bool togglePerf = gInputSystem && gInputSystem->IsWindowKeyPressed(GLFW_KEY_F1);
            Framework::PerfFrameStart(dt, togglePerf);

            auto t0 = clock::now();
            gSystems.UpdateAll(dt);
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
            gSystems.DrawAll();
            }, "mygame::draw");
    }
    // ------------------------------------------------------------
   // Shutdown
   // ------------------------------------------------------------
    void shutdown()
    {
        gSystems.ShutdownAll();
    } 
}