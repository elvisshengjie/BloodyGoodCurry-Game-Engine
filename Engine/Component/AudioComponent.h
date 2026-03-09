/*********************************************************************************************
 \file      AudioComponent.h
 \par       SofaSpuds
 \author    Choo Jian Wei - Primary Author

 \brief     Declares the AudioComponent class, a generic engine-side component that manages
            sound registration and playback for any game entity.

 \details
            AudioComponent is a pure engine primitive. It knows nothing about players,
            enemies, or any game-specific concept. Its only responsibilities are:

            - Store metadata about each sound (id + loop flag) loaded from prefab JSON.
            - Track whether individual sounds are currently playing.
            - Provide Play / Stop / TriggerSound / IsPlaying methods that forward to SoundManager.
            - Expose GetSoundKeys() so game-side controllers can inspect what was loaded
              and build their own randomisation pools without hardcoding clip names.
            - Support serialization for prefab and level loading.
            - Support deep-copy (Clone) for object instancing.

            Game-specific logic (which clips belong to a player, footstep timing,
            random clip selection, enemy attack pools, etc.) belongs entirely in
            game-side controllers (PlayerAudioController, EnemyAudioController)
            that live in the MyGame project and call into this component's public API.

 \note      This file must never reference any MyGame headers or game-specific concepts.

 \copyright
            All content  2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include "Composition/Component.h"
#include "Memory/ComponentPool.h"
#include "Serialization/Serialization.h"
#include "Audio/SoundManager.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Framework
{
    /*****************************************************************************************
      \struct SoundInfo
      \brief  Lightweight metadata for a single registered sound.

      \var id    The asset identifier used by SoundManager to locate and play the sound.
      \var loop     Whether the sound should loop on playback.
      \var spatial  Whether the sound should be treated as spatial/3D audio.
    *****************************************************************************************/
    struct SoundInfo
    {
        std::string id;
        bool        loop{ false };
        bool spatial { false };
    };

    /*****************************************************************************************
      \class AudioComponent
      \brief Generic engine component that owns a named sound registry and drives playback.

      Sounds are registered entirely from the prefab JSON via Serialize(). Game-side
      controllers then call GetSoundKeys() to inspect what was loaded and sort keys into
      their own pools (footsteps, slashes, etc.) without any clip names being hardcoded
      in engine code.

      Public API:
        GetSoundKeys()          - iterate all registered keys (used by game-side controllers)
        HasSound()              - check if a key is registered
        AddSound()              - register a sound at runtime (optional, used by controllers)
        RemoveSound()           - unregister a sound
        ClearSounds()           - unregister all sounds
        Play()                  - play a registered sound (optionally 3D)
        Stop()                  - stop a registered sound
        TriggerSound()          - fire-and-forget play
        IsPlaying()             - query playback state
        UpdateSoundPosition()   - reposition a 3D sound (call each frame from game side)
    *****************************************************************************************/
    class AudioComponent : public GameComponent
    {
    public:

        float volume{ 1.0f };

    private:

        std::unordered_map<std::string, SoundInfo> m_sounds;   ///< Registered sound entries.
        std::unordered_map<std::string, bool>      m_playing;  ///< Per-sound playback state.

    public:

        // ---------------------------------------------------------------------------------
        // Construction / destruction
        // ---------------------------------------------------------------------------------

        AudioComponent() = default;

        ~AudioComponent() override
        {
            // Stop all active sounds on destruction to avoid orphaned FMOD channels.
            for (auto& [action, isPlaying] : m_playing)
            {
                if (isPlaying)
                {
                    auto it = m_sounds.find(action);
                    if (it != m_sounds.end())
                        SoundManager::getInstance().stopSound(it->second.id);
                }
            }
        }

        // ---------------------------------------------------------------------------------
        // Registry API
        // ---------------------------------------------------------------------------------

        /*************************************************************************************
          \brief Return all registered sound keys.

          \details  Called by game-side controllers in their initialize() to build
                    randomisation pools from whatever the prefab JSON declared, without
                    hardcoding any clip names in game code or engine code.

          \return   A vector of every logical action name currently in the registry.
        *************************************************************************************/
        std::vector<std::string> GetSoundKeys() const
        {
            std::vector<std::string> keys;
            keys.reserve(m_sounds.size());
            for (const auto& [key, info] : m_sounds)
                keys.push_back(key);
            return keys;
        }

        /*************************************************************************************
          \brief Return a const reference to the full sound registry.

          \details  Used by game-side controllers that need both the key and the SoundInfo
                    (e.g. to inspect the loop flag) when building pools. Prefer GetSoundKeys()
                    when only the key names are needed.

          \return   Const reference to the internal key -> SoundInfo map.
        *************************************************************************************/
        const std::unordered_map<std::string, SoundInfo>& GetSounds() const
        {
            return m_sounds;
        }

        /*************************************************************************************
          \brief Check whether a logical sound name is registered.
        *************************************************************************************/
        bool HasSound(const std::string& action) const
        {
            return m_sounds.find(action) != m_sounds.end();
        }

        /*************************************************************************************
          \brief Register a named sound entry at runtime.

          \details  Sounds are normally loaded from the prefab JSON via Serialize().
                    This method is available for controllers that need to register
                    additional sounds programmatically (e.g. dynamically loaded 3D clips).

          \param action  Logical name used as a lookup key.
          \param id      Asset identifier forwarded to SoundManager.
          \param loop     True if the sound should loop when played.
          \param spatial  True if the sound should be treated as spatial/3D audio.
        *************************************************************************************/
        void AddSound(const std::string& action, const std::string& id, bool loop = false, bool spatial = false)
        {
            m_sounds[action] = { id, loop, spatial };
            m_playing[action] = false;
        }

        /*************************************************************************************
          \brief Unregister a sound and stop it if currently playing.
        *************************************************************************************/
        void RemoveSound(const std::string& action)
        {
            auto it = m_sounds.find(action);
            if (it != m_sounds.end())
            {
                SoundManager::getInstance().stopSound(it->second.id);
                m_sounds.erase(it);
                m_playing.erase(action);
            }
        }

        /*************************************************************************************
          \brief Unregister all sounds, stopping any that are currently playing.
        *************************************************************************************/
        void ClearSounds()
        {
            for (auto& [action, isPlaying] : m_playing)
            {
                if (isPlaying)
                {
                    auto it = m_sounds.find(action);
                    if (it != m_sounds.end())
                        SoundManager::getInstance().stopSound(it->second.id);
                }
            }
            m_sounds.clear();
            m_playing.clear();
        }

        // ---------------------------------------------------------------------------------
        // Playback API
        // ---------------------------------------------------------------------------------

        /*************************************************************************************
          \brief Play a registered sound by its logical name.

          \param action  Logical name of the sound to play.
          \param posX    World X position (used only when is3D == true).
          \param posY    World Y position (used only when is3D == true).
          \param is3D    If true, sets the FMOD 3D position immediately after play.
        *************************************************************************************/
        void Play(const std::string& action,
            float posX = 0.0f, float posY = 0.0f,
            bool  is3D = false)
        {
            auto it = m_sounds.find(action);
            if (it == m_sounds.end())                                      return;
            if (!SoundManager::getInstance().isSoundLoaded(it->second.id)) return;

            SoundManager::getInstance().playSound(it->second.id, volume, 1.0f, it->second.loop);
            m_playing[action] = true;

            if (is3D)
            {
                float pos[3] = { posX, posY, 0.0f };
                float vel[3] = { 0.0f, 0.0f, 0.0f };
                SoundManager::getInstance().setSoundPos(it->second.id, pos, vel);
            }
        }

        /*************************************************************************************
          \brief Stop a currently active sound.

          \param action  Logical name of the sound to stop.
        *************************************************************************************/
        void Stop(const std::string& action)
        {
            auto it = m_sounds.find(action);
            if (it != m_sounds.end())
            {
                SoundManager::getInstance().stopSound(it->second.id);
                m_playing[action] = false;
            }
        }

        /*************************************************************************************
          \brief Fire-and-forget play. Does not modify playback-state tracking.

          \details  Intended for one-shot events (hit SFX, UI clicks, etc.) where the caller
                    does not need to query or stop the sound afterwards. Clip-pool selection
                    and randomisation are the caller's responsibility (see game-side controllers).

          \param action  Logical name of the sound to trigger.
          \param posX    World X position (used only when is3D == true).
          \param posY    World Y position (used only when is3D == true).
          \param is3D    If true, sets the FMOD 3D position immediately after play.
        *************************************************************************************/
        void TriggerSound(const std::string& action,
            float posX = 0.0f, float posY = 0.0f,
            bool  is3D = false)
        {
            Play(action, posX, posY, is3D);
        }

        /*************************************************************************************
          \brief Query whether a named sound is currently marked as playing.

          \param action  Logical name to query.
          \return True if playing, false if stopped or not registered.
        *************************************************************************************/
        bool IsPlaying(const std::string& action) const
        {
            auto it = m_playing.find(action);
            return (it != m_playing.end()) && it->second;
        }

        /*************************************************************************************
          \brief Update the 3D position of a specific registered sound.

          \details  Call each frame from a game-side controller or AudioSystem for any
                    sound that should track a moving entity.

          \param action  Logical name of the sound to reposition.
          \param posX    New world X position.
          \param posY    New world Y position.
          \param velX    X velocity for Doppler simulation (pass 0 if unused).
          \param velY    Y velocity for Doppler simulation (pass 0 if unused).
        *************************************************************************************/
        void UpdateSoundPosition(const std::string& action,
            float posX, float posY,
            float velX = 0.0f, float velY = 0.0f)
        {
            auto it = m_sounds.find(action);
            if (it == m_sounds.end()) return;

            float pos[3] = { posX, posY, 0.0f };
            float vel[3] = { velX, velY, 0.0f };
            SoundManager::getInstance().setSoundPos(it->second.id, pos, vel);
        }

        // ---------------------------------------------------------------------------------
        // GameComponent interface
        // ---------------------------------------------------------------------------------

        /*************************************************************************************
          \brief No engine-side initialization logic.

          \details  Sound registration happens via Serialize() (prefab JSON load).
                    Game-side controllers call GetSoundKeys() after this to build pools.
        *************************************************************************************/
        void initialize() override {}

        /*************************************************************************************
          \brief No per-frame engine-side audio logic.

          \details  All per-frame behaviour (footstep timing, 3D position sync, etc.)
                    is handled by game-side controllers. This override exists solely to
                    satisfy the GameComponent interface.
        *************************************************************************************/
        void Update(float dt){ (void)dt; }

        // ---------------------------------------------------------------------------------
        // Serialization
        // ---------------------------------------------------------------------------------

        /*************************************************************************************
          \brief Populate the sound registry from prefab JSON data.

          \details  The JSON "sounds" block is the single source of truth for which clips
                    this component owns. Each key in the block becomes a logical action name;
                    its "id" and "loop" fields populate the SoundInfo entry.

          \param s  Reference to the serializer.
        *************************************************************************************/
        void Serialize(ISerializer& s) override
        {
            if (s.HasKey("volume"))
                StreamRead(s, "volume", volume);

            if (s.EnterObject("sounds"))  // ← this was missing
            {
                m_sounds.clear();
                m_playing.clear();

                for (const auto& action : s.CurrentKeys())
                {
                    if (!s.EnterObject(action))
                        continue;

                    SoundInfo info{};
                    StreamRead(s, "id", info.id);
                    if (s.HasKey("loop"))    StreamRead(s, "loop", info.loop);
                    if (s.HasKey("spatial")) StreamRead(s, "spatial", info.spatial);

                    m_sounds[action] = std::move(info);
                    m_playing[action] = false;
                    s.ExitObject();
                }
                s.ExitObject();  // ← and this
            }
        }

        // ---------------------------------------------------------------------------------
        // Cloning
        // ---------------------------------------------------------------------------------

        /*************************************************************************************
          \brief Deep-copy this component for prefab instancing.

          \details  Copies the full sound registry and volume. Game-side controllers
                    will call GetSoundKeys() and rebuild their pools in their own
                    initialize() after being attached to the cloned object.

          \return   A ComponentHandle owning the new AudioComponent copy.
        *************************************************************************************/
        ComponentHandle Clone() const override
        {
            auto copy = ComponentPool<AudioComponent>::CreateTyped();
            copy->volume = volume;
            copy->m_sounds = m_sounds;
            copy->m_playing = m_playing;
            return copy;
        }
    };

} // namespace Framework
