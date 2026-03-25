/*********************************************************************************************
 \file      Game.hpp
 \par       SofaSpuds
 \author    All TEAM MEMBERS

 \brief     Public interface of the sandbox game layer. It exposes the lifecycle
            entry points used by the engine:
              â€¢ init(gfx::Window&): Hook up the Window, create the GameObjectFactory,
                load prefabs/level data, initialize Graphics, ImGui, text, and audio.
              â€¢ update(float dt): Handle input, advance animation/physics, process the
                factory sweep, and record per-stage CPU timings for the profiler.
              â€¢ draw(): Render background, sprites/rects/circles and on-screen text,
                build debug UIs (Spawn, Crash Tests, Performance), and submit ImGui.
              â€¢ shutdown(): Tear down audio/graphics resources, unload prefabs, and
                destroy ImGui/context objects safely.

            Audio helpers (initializeAudio / cleanupAudio) wrap SoundManager start/stop.

 \copyright
            All content Â© 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once
#include "Graphics/Window.hpp"
#include <filesystem>

namespace mygame {

    // Game lifecycle
    void init(gfx::Window& win);
    void update(float dt);
    void draw();
    void shutdown();
    void onAppFocusChanged(bool suspended);
    // Editor simulation controls
    bool IsEditorSimulationRunning();
    void EditorPlaySimulation();
    void EditorStopSimulation();
    /// Queue an incremental reload of the current gameplay level with the transition video.
    bool RequestReloadLevel(bool hideGameplayUntilDelay = false);
    /// Queue an incremental load of a new gameplay level with the transition video.
    bool RequestLoadLevel(const std::filesystem::path& levelPath);
    /// Queue an incremental level load from editor UI without showing the transition video.
    bool LoadLevelFromEditor(const std::filesystem::path& levelPath);
    /// Request that gameplay switch into the pause menu.
    bool RequestPauseMenu();
    /// Returns true when gameplay scripts should ignore player-driven input for the current frame.
    bool IsGameplayInputBlocked();

}
