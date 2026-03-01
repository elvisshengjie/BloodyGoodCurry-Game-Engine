#pragma once

#include <functional>

namespace Framework
{
    class GameObjectComposition;

    enum class CombatAudioEvent
    {
        PlayerHurt,
        PlayerDeath,
        EnemyHurt,
        EnemyDeath,
        PlayerAttackHit,
        PlayerAttackBlocked,
        PlayerAttackMiss
    };

    using CombatAudioCallback = std::function<void(GameObjectComposition*, CombatAudioEvent)>;
}
