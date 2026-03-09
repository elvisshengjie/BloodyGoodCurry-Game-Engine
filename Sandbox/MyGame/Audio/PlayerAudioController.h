/*********************************************************************************************
 \file      PlayerAudioController.h
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Decleration of Game-side controller that organises player audio pools and drives the engine-side
            AudioComponent.

 \details
            Sounds are declared entirely in the player prefab JSON under "AudioComponent".
            This controller does NOT hardcode any clip names. On construction it calls
            AudioComponent::GetSounds() and buckets every key by substring match into the
            correct pool (footsteps, slashes, punches, grapples, boinks).

            From that point on it owns:
            - Randomisation via a seeded Mersenne-Twister (no raw rand()).
            - Named play helpers called by player behaviour scripts:
                PlayFootstep / PlaySlash / PlayPunch / PlayGrapple / PlayBoink /
                PlayPlayerHit / PlayPlayerDead

            To change which sounds the player uses, edit the prefab JSON.
            To change when they play, edit the behaviour script.
            Neither requires touching the engine.

 \note      Must only be compiled as part of MyGame.vcxproj.
            Namespace is MyGame, not Framework.

 \copyright
            All content  2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include "Composition/Composition.h"
#include "Component/AudioComponent.h"
#include <vector>
#include <string>
#include <random>
#include <iostream>

namespace mygame
{
    /*************************************************************************************
      \class PlayerAudioController
      \brief Owns player sound pools and named play helpers.

      Constructed with a pointer to the player's AudioComponent. Pools are built
      immediately in the constructor by scanning the already-loaded sound keys.
    *************************************************************************************/
    class PlayerAudioController
    {
    public:
        explicit PlayerAudioController(Framework::AudioComponent* audio);
        void PlayFootstep();
        void PlaySlash();
        void PlayPunch();
        void PlayGrapple();
        void PlayBoink();
        void PlayPlayerHit();
        void PlayPlayerDead();

    private:

        Framework::AudioComponent* m_Audio{ nullptr };

        // ---- Clip pools (populated from JSON keys in constructor) ----------------------
        std::vector<std::string> m_Footsteps;   // prefix: "ConcreteFootsteps"
        std::vector<std::string> m_Slashes;      // prefix: "Slash"
        std::vector<std::string> m_Punches;      // prefix: "Punch"
        std::vector<std::string> m_Grapples;     // prefix: "GrappleShoot"
        std::vector<std::string> m_Boinks;       // prefix: "IneffectiveBoink"

        // ---- RNG -----------------------------------------------------------------------
        std::mt19937 m_Rng;
        std::string GetRandom(const std::vector<std::string>& pool);
    };

} 
