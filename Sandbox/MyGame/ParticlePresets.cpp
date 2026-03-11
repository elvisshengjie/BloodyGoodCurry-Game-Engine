/*********************************************************************************************
 \file      ParticlePresets.cpp
 \par       SofaSpuds
 \author
 \brief     Implements sandbox-specific particle effect presets.
 \details   Defines reusable game-layer particle bursts and trails built on top of the
            engine's generic ParticleSystem spawn primitives.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "ParticlePresets.hpp"

#include "Systems/ParticleSystem.h"

#include <cmath>
#include <random>

namespace mygame {
    namespace
    {
        /*************************************************************************************
          \brief Builds the default enemy-death particle preset values.
          \return Default-initialized enemy death particle settings.
        *************************************************************************************/
        EnemyDeathParticlePreset DefaultEnemyDeathParticlePreset()
        {
            return {};
        }

        /*************************************************************************************
          \brief Builds the default run-trail particle preset values.
          \return Default-initialized running particle settings.
        *************************************************************************************/
        RunParticlePreset DefaultRunParticlePreset()
        {
            return {};
        }

        EnemyDeathParticlePreset gEnemyDeathPreset = DefaultEnemyDeathParticlePreset();
        RunParticlePreset gRunParticlePreset = DefaultRunParticlePreset();
    }

    /*************************************************************************************
      \brief Returns the mutable enemy-death preset used by the current game.
      \return Reference to the shared enemy-death particle preset.
    *************************************************************************************/
    EnemyDeathParticlePreset& GetEnemyDeathParticlePreset()
    {
        return gEnemyDeathPreset;
    }

    /*************************************************************************************
      \brief Returns the mutable running particle preset used by the current game.
      \return Reference to the shared run particle preset.
    *************************************************************************************/
    RunParticlePreset& GetRunParticlePreset()
    {
        return gRunParticlePreset;
    }

    /*************************************************************************************
      \brief Restores the enemy-death preset back to its default values.
    *************************************************************************************/
    void ResetEnemyDeathParticlePreset()
    {
        gEnemyDeathPreset = DefaultEnemyDeathParticlePreset();
    }

    /*************************************************************************************
      \brief Restores the running particle preset back to its default values.
    *************************************************************************************/
    void ResetRunParticlePreset()
    {
        gRunParticlePreset = DefaultRunParticlePreset();
    }

    /*************************************************************************************
      \brief Spawns the current enemy-death particle burst at a world position.
      \param particleSystem Active engine particle system.
      \param worldPos       World position used as the burst origin.
      \param count          Optional override for particle count. Uses preset count when 0.
    *************************************************************************************/
    void SpawnEnemyDeathParticles(
        Framework::ParticleSystem& particleSystem,
        const glm::vec2& worldPos,
        std::size_t count)
    {
        const EnemyDeathParticlePreset& preset = gEnemyDeathPreset;
        if (count == 0)
            count = preset.count;

        if (count == 0)
            return;

        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> angleDist(0.0f, 6.283185f);
        std::uniform_real_distribution<float> speedDist(
            std::min(preset.speedMin, preset.speedMax),
            std::max(preset.speedMin, preset.speedMax));
        std::uniform_real_distribution<float> lifeDist(
            std::min(preset.lifeMin, preset.lifeMax),
            std::max(preset.lifeMin, preset.lifeMax));
        std::uniform_real_distribution<float> radiusDist(
            std::min(preset.radiusMin, preset.radiusMax),
            std::max(preset.radiusMin, preset.radiusMax));
        std::uniform_real_distribution<float> greenJitterDist(
            std::min(preset.greenJitterMin, preset.greenJitterMax),
            std::max(preset.greenJitterMin, preset.greenJitterMax));

        for (std::size_t i = 0; i < count; ++i)
        {
            const float baseRadius = radiusDist(rng);
            const float angle = angleDist(rng);
            const float speed = speedDist(rng);

            Framework::ParticleSystem::CircleParticleSpec spec;
            spec.objectName = "EnemyDeathParticle";
            spec.position = worldPos;
            spec.velocity = {
                std::cos(angle) * speed,
                std::sin(angle) * speed + preset.upwardVelocityBias
            };
            spec.life = lifeDist(rng);
            spec.startRadius = baseRadius;
            spec.endRadius = baseRadius * preset.endRadiusScale;
            spec.r = preset.red;
            spec.g = preset.green + greenJitterDist(rng);
            spec.b = preset.blue;
            spec.startAlpha = preset.startAlpha;
            spec.endAlpha = preset.endAlpha;

            particleSystem.SpawnCircleParticle(spec);
        }
    }

    /*************************************************************************************
      \brief Spawns the current running-trail particles behind the player.
      \param particleSystem Active engine particle system.
      \param worldPos       World-space origin near the runner.
      \param facingDir      Horizontal facing direction used to flip the spawn bias.
      \param count          Optional override for particle count. Uses preset count when 0.
    *************************************************************************************/
    void SpawnRunParticles(
        Framework::ParticleSystem& particleSystem,
        const glm::vec2& worldPos,
        float facingDir,
        std::size_t count)
    {
        const RunParticlePreset& preset = gRunParticlePreset;
        if (count == 0)
            count = preset.count;

        if (count == 0)
            return;

        static std::mt19937 rng(std::random_device{}());
        const float dir = (facingDir >= 0.0f) ? 1.0f : -1.0f;
        std::uniform_real_distribution<float> speedDist(
            std::min(preset.speedMin, preset.speedMax),
            std::max(preset.speedMin, preset.speedMax));
        std::uniform_real_distribution<float> lifeDist(
            std::min(preset.lifeMin, preset.lifeMax),
            std::max(preset.lifeMin, preset.lifeMax));
        std::uniform_real_distribution<float> sizeDist(
            std::min(preset.sizeMin, preset.sizeMax),
            std::max(preset.sizeMin, preset.sizeMax));
        std::uniform_real_distribution<float> jitterDist(
            std::min(preset.jitterMin, preset.jitterMax),
            std::max(preset.jitterMin, preset.jitterMax));
        std::uniform_real_distribution<float> riseDist(
            std::min(preset.riseMin, preset.riseMax),
            std::max(preset.riseMin, preset.riseMax));

        for (std::size_t i = 0; i < count; ++i)
        {
            const float baseSize = sizeDist(rng);

            Framework::ParticleSystem::SpriteParticleSpec spec;
            spec.objectName = "RunParticle";
            spec.textureKey = "particle_ui";
            spec.texturePath = "Textures/UI/Particle.png";
            spec.position = {
                worldPos.x + (dir * preset.offsetX) + jitterDist(rng),
                worldPos.y + preset.offsetY + jitterDist(rng)
            };
            spec.velocity = {
                -dir * speedDist(rng) + jitterDist(rng),
                riseDist(rng)
            };
            spec.life = lifeDist(rng);
            spec.startSize = baseSize;
            spec.endSize = baseSize * preset.endSizeScale;
            spec.r = preset.red;
            spec.g = preset.green;
            spec.b = preset.blue;
            spec.startAlpha = preset.startAlpha;
            spec.endAlpha = preset.endAlpha;

            particleSystem.SpawnSpriteParticle(spec);
        }
    }

} // namespace mygame
