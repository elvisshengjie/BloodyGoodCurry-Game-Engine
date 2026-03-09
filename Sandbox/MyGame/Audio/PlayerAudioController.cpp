/*********************************************************************************************
 \file      PlayerAudioController.cpp
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Implementation of PlayerAudioController.

 \copyright
            All content  2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "PlayerAudioController.h"
#include <iostream>

namespace mygame
{
    /*****************************************************************************************
      \brief Constructs the controller and buckets audio clip keys from the AudioComponent.
      \param audio AudioComponent pre-loaded with sound keys from the player's prefab JSON.
      \details
      Scans every registered sound key and sorts them into footstep, slash, punch, grapple,
      and boink pools by substring matching. No clip names are hardcoded — the JSON is the
      source of truth. PlayerHit and PlayerDead are played by exact key and need no pool.
      Logs a warning if the AudioComponent is null.
    *****************************************************************************************/

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

    /*****************************************************************************************
      \brief Plays a randomly selected footstep clip.
      \details Samples m_Footsteps; no-op if the pool is empty.
    *****************************************************************************************/

    void PlayerAudioController::PlayFootstep()
    {
        std::string clip = GetRandom(m_Footsteps);
        if (!clip.empty()) m_Audio->Play(clip);
    }
    /*****************************************************************************************
      \brief Plays a randomly selected slash clip.
      \details Samples m_Slashes; no-op if the pool is empty.
    *****************************************************************************************/
    void PlayerAudioController::PlaySlash()
    {
        std::string clip = GetRandom(m_Slashes);
        if (!clip.empty()) m_Audio->Play(clip);
    }
    /*****************************************************************************************
      \brief Plays a randomly selected punch clip.
      \details Samples m_Punches; no-op if the pool is empty.
    *****************************************************************************************/
    void PlayerAudioController::PlayPunch()
    {
        std::string clip = GetRandom(m_Punches);
        if (!clip.empty()) m_Audio->Play(clip);
    }
    /*****************************************************************************************
      \brief Plays a randomly selected grapple/throw clip.
      \details Samples m_Grapples; no-op if the pool is empty.
    *****************************************************************************************/
    void PlayerAudioController::PlayGrapple()
    {
        std::string clip = GetRandom(m_Grapples);
        if (!clip.empty()) m_Audio->Play(clip);
    }
    /*****************************************************************************************
      \brief Plays a randomly selected boink clip.
      \details Samples m_Boinks; no-op if the pool is empty.
    *****************************************************************************************/
    void PlayerAudioController::PlayBoink()
    {
        std::string clip = GetRandom(m_Boinks);
        if (!clip.empty()) m_Audio->Play(clip);
    }
    /*****************************************************************************************
    \brief Plays the player hurt sound by exact key "PlayerHit".
    *****************************************************************************************/
    void PlayerAudioController::PlayPlayerHit()
    {
        m_Audio->Play("PlayerHit");
    }
    /*****************************************************************************************
      \brief Plays the player death sound by exact key "PlayerDead".
    *****************************************************************************************/
    void PlayerAudioController::PlayPlayerDead()
    {
        m_Audio->Play("PlayerDead");
    }

    /*****************************************************************************************
      \brief Returns a uniformly random element from a string pool.
      \param pool Vector of sound clip keys to sample from.
      \return A randomly selected key, or an empty string if the pool is empty.
    *****************************************************************************************/
    std::string PlayerAudioController::GetRandom(const std::vector<std::string>& pool)
    {
        if (pool.empty()) return "";
        std::uniform_int_distribution<std::size_t> dist(0, pool.size() - 1);
        return pool[dist(m_Rng)];
    }

} // namespace mygame
