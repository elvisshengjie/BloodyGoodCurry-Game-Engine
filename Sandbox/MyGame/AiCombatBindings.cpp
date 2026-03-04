/*********************************************************************************************
 \file      AiCombatBindings.cpp
 \par       SofaSpuds
 \author
 \brief     Binds sandbox enemy combat spawning hooks into the engine AI system.
 \details   Connects game-specific enemy attack behaviour to the engine AI callback
            interface so hitboxes and projectiles are spawned from the game layer.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "EngineCall.hpp"

#include "Systems/AiSystem.h"
#include "Systems/HitBoxSystem.h"
#include "Systems/LogicSystem.h"

static constexpr float kEnemyProjectileSpeedScale = 1.5f;
static constexpr float kEnemyProjectileHitboxScale = 1.4f;
static constexpr float kEnemyMeleeHitboxScale = 1.4f;

namespace mygame {

    void BindAiCombat(Framework::AiSystem& ai, Framework::LogicSystem& logic)
    {
        auto* const logicPtr = &logic;
        ai.SetBehaviorBindingCallback(
            [logicPtr](Framework::BehaviorTreeComponent& behavior, Framework::GOC* /*owner*/)
            {
                behavior.spawnHitBoxFn =
                    [logicPtr](Framework::GOC* owner, float x, float y, float w, float h,
                        float dmg, float dur, float /*knockback*/)
                {
                    if (logicPtr && logicPtr->hitBoxSystem)
                    {
                        logicPtr->hitBoxSystem->SpawnHitBox(
                            owner, x, y, w* kEnemyMeleeHitboxScale, h*kEnemyMeleeHitboxScale, dmg, dur,
                            Framework::HitBoxComponent::Team::Enemy, 0.0f);
                    }
                };

                behavior.spawnProjectileFn =
                    [logicPtr](Framework::GOC* owner, float x, float y, float dirX, float dirY,
                        float speed, float w, float h, float dmg, float lifetime)
                {
                    if (logicPtr && logicPtr->hitBoxSystem)
                    {
                        logicPtr->hitBoxSystem->SpawnProjectile(
                            owner, x, y, dirX, dirY, speed*kEnemyProjectileSpeedScale, w*kEnemyProjectileHitboxScale, h* kEnemyProjectileHitboxScale, dmg, lifetime,
                            Framework::HitBoxComponent::Team::Enemy);
                    }
                };
            });
    }

} // namespace mygame
