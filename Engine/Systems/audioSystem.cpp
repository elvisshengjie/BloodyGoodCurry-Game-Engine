/*********************************************************************************************
 \file      AudioSystem.cpp
 \par       SofaSpuds
 \author    Choo Jian Wei (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Implementation of the AudioSystem class, responsible for ticking the FMOD
            engine and keeping the spatial audio listener in sync with the player.

 \details
            AudioSystem integrates SoundManager and Resource_Manager to:
            - Initialise and shut down the audio backend.
            - Load all audio assets at startup from the game's Audio asset folder.
            - Each frame: update the FMOD listener position and tick SoundManager.
            - Render an ImGui-based debug panel in editor builds via AudioImGui.

            All game-specific audio logic (footstep timing, clip pool randomisation,
            enemy 3D sound tracking) is handled by game-side controllers in MyGame
            and is intentionally absent from this file.

 \copyright
            All content  2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "AudioSystem.h"
#include "Core/PathUtils.h"
#include "RenderSystem.h"
#include "Resource_Asset_Manager/Resource_Manager.h"
#include "Audio/SoundManager.h"
#include "Component/TransformComponent.h"
#include <filesystem>
#include <iostream>
#include "Common/CRTDebug.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif

namespace Framework
{
    /*****************************************************************************************
     \brief Construct with a reference to the main window (used for ImGui context).
    *****************************************************************************************/
    AudioSystem::AudioSystem(gfx::Window& window)
        : window(&window)
    {
    }

    /*****************************************************************************************
     \brief Initialise the audio engine and load all audio assets.

     \details
        1. Starts the SoundManager (FMOD) backend.
        2. Loads all audio files found under the game's Assets/Audio folder.
        3. In editor builds, initialises the AudioImGui debug panel.
    *****************************************************************************************/
    void AudioSystem::Initialize()
    {
        if (!SoundManager::getInstance().initialize())
        {
            std::cerr << "[AudioSystem] Failed to initialize SoundManager!\n";
            return;
        }

#if !SOFASPUDS_DISABLE_AUDIO
        namespace fs = std::filesystem;
        fs::path audioPath = Framework::ResolveAssetPath("Audio");
        if (!fs::exists(audioPath))
        {
            const fs::path fallbackAudioPath = Framework::ResolveAssetPath("Audio__OFF_WEB");
            if (fs::exists(fallbackAudioPath))
            {
                std::cout << "[AudioSystem] Using fallback audio folder: "
                          << fallbackAudioPath.string() << "\n";
                audioPath = fallbackAudioPath;
            }
        }

        Resource_Manager::loadAll(audioPath.string());
#endif

#if SOFASPUDS_ENABLE_EDITOR
        AudioImGui::Initialize(*window);
#endif

        std::cout << "[AudioSystem] Initialized successfully.\n";
    }

    /*****************************************************************************************
     \brief Per-frame update.

     \details
        1. Updates the FMOD listener position to the player's current world position
           so spatial audio attenuation is correct.
        2. Ticks SoundManager so FMOD processes channel events internally.

        No game-specific audio logic runs here. Footstep timing, enemy 3D position
        tracking, and clip-pool randomisation are handled by game-side controllers
        (PlayerAudioController, EnemyAudioController) in MyGame.

     \param dt  Delta time in seconds.
    *****************************************************************************************/
    void AudioSystem::Update(float dt)
    {
        (void)dt;

        UpdateListener();
        SoundManager::getInstance().update(dt);
    }

    /*****************************************************************************************
     \brief Locate the player and push its world position to the FMOD listener.

     \details
        Searches all active GameObjects for one that owns a PlayerComponent.
        Reads its TransformComponent and calls SoundManager::setListenerPos().
        If no player is found (e.g. during a loading screen) the listener is not moved.
    *****************************************************************************************/
    void AudioSystem::UpdateListener()
    {
        if (!FACTORY) return;
        GOC* listener = listenerQueryCallback ? listenerQueryCallback() : nullptr;
        if (!listener)
            return;

        auto* tr = listener->GetComponentType<TransformComponent>(
            ComponentTypeId::CT_TransformComponent);
        if (!tr)
            return;

        // Game is 2D — Z is fixed at 0. Forward points into the screen.
        float listenerPos[3] = { tr->x, tr->y, 0.0f };
        float forward[3] = { 0.0f, 0.0f, 1.0f };
        float up[3] = { 0.0f, 1.0f, 0.0f };

        SoundManager::getInstance().setListenerPos(listenerPos, forward, up);
    }

    /*****************************************************************************************
     \brief Render the ImGui audio debug panel (editor builds only).
    *****************************************************************************************/
    void AudioSystem::draw()
    {
#if SOFASPUDS_ENABLE_EDITOR
        if (!RenderSystem::IsEditorVisible())
            return;

        AudioImGui::Render();
#endif
    }

    /*****************************************************************************************
     \brief Shut down the audio engine and release all resources.

     \details
        - Unloads all sounds from SoundManager.
        - Shuts down the FMOD backend to release native allocations.
        - Clears cached sound entries from Resource_Manager.
        - In editor builds, shuts down AudioImGui.
    *****************************************************************************************/
    void AudioSystem::Shutdown()
    {
        SoundManager::getInstance().unloadAllSounds();
        SoundManager::getInstance().shutdown();
        Resource_Manager::unloadAll(Resource_Manager::Sound);

#if SOFASPUDS_ENABLE_EDITOR
        AudioImGui::Shutdown();
#endif

        std::cout << "[AudioSystem] Audio system shutdown completed.\n";
    }

} // namespace Framework
