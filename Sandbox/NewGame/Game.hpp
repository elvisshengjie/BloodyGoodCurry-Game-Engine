/*********************************************************************************************
 \file      Game.hpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Declares the minimal lifecycle interface for a generated game project.
 \details   Exposes the engine-facing callbacks used by Core and the editor bridge:
            initialization, update, draw, shutdown, focus handling, simulation control,
            and level loading from the editor.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include "Graphics/Window.hpp"
#include <filesystem>

namespace mygame
{
    /// \brief Initialize the generated game project's engine systems and bindings.
    void init(gfx::Window& win);
    /// \brief Advance the generated game project by one simulation tick.
    void update(float dt);
    /// \brief Draw the generated game project through the registered systems.
    void draw();
    /// \brief Shut down all systems and clear cached game pointers.
    void shutdown();
    /// \brief React to app focus changes by pausing audio and clearing input state.
    void onAppFocusChanged(bool suspended);

    /// \brief Report whether editor-driven simulation is currently running.
    bool IsEditorSimulationRunning();
    /// \brief Start editor-driven simulation for the generated game project.
    void EditorPlaySimulation();
    /// \brief Stop editor-driven simulation for the generated game project.
    void EditorStopSimulation();
    /// \brief Load a level selected from the editor into the active logic system.
    bool LoadLevelFromEditor(const std::filesystem::path& levelPath);
}
