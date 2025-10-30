/*********************************************************************************************
 \file      Game.cpp
 \par       SofaSpuds
 \author    All TEAM MEMBERS
 \brief     Game lifecycle + Main Menu page (no GUISystem).
            Clicking Start transitions to your existing game; Exit closes the window.
*********************************************************************************************/
#include "Graphics/Window.hpp"
#include "Systems/SystemManager.h"
#include "Systems/InputSystem.h"
#include "Systems/LogicSystem.h"
#include "Systems/PhysicSystem.h"
#include "Systems/RenderSystem.h"
#include "Systems/AiSystem.h"
#include "Systems/audioSystem.h"
#include "Systems/EnemySystem.h"
#include "Debug/CrashLogger.hpp"
#include "Debug/Perf.h"

#include "MainMenuPage.hpp"
#include <GLFW/glfw3.h>
#include <chrono>

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
        Framework::EnemySystem* gEnemySystem = nullptr;

        enum class GameState { MAIN_MENU, PLAYING, EXIT };
        GameState currentState = GameState::MAIN_MENU;

        MainMenuPage mainMenu;
        Framework::AiSystem* gAiSystem = nullptr;
    }

    void init(gfx::Window& win)
    {
        gInputSystem = gSystems.RegisterSystem<Framework::InputSystem>(win);
        gLogicSystem = gSystems.RegisterSystem<Framework::LogicSystem>(win, *gInputSystem);
        gPhysicsSystem = gSystems.RegisterSystem<Framework::PhysicSystem>(*gLogicSystem);
        gAudioSystem = gSystems.RegisterSystem<Framework::AudioSystem>(win);
        gRenderSystem = gSystems.RegisterSystem<Framework::RenderSystem>(win, *gLogicSystem);
        gAiSystem = gSystems.RegisterSystem<Framework::AiSystem>(win);

        gEnemySystem = gSystems.RegisterSystem<Framework::EnemySystem>(win);
        
      
        //(void)gPhysicsSystem;
        //(void)gAudioSystem;
        //(void)gRenderSystem;

        gSystems.IntializeAll();
        
        gEnemySystem->Initialize();

        // IMPORTANT: pass the real window size so mouse-Y flip is correct.
        mainMenu.Init(win.Width(), win.Height());
    }

    void update(float dt)
    {
        TryGuard::Run([&] {
            const bool togglePerf = gInputSystem && gInputSystem->IsWindowKeyPressed(GLFW_KEY_F1);
            Framework::PerfFrameStart(dt, togglePerf);

            switch (currentState)
            {
            case GameState::MAIN_MENU:
                mainMenu.Update(gInputSystem);

                // One-shot events from the page:
                if (mainMenu.ConsumeStart())
                {
                    currentState = GameState::PLAYING;   // → your existing game
                }
                else if (mainMenu.ConsumeExit())
                {
                    currentState = GameState::EXIT;
                }
                break;

            case GameState::PLAYING:
                // Your existing update path (unchanged)
                gSystems.UpdateAll(dt);
                break;

            case GameState::EXIT:
                if (auto* win = gInputSystem->Window())
                    win->close();
                break;
            }

            // (Keep your timing calc if you have one)
            Framework::setUpdate(
                std::chrono::duration<double, std::milli>(clock::now() - clock::now()).count());
            }, "mygame::update");
    }

    void draw()
    {
        TryGuard::Run([&] {
            switch (currentState)
            {
            case GameState::MAIN_MENU:
                // Draw the menu (it renders its own buttons + text)
                mainMenu.Draw(gRenderSystem);
                break;

            case GameState::PLAYING:
                // Your existing draw path (unchanged)
                gSystems.DrawAll();
                break;

            case GameState::EXIT:
                break;
            }
            }, "mygame::draw");
    }

    void shutdown()
    {
        std::cout << "[Game] Shutting down systems...\n";

        // Only call ShutdownAll(), do NOT manually delete gEnemySystem etc.
        gSystems.ShutdownAll();

        // Null out global pointers so you don’t accidentally access them later
        gEnemySystem = nullptr;
        gAiSystem = nullptr;
        gRenderSystem = nullptr;
        gAudioSystem = nullptr;
        gPhysicsSystem = nullptr;
        gLogicSystem = nullptr;
        gInputSystem = nullptr;

        std::cout << "[Game] Shutdown complete.\n";
    }
}
