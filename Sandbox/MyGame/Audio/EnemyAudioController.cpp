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

namespace Framework
{
    EnemyAudioController::EnemyAudioController(std::shared_ptr<Framework::AudioComponent> audio)
        : m_Audio(std::move(audio))
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

    // ---------------------------------------------------------------------------------
    // Play helpers
    // ---------------------------------------------------------------------------------

    void EnemyAudioController::PlayAttack()
    {
        std::string clip = GetRandom(m_AttackClips);
        if (!clip.empty()) m_Audio->Play(clip);
    }

    void EnemyAudioController::PlayHurt()
    {
        std::string clip = GetRandom(m_HurtClips);
        if (!clip.empty()) m_Audio->Play(clip);
    }

    void EnemyAudioController::PlayDeath()
    {
        std::string clip = GetRandom(m_DeathClips);
        if (!clip.empty()) m_Audio->Play(clip);
    }

    // ---------------------------------------------------------------------------------
    // Per-frame 3D position update
    // ---------------------------------------------------------------------------------

    void EnemyAudioController::Update(float posX, float posY)
    {
        // Reposition every active sound so FMOD spatial audio tracks the enemy.
        for (const auto& clip : m_AttackClips)
            if (m_Audio->IsPlaying(clip))
                m_Audio->UpdateSoundPosition(clip, posX, posY);

        for (const auto& clip : m_HurtClips)
            if (m_Audio->IsPlaying(clip))
                m_Audio->UpdateSoundPosition(clip, posX, posY);

        for (const auto& clip : m_DeathClips)
            if (m_Audio->IsPlaying(clip))
                m_Audio->UpdateSoundPosition(clip, posX, posY);
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