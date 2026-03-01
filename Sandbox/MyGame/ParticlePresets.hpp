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
