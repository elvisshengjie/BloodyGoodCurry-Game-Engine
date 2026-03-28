/*********************************************************************************************
 \file      AiCombatBindings.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) -  Author, 100%
 \brief     Binds sandbox enemy combat spawning hooks into the engine AI system.
 \details   Connects game-specific enemy attack behaviour to the engine AI callback
            interface so hitboxes and projectiles are spawned from the game layer.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "EngineCall.hpp"

#include "Common/GameComponentIDs.h"
#include "Components/EnemyComponent.h"
#include "Systems/AiSystem.h"
#include "Systems/HitBoxSystem.h"
#include "Systems/LogicSystem.h"

#include <cmath>

static constexpr float kEnemyProjectileSpeedScale = 1.0f;
static constexpr float kEnemyProjectileHitboxScale = 1.4f;
static constexpr float kEnemyMeleeHitboxScale = 1.4f;

namespace mygame {

    /*************************************************************************************
      \brief Binds game-owned melee/projectile spawning callbacks into the engine AI system.
      \param ai     Active engine AiSystem receiving the binding callbacks.
      \param logic  Active LogicSystem that owns the game-side HitBoxSystem.

      \details
              Filters AI-controlled objects to enemies, then installs lambdas that route
              melee hitbox and projectile spawning through the current game-side combat
              runtime instead of hardcoded engine logic.
    *************************************************************************************/
    void BindAiCombat(Framework::AiSystem& ai, Framework::LogicSystem& logic)
    {
        auto* const logicPtr = &logic;
        ai.SetObjectFilterCallback([](const Framework::GOC& object)
        {
            return object.GetComponent(mygame::CT_EnemyComponent()) != nullptr;
        });
        ai.SetBehaviorBindingCallback(
            [logicPtr](Framework::BehaviorTreeComponent& behavior, Framework::GOC* /*owner*/)
            {
                behavior.spawnHitBoxFn =
                    [logicPtr](Framework::GOC* owner, float x, float y, float w, float h,
                        float dmg, float dur, float /*knockback*/, float rotation, bool consumeOnHit)
                {
                    if (logicPtr && logicPtr->hitBoxSystem)
                    {
                        const float hitboxScale = (std::fabs(rotation) > 0.0001f) ? 1.0f : kEnemyMeleeHitboxScale;
                        return logicPtr->hitBoxSystem->SpawnHitBox(
                            owner, x, y, w * hitboxScale, h * hitboxScale, dmg, dur,
                            Framework::HitBoxComponent::Team::Enemy, rotation, consumeOnHit, 0.0f);
                    }
                    return static_cast<Framework::HitBoxComponent*>(nullptr);
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
