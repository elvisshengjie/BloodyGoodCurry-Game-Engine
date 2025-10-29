#include "Graphics/Window.hpp"
#include "Systems/SystemManager.h"
#include "Systems/InputSystem.h"
#include "Systems/LogicSystem.h"
#include "Systems/PhysicSystem.h"
#include "Systems/RenderSystem.h"
#include "Systems/audioSystem.h"

#include "MainMenuPage.hpp"   // <-- correct include (MyGame folder)
#include "Debug/CrashLogger.hpp"
#include "Debug/Perf.h"
#include <GLFW/glfw3.h>
#include <chrono>

namespace mygame {

    namespace {
        using clock = std::chrono::high_resolution_clock;

        Framework::SystemManager gSystems;
        Framework::InputSystem* gInputSystem = nullptr;
        Framework::LogicSystem* gLogicSystem = nullptr;
        Framework::PhysicSystem* gPhysicsSystem = nullptr;
        Framework::AudioSystem* gAudioSystem = nullptr;
        Framework::RenderSystem* gRenderSystem = nullptr;

        enum class GameState { MAIN_MENU, PLAYING, EXIT };
        GameState currentState = GameState::MAIN_MENU;

        MainMenuPage mainMenu;
    }

    void init(gfx::Window& win)
    {
        gInputSystem = gSystems.RegisterSystem<Framework::InputSystem>(win);
        gLogicSystem = gSystems.RegisterSystem<Framework::LogicSystem>(win, *gInputSystem);
        gPhysicsSystem = gSystems.RegisterSystem<Framework::PhysicSystem>(*gLogicSystem);
        gAudioSystem = gSystems.RegisterSystem<Framework::AudioSystem>(win);
        gRenderSystem = gSystems.RegisterSystem<Framework::RenderSystem>(win, *gLogicSystem);
        gSystems.IntializeAll();

        mainMenu.Init(gRenderSystem->ScreenWidth(), gRenderSystem->ScreenHeight());
        currentState = GameState::MAIN_MENU;
    }

    void update(float dt)
    {
        const bool togglePerf = gInputSystem && gInputSystem->IsWindowKeyPressed(GLFW_KEY_F1);
        Framework::PerfFrameStart(dt, togglePerf);

        switch (currentState)
        {
        case GameState::MAIN_MENU:
            mainMenu.Update(gInputSystem);
            if (mainMenu.ConsumeStart()) currentState = GameState::PLAYING;
            if (mainMenu.ConsumeExit())  currentState = GameState::EXIT;
            break;

        case GameState::PLAYING:
            gSystems.UpdateAll(dt);
            break;

        case GameState::EXIT:
            if (gInputSystem) {
                if (auto* w = gInputSystem->Window()) w->close();  // guard to silence C6011
            }
            break;
        }

        Framework::setUpdate(0.0); // placeholder
    }

    void draw()
    {
        switch (currentState)
        {
        case GameState::MAIN_MENU:
            if (gRenderSystem) {
                gRenderSystem->BeginMenuFrame();
                mainMenu.Draw(gRenderSystem);     // draws menu.jpg + buttons
                gRenderSystem->EndMenuFrame();
            }
            break;

        case GameState::PLAYING:
            gSystems.DrawAll();                   // uses engine default background (house)
            break;

        case GameState::EXIT:
            break;
        }
    }

    void shutdown()
    {
        gSystems.ShutdownAll();
    }

} // namespace mygame
