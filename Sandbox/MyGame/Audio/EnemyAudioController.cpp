/*********************************************************************************************
 \file      EnemyAudioController.cpp
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Spatialised 3D audio controller for enemy hurt, attack, and death sounds.

 \copyright
            All content  2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "EnemyAudioController.h"

namespace mygame
{
    /*****************************************************************************************
      \brief Constructs the controller and buckets audio clips from the AudioComponent.
      \param audio AudioComponent pre-loaded with sound keys from the enemy's prefab JSON.
      \details
      Scans every registered sound key and sorts them into hurt, attack, and death pools
      by substring matching. This works for both melee (water) and ranged (fire) enemy
      prefabs without branching — the pool contents simply differ based on what the JSON
      declared. Logs a warning if the AudioComponent is null.
    *****************************************************************************************/

    EnemyAudioController::EnemyAudioController(Framework::AudioComponent* audio)
        : m_Audio(audio)
        , m_Rng(std::random_device{}())
    {
        if (!m_Audio)
        {
            std::cerr << "[EnemyAudioController] AudioComponent is null!\n";
            return;
        }

        // Scan every key the prefab JSON registered and bucket by substring.
        // Works for both water (melee) and fire (ranged) prefabs without any
        // branching — the pool contents simply differ based on what the JSON declared.
        for (const auto& [key, info] : m_Audio->GetSounds())
        {
            if (key.find("GhostHurt") != std::string::npos) m_HurtClips.push_back(key);
            else if (key.find("WaterGhostAttack") != std::string::npos) m_AttackClips.push_back(key);
            else if (key.find("FireGhostProjectile") != std::string::npos) m_AttackClips.push_back(key);
            else if (key.find("WaterGhostExplosion") != std::string::npos) m_DeathClips.push_back(key);
            else if (key.find("FireGhostExplosion") != std::string::npos) m_DeathClips.push_back(key);
        }
    }
    /*****************************************************************************************
      \brief Plays a named sound clip at a 3D world position.
      \param clip  Key of the sound clip to play.
      \param posX  World X position for 3D spatialisation.
      \param posY  World Y position for 3D spatialisation.
      \details
      Submits the clip to SoundManager as a 3D channel, then stores the returned channel ID
      in m_ActiveChannels so Update() can reposition it each frame. No-op if clip is empty.
    *****************************************************************************************/
    void EnemyAudioController::PlayClip3D(const std::string& clip, float posX, float posY)
    {
        if (clip.empty()) return;
        if (!m_Audio) return;

        const auto it = m_Audio->GetSounds().find(clip);
        if (it == m_Audio->GetSounds().end()) return;
        if (!SoundManager::getInstance().isSoundLoaded(it->second.id)) return;

        FMOD_VECTOR pos = { posX, posY, 0.0f };
        FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };

        const float playbackVolume = m_Audio->volume * it->second.volume;
        auto channelId = SoundManager::getInstance().playSound3DChannel(
            it->second.id, playbackVolume, 1.0f, it->second.loop, &pos, &vel);
        if (channelId != 0)
        {
            // Optional: store for per-frame update
            m_ActiveChannels.push_back(channelId);

            // Configure 3D spatial settings
            auto& audio = *SoundManager::getInstance().getAudioManager();
            audio.setChannel3DPosition(channelId, &pos, &vel);
        }
    }
    /*****************************************************************************************
    \brief Plays a randomly selected attack sound at the given world position.
    \param posX  World X position for 3D spatialisation.
    \param posY  World Y position for 3D spatialisation.
  *****************************************************************************************/

    void EnemyAudioController::PlayAttack(float posX, float posY)
    {
        std::string clip = GetRandom(m_AttackClips);
        PlayClip3D(clip, posX, posY);
    }
    /*****************************************************************************************
      \brief Plays a randomly selected hurt sound at the given world position.
      \param posX  World X position for 3D spatialisation.
      \param posY  World Y position for 3D spatialisation.
    *****************************************************************************************/
    void EnemyAudioController::PlayHurt(float posX, float posY)
    {
        std::string clip = GetRandom(m_HurtClips);
        PlayClip3D(clip, posX, posY);
    }
    /*****************************************************************************************
      \brief Plays a randomly selected death sound at the given world position.
      \param posX  World X position for 3D spatialisation.
      \param posY  World Y position for 3D spatialisation.
    *****************************************************************************************/
    void EnemyAudioController::PlayDeath(float posX, float posY)
    {
        std::string clip = GetRandom(m_DeathClips);
        PlayClip3D(clip, posX, posY);
    }

    /*****************************************************************************************
      \brief Per-frame update that repositions active 3D audio channels and prunes stopped ones.
      \param posX  Current world X position of the enemy.
      \param posY  Current world Y position of the enemy.
      \details
      Pushes the new position to every channel ID in m_ActiveChannels via SoundManager,
      then removes any channels that are no longer playing to prevent the list growing unbounded.
    *****************************************************************************************/
    void EnemyAudioController::Update(float posX, float posY)
    {
        FMOD_VECTOR enemyPos = { posX, posY, 0.0f };
        FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };

        // Update all active channels for this enemy
        for (auto channelId : m_ActiveChannels)
        {
            SoundManager::getInstance().setChannel3DPosition(channelId, &enemyPos, &vel);
        }

        // Optional: prune stopped channels
        m_ActiveChannels.erase(
            std::remove_if(m_ActiveChannels.begin(), m_ActiveChannels.end(),
                [](AudioManager::ChannelID id)
                {
                    return !SoundManager::getInstance().isChannelPlaying(id);
                }),
            m_ActiveChannels.end());
    }
    /*****************************************************************************************
      \brief Returns a uniformly random element from a string pool.
      \param pool Vector of sound clip keys to sample from.
      \return A randomly selected key, or an empty string if the pool is empty.
    *****************************************************************************************/
    std::string EnemyAudioController::GetRandom(const std::vector<std::string>& pool)
    {
        if (pool.empty()) return "";
        std::uniform_int_distribution<std::size_t> dist(0, pool.size() - 1);
        return pool[dist(m_Rng)];
    }

} 
