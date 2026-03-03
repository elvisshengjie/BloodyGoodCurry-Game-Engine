/*********************************************************************************************
 \file      PlayerAudioController.cpp
 \par       MyGame
 \author    Choo Jian Wei - Primary Author

 \brief     Implementation of PlayerAudioController.

 \copyright
            All content  2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "PlayerAudioController.h"
#include <iostream>

namespace mygame
{
    PlayerAudioController::PlayerAudioController(Framework::AudioComponent* audio)
        : m_Audio(audio)
        , m_Rng(std::random_device{}())
    {
        if (!m_Audio)
        {
            std::cerr << "[PlayerAudioController] AudioComponent is null!\n";
            return;
        }

        // Scan every key the prefab JSON registered and bucket by substring.
        // No clip names are hardcoded here — the JSON is the source of truth.
        for (const auto& [key, info] : m_Audio->GetSounds())
        {
            if (key.find("ConcreteFootsteps") != std::string::npos) m_Footsteps.push_back(key);
            else if (key.find("Slash") != std::string::npos) m_Slashes.push_back(key);
            else if (key.find("Punch") != std::string::npos) m_Punches.push_back(key);
            else if (key.find("GrappleShoot") != std::string::npos) m_Grapples.push_back(key);
            else if (key.find("IneffectiveBoink") != std::string::npos) m_Boinks.push_back(key);
            // "PlayerHit" / "PlayerDead" are called by exact name — no pool needed.
        }
    }

    // ---------------------------------------------------------------------------------
    // Play helpers
    // ---------------------------------------------------------------------------------

    void PlayerAudioController::PlayFootstep()
    {
        std::string clip = GetRandom(m_Footsteps);
        if (!clip.empty()) m_Audio->Play(clip);
    }

    void PlayerAudioController::PlaySlash()
    {
        std::string clip = GetRandom(m_Slashes);
        if (!clip.empty()) m_Audio->Play(clip);
    }

    void PlayerAudioController::PlayPunch()
    {
        std::string clip = GetRandom(m_Punches);
        if (!clip.empty()) m_Audio->Play(clip);
    }

    void PlayerAudioController::PlayGrapple()
    {
        std::string clip = GetRandom(m_Grapples);
        if (!clip.empty()) m_Audio->Play(clip);
    }

    void PlayerAudioController::PlayBoink()
    {
        std::string clip = GetRandom(m_Boinks);
        if (!clip.empty()) m_Audio->Play(clip);
    }

    void PlayerAudioController::PlayPlayerHit()
    {
        m_Audio->Play("PlayerHit");
    }

    void PlayerAudioController::PlayPlayerDead()
    {
        m_Audio->Play("PlayerDead");
    }

    // ---------------------------------------------------------------------------------
    // Internal
    // ---------------------------------------------------------------------------------

    std::string PlayerAudioController::GetRandom(const std::vector<std::string>& pool)
    {
        if (pool.empty()) return "";
        std::uniform_int_distribution<std::size_t> dist(0, pool.size() - 1);
        return pool[dist(m_Rng)];
    }

} // namespace MyGame
