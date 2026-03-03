/*********************************************************************************************
 \file      EnemyAudioController.cpp
 \par       SofaSpuds
 \author    Choo Jian Wei - Primary Author

 \brief     Implementation of EnemyAudioController.

 \copyright
            All content  2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "EnemyAudioController.h"

namespace mygame
{
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

    void EnemyAudioController::PlayClip3D(const std::string& clip, float posX, float posY)
    {
        if (clip.empty()) return;

        FMOD_VECTOR pos = { posX, 0.0f, posY };
        FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };

        auto channelId = SoundManager::getInstance().playSound3DChannel(clip, 1.0f, 1.0f, false, &pos, &vel);
        if (channelId != 0)
        {
            // Optional: store for per-frame update
            m_ActiveChannels.push_back(channelId);

            // Configure 3D spatial settings
            auto& audio = *SoundManager::getInstance().getAudioManager();
            audio.setChannel3DPosition(channelId, &pos, &vel);
        }
    }
    // ---------------------------------------------------------------------------------
    // Play helpers
    // ---------------------------------------------------------------------------------

    void EnemyAudioController::PlayAttack(float posX, float posY)
    {
        std::string clip = GetRandom(m_AttackClips);
        PlayClip3D(clip, posX, posY);
    }

    void EnemyAudioController::PlayHurt(float posX, float posY)
    {
        std::string clip = GetRandom(m_HurtClips);
        PlayClip3D(clip, posX, posY);
    }

    void EnemyAudioController::PlayDeath(float posX, float posY)
    {
        std::string clip = GetRandom(m_DeathClips);
        PlayClip3D(clip, posX, posY);
    }

    // ---------------------------------------------------------------------------------
    // Per-frame 3D position update
    // ---------------------------------------------------------------------------------
    void EnemyAudioController::Update(float posX, float posY)
    {
        FMOD_VECTOR enemyPos = { posX, 0.0f, posY };
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

    // ---------------------------------------------------------------------------------
    // Internal
    // ---------------------------------------------------------------------------------

    std::string EnemyAudioController::GetRandom(const std::vector<std::string>& pool)
    {
        if (pool.empty()) return "";
        std::uniform_int_distribution<std::size_t> dist(0, pool.size() - 1);
        return pool[dist(m_Rng)];
    }

} 
