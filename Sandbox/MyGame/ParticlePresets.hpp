/*********************************************************************************************
 \file      ParticlePresets.hpp
 \par       SofaSpuds
 \author
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

    void SpawnEnemyDeathParticles(
        Framework::ParticleSystem& particleSystem,
        const glm::vec2& worldPos,
        std::size_t count = 12);

    void SpawnRunParticles(
        Framework::ParticleSystem& particleSystem,
        const glm::vec2& worldPos,
        float facingDir,
        std::size_t count = 3);

} // namespace mygame
