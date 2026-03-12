/*********************************************************************************************
 \file      ParticlePresets.hpp
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Primary Author, 100%
 \brief     Declares sandbox-specific particle effect preset helpers.
 \details   Provides game-side convenience functions for spawning predefined particle
            effects using the engine's generic ParticleSystem.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

#include <glm/vec2.hpp>
#include <cstddef>

namespace Framework {
    class ParticleSystem;
}

namespace mygame {

    struct EnemyDeathParticlePreset
    {
        std::size_t count{ 12 };
        float speedMin{ 0.15f };
        float speedMax{ 0.45f };
        float lifeMin{ 0.35f };
        float lifeMax{ 0.6f };
        float radiusMin{ 0.02f };
        float radiusMax{ 0.05f };
        float upwardVelocityBias{ 0.05f };
        float red{ 1.0f };
        float green{ 0.45f };
        float blue{ 0.1f };
        float greenJitterMin{ -0.05f };
        float greenJitterMax{ 0.05f };
        float startAlpha{ 0.95f };
        float endAlpha{ 0.0f };
        float endRadiusScale{ 0.2f };
    };

    struct RunParticlePreset
    {
        std::size_t count{ 3 };
        float speedMin{ 0.05f };
        float speedMax{ 0.18f };
        float lifeMin{ 0.2f };
        float lifeMax{ 0.35f };
        float sizeMin{ 0.04f };
        float sizeMax{ 0.07f };
        float jitterMin{ -0.015f };
        float jitterMax{ 0.015f };
        float riseMin{ 0.01f };
        float riseMax{ 0.06f };
        float offsetX{ -0.08f };
        float offsetY{ -0.03f };
        float endSizeScale{ 1.5f };
        float red{ 1.0f };
        float green{ 1.0f };
        float blue{ 1.0f };
        float startAlpha{ 0.7f };
        float endAlpha{ 0.0f };
    };

    EnemyDeathParticlePreset& GetEnemyDeathParticlePreset();
    RunParticlePreset& GetRunParticlePreset();
    void ResetEnemyDeathParticlePreset();
    void ResetRunParticlePreset();

    void SpawnEnemyDeathParticles(
        Framework::ParticleSystem& particleSystem,
        const glm::vec2& worldPos,
        std::size_t count = 0);

    void SpawnRunParticles(
        Framework::ParticleSystem& particleSystem,
        const glm::vec2& worldPos,
        float facingDir,
        std::size_t count = 0);

} // namespace mygame
