/*********************************************************************************************
 \file      EnemyAudioController.h
 \par       SofaSpuds
 \author    Choo Jian Wei (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Immplementation ofGame-side controller that organises enemy audio pools and drives the engine-side
            AudioComponent.

 \details
            Sounds are declared entirely in each enemy's prefab JSON under "AudioComponent".
            This controller does NOT hardcode any clip names. On construction it calls
            AudioComponent::GetSounds() and buckets every key by substring match:

              Water (melee) prefab keys:
                Attack pool  — "WaterGhostAttack"
                Hurt pool    — "GhostHurt1" .. "GhostHurt8"
                Death pool   — "WaterGhostExplosion1"

              Fire (ranged) prefab keys:
                Attack pool  — "FireGhostProjectile1", "FireGhostProjectile2"
                Hurt pool    — "GhostHurt1" .. "GhostHurt8"
                Death pool   — "FireGhostExplosion"

            Both enemy types share the same controller — the pools are simply populated
            from whatever the prefab JSON declares. Adding a new enemy type requires
            only a new prefab JSON with matching key prefixes; no code changes needed.

            Responsibilities:
            - Build attack / hurt / death pools from loaded sound keys.
            - Expose PlayAttack / PlayHurt / PlayDeath helpers for behaviour scripts.
            - Optionally update 3D sound positions each frame via Update().

 \note      Must only be compiled as part of MyGame.vcxproj.
            Namespace is MyGame, not Framework.

 \copyright
            All content  2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include "Component/AudioComponent.h"
#include <vector>
#include <string>
#include <random>
#include <iostream>

namespace mygame
{
    class EnemyAudioController
    {
    public:


        explicit EnemyAudioController(Framework::AudioComponent* audio);
        void PlayClip3D(const std::string& clip, float posX, float posY);
        void PlayAttack(float posX = 0.0f, float posY = 0.0f);
        void PlayHurt(float posX = 0.0f, float posY = 0.0f);
        void PlayDeath(float posX = 0.0f, float posY = 0.0f);
        void Update(float posX, float posY);

    private:

        Framework::AudioComponent* m_Audio{ nullptr };

        // ---- Clip pools (populated from JSON keys in constructor) ----------------------
        std::vector<std::string> m_AttackClips;  // "WaterGhostAttack" | "FireGhostProjectile*"
        std::vector<std::string> m_HurtClips;    // "GhostHurt*"
        std::vector<std::string> m_DeathClips;   // "WaterGhostExplosion*" | "FireGhostExplosion"

        // ---- RNG -----------------------------------------------------------------------
        std::mt19937 m_Rng;
       
        // Active 3D sound channels
        std::vector<AudioManager::ChannelID> m_ActiveChannels;
        /*************************************************************************************
          \brief Return a uniformly random element from a pool.

          \param pool  The clip pool to sample from.
          \return      A random key string, or an empty string if the pool is empty.
        *************************************************************************************/
        std::string GetRandom(const std::vector<std::string>& pool);
    };

} 
