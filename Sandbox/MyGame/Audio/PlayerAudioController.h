/*********************************************************************************************
 \file      PlayerAudioController.h
 \par       SofaSpuds
 \author    Choo Jian Wei - Primary Author

 \brief     Game-side controller that organises player audio pools and drives the engine-side
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

        /*************************************************************************************
          \brief Construct and build all sound pools from the AudioComponent's loaded keys.

          \param audio  Pointer to the sibling AudioComponent on the player object.
                        Must not be null; asserted in debug.
        *************************************************************************************/
        explicit PlayerAudioController(Framework::AudioComponent* audio);

        // ---------------------------------------------------------------------------------
        // Play helpers  (called by player behaviour scripts)
        // ---------------------------------------------------------------------------------

        /// Play a random concrete footstep clip.
        void PlayFootstep();

        /// Play a random slash-on-enemy clip.
        void PlaySlash();

        /// Play a random air-slash / punch clip.
        void PlayPunch();

        /// Play a random grapple-shoot clip.
        void PlayGrapple();

        /// Play a random ineffective-hit (boink) clip.
        void PlayBoink();

        /// Play the player-hit (damage received) sound.
        void PlayPlayerHit();

        /// Play the player-death sound.
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

        /*************************************************************************************
          \brief Return a uniformly random element from a pool.

          \param pool  The clip pool to sample from.
          \return      A random key string, or an empty string if the pool is empty.
        *************************************************************************************/
        std::string GetRandom(const std::vector<std::string>& pool);
    };

} 
