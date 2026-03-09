/*********************************************************************************************
 \file      Game.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Implements a minimal generated game-project lifecycle.
 \details   Registers core engine systems, wires the generated bootstrap hooks, supports
            editor simulation control, and keeps focus/audio/input handling functional
            for fresh projects without carrying over BloodyGoodCurry-specific logic.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "Game.hpp"

#include "EngineCall.hpp"

#include "Audio/SoundManager.h"
#include "Factory/Factory.h"
#include "Systems/InputSystem.h"
#include "Systems/LogicSystem.h"
#include "Systems/PhysicSystem.h"
#include "Systems/RenderSystem.h"
#include "Systems/SystemManager.h"
#include "Systems/audioSystem.h"

namespace mygame
{
    namespace
    {
        Framework::SystemManager gSystems;
        Framework::InputSystem* gInputSystem = nullptr;
        Framework::LogicSystem* gLogicSystem = nullptr;
        Framework::PhysicSystem* gPhysicsSystem = nullptr;
        Framework::AudioSystem* gAudioSystem = nullptr;
        Framework::RenderSystem* gRenderSystem = nullptr;
        bool gEditorSimulationRunning = false;
    }

    /*************************************************************************************
     \brief  Initialize the generated game project and register its engine systems.
     \param  win  Main application window used by window-dependent systems.
    *************************************************************************************/
    void init(gfx::Window& win)
    {
        gInputSystem = gSystems.RegisterSystem<Framework::InputSystem>(win);
        gLogicSystem = gSystems.RegisterSystem<Framework::LogicSystem>(win, *gInputSystem);
        ConfigureGameBootstrap(*gLogicSystem);
        RegisterMyGameScripts(*gLogicSystem);

        gPhysicsSystem = gSystems.RegisterSystem<Framework::PhysicSystem>();
        gAudioSystem = gSystems.RegisterSystem<Framework::AudioSystem>(win);
        gRenderSystem = gSystems.RegisterSystem<Framework::RenderSystem>(win, *gLogicSystem);
        ConfigureRenderBootstrap(*gRenderSystem);

        gSystems.IntializeAll();

        if (gAudioSystem)
        {
            gAudioSystem->SetListenerQueryCallback([]() -> Framework::GOC*
            {
                return gLogicSystem ? gLogicSystem->FindAnyAlivePlayer() : nullptr;
            });
        }

        gEditorSimulationRunning = false;
    }

    /*************************************************************************************
     \brief  Update the generated game project for one frame.
     \param  dt  Delta time in seconds.
     \details Runs all systems during gameplay or editor simulation, but when the editor
              is open and simulation is stopped it only refreshes input state.
    *************************************************************************************/
    void update(float dt)
    {
        const bool editorVisible = Framework::RenderSystem::IsEditorVisible();
        if (!editorVisible || gEditorSimulationRunning)
        {
            gSystems.UpdateAll(dt);
            return;
        }

        if (gInputSystem)
            gInputSystem->Update(dt);
    }

    /*************************************************************************************
     \brief  Draw the generated game project through the registered systems.
    *************************************************************************************/
    void draw()
    {
        gSystems.DrawAll();
    }

    /*************************************************************************************
     \brief  Shut down all systems and clear cached subsystem pointers.
    *************************************************************************************/
    void shutdown()
    {
        gSystems.ShutdownAll();
        gRenderSystem = nullptr;
        gAudioSystem = nullptr;
        gPhysicsSystem = nullptr;
        gLogicSystem = nullptr;
        gInputSystem = nullptr;
    }

    /*************************************************************************************
     \brief  Pause or resume runtime audio and clear transient input state on focus changes.
     \param  suspended  True when the app loses focus, false when focus is restored.
    *************************************************************************************/
    void onAppFocusChanged(bool suspended)
    {
        SoundManager::getInstance().pauseAllSounds(suspended);
        if (gInputSystem)
            gInputSystem->Manager().ClearState();
    }

    /*************************************************************************************
     \brief  Report whether editor-driven simulation is currently active.
     \return True when the generated project is simulating from the editor.
    *************************************************************************************/
    bool IsEditorSimulationRunning()
    {
        return gEditorSimulationRunning;
    }

    /*************************************************************************************
     \brief  Start editor-driven simulation for the generated project.
    *************************************************************************************/
    void EditorPlaySimulation()
    {
        gEditorSimulationRunning = true;
        if (Framework::FACTORY)
            Framework::FACTORY->Layers().LogVisibilitySummary("EditorPlaySimulation");
    }

    /*************************************************************************************
     \brief  Stop editor-driven simulation for the generated project.
    *************************************************************************************/
    void EditorStopSimulation()
    {
        gEditorSimulationRunning = false;
        if (Framework::FACTORY)
            Framework::FACTORY->Layers().LogVisibilitySummary("EditorStopSimulation");
    }

    /*************************************************************************************
     \brief  Load a level selected from the editor into the active logic system.
     \param  levelPath  Absolute or project-relative path to the selected level file.
     \return True if the level request is valid and forwarded to LogicSystem.
    *************************************************************************************/
    bool LoadLevelFromEditor(const std::filesystem::path& levelPath)
    {
        if (!gLogicSystem || levelPath.empty())
            return false;

        gLogicSystem->LoadLevel(levelPath);
        return true;
    }
}
