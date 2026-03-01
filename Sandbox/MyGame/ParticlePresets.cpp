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

    void SpawnEnemyDeathParticles(
        Framework::ParticleSystem& particleSystem,
        const glm::vec2& worldPos,
        std::size_t count)
    {
        if (count == 0)
            return;

        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> angleDist(0.0f, 6.283185f);
        std::uniform_real_distribution<float> speedDist(0.15f, 0.45f);
        std::uniform_real_distribution<float> lifeDist(0.35f, 0.6f);
        std::uniform_real_distribution<float> radiusDist(0.02f, 0.05f);
        std::uniform_real_distribution<float> hueJitter(-0.05f, 0.05f);

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
                std::sin(angle) * speed + 0.05f
            };
            spec.life = lifeDist(rng);
            spec.startRadius = baseRadius;
            spec.endRadius = baseRadius * 0.2f;
            spec.r = 1.0f;
            spec.g = 0.45f + hueJitter(rng);
            spec.b = 0.1f;
            spec.startAlpha = 0.95f;
            spec.endAlpha = 0.0f;

            particleSystem.SpawnCircleParticle(spec);
        }
    }

    void SpawnRunParticles(
        Framework::ParticleSystem& particleSystem,
        const glm::vec2& worldPos,
        float facingDir,
        std::size_t count)
    {
        if (count == 0)
            return;

        static std::mt19937 rng(std::random_device{}());
        const float dir = (facingDir >= 0.0f) ? 1.0f : -1.0f;
        std::uniform_real_distribution<float> speedDist(0.05f, 0.18f);
        std::uniform_real_distribution<float> lifeDist(0.2f, 0.35f);
        std::uniform_real_distribution<float> sizeDist(0.04f, 0.07f);
        std::uniform_real_distribution<float> jitterDist(-0.015f, 0.015f);
        std::uniform_real_distribution<float> riseDist(0.01f, 0.06f);

        for (std::size_t i = 0; i < count; ++i)
        {
            const float baseSize = sizeDist(rng);

            Framework::ParticleSystem::SpriteParticleSpec spec;
            spec.objectName = "RunParticle";
            spec.textureKey = "particle_ui";
            spec.texturePath = "Textures/UI/Particle.png";
            spec.position = {
                worldPos.x + (-dir * 0.08f) + jitterDist(rng),
                worldPos.y - 0.03f + jitterDist(rng)
            };
            spec.velocity = {
                -dir * speedDist(rng) + jitterDist(rng),
                riseDist(rng)
            };
            spec.life = lifeDist(rng);
            spec.startSize = baseSize;
            spec.endSize = baseSize * 1.5f;
            spec.r = 1.0f;
            spec.g = 1.0f;
            spec.b = 1.0f;
            spec.startAlpha = 0.7f;
            spec.endAlpha = 0.0f;

            particleSystem.SpawnSpriteParticle(spec);
        }
    }

} // namespace mygame
