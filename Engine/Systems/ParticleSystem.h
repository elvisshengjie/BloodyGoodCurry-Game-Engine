/*********************************************************************************************
 \file      ParticleSystem.h
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Primary Author, 100%
 \brief     Declares a lightweight particle system for one-off particles.
 \details   Spawns and updates short-lived circle/sprite particles while the
            sandbox/game layer owns any effect presets built on top of it.
 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include "Common/System.h"
#include "Composition/Composition.h"
#include <glm/vec2.hpp>
#include <string>
#include <vector>

namespace Framework {

    
    class ParticleSystem : public ISystem {
    public:
        struct CircleParticleSpec
        {
            std::string objectName{ "Particle" };
            glm::vec2 position{};
            glm::vec2 velocity{};
            float life = 0.5f;
            float startRadius = 0.04f;
            float endRadius = 0.0f;
            float r = 1.0f;
            float g = 1.0f;
            float b = 1.0f;
            float startAlpha = 1.0f;
            float endAlpha = 0.0f;
        };

        struct SpriteParticleSpec
        {
            std::string objectName{ "Particle" };
            std::string textureKey;
            std::string texturePath;
            glm::vec2 position{};
            glm::vec2 velocity{};
            float life = 0.5f;
            float startSize = 0.05f;
            float endSize = 0.05f;
            float r = 1.0f;
            float g = 1.0f;
            float b = 1.0f;
            float startAlpha = 1.0f;
            float endAlpha = 0.0f;
        };

        /*************************************************************************************
          \brief  Construct the particle system.
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

        void SpawnCircleParticle(const CircleParticleSpec& spec);
        void SpawnSpriteParticle(const SpriteParticleSpec& spec);
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

        static ParticleSystem* instance; 
    };

} // namespace Framework
