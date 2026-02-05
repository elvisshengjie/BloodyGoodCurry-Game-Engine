/*********************************************************************************************
 \file      ParticleSystem.h
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Primary Author, 100%
 \brief     Declares a lightweight particle system for one-off gameplay effects.
 \details   Spawns and updates short-lived particles for simple gameplay VFX such as
            enemy death bursts (circle-based) and player run trails (sprite-based).
            Uses Factory-managed lifetime and interpolates particle fade/size over time.
 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include "Common/System.h"
#include "Composition/Composition.h"
#include <glm/vec2.hpp>
#include <random>
#include <vector>

namespace Framework {

    
    class ParticleSystem : public ISystem {
    public:
        /*************************************************************************************
          \brief  Construct the particle system and initialize RNG.
          \details Sets the singleton instance pointer to this system.
        *************************************************************************************/
        ParticleSystem();

        /*************************************************************************************
          \brief  Initialize the particle system state.
          \details Clears all currently tracked particles.
        *************************************************************************************/
        void Initialize() override;

        /*************************************************************************************
          \brief  Update all active particles (movement, fade/size interpolation, cleanup).
          \param  dt  Delta time in seconds.
        *************************************************************************************/
        void Update(float dt) override;

        /*************************************************************************************
          \brief  Shutdown the particle system and clear all particles.
          \details Resets the singleton instance pointer if this system owns it.
        *************************************************************************************/
        void Shutdown() override;

        /*************************************************************************************
          \brief  Return the system name for the SystemManager.
        *************************************************************************************/
        std::string GetName() override { return "ParticleSystem"; }

        /*************************************************************************************
          \brief  Get the current ParticleSystem instance.
          \return Pointer to the ParticleSystem instance (or nullptr).
        *************************************************************************************/
        static ParticleSystem* Instance();

        /*************************************************************************************
          \brief  Spawn enemy death burst particles (circle-based).
          \param  worldPos  Spawn position in world space.
          \param  count     Number of particles to spawn.
        *************************************************************************************/
        void SpawnEnemyDeathParticles(const glm::vec2& worldPos, std::size_t count = 12);

        /*************************************************************************************
          \brief  Spawn run trail particles (sprite-based).
          \param  worldPos    Spawn position in world space.
          \param  facingDir   Facing direction sign (mirrors spawn/velocity).
          \param  count       Number of particles to spawn.
        *************************************************************************************/
        void SpawnRunParticles(const glm::vec2& worldPos, float facingDir, std::size_t count = 3);
    private:
        /*************************************************************************************
          \brief  Particle rendering mode (circle vs. sprite).
        *************************************************************************************/
        enum class ParticleVisual
        {
            Circle,
            Sprite
        };

        /*************************************************************************************
          \brief  Runtime particle metadata keyed by a Factory GOC ID.
        *************************************************************************************/
        struct Particle {
            GOCId id{};                                     
            ParticleVisual visual{ ParticleVisual::Circle }; 
            glm::vec2 velocity{};                            
            float life = 0.0f;                               
            float totalLife = 0.0f;                          
            float startRadius = 0.0f;                       
            float endRadius = 0.0f;                                    
            float startSize = 0.0f;                          
            float endSize = 0.0f;                            
            float startAlpha = 1.0f;                         
            float endAlpha = 0.0f;                           
        };

        std::vector<Particle> particles; 
        std::mt19937 rng;                

        static ParticleSystem* instance; 
    };

} // namespace Framework
