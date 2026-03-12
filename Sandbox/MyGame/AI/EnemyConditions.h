/*********************************************************************************************
 \file      EnemyConditions.h
 \par       SofaSpuds
 \author    Choo Jian Wei (jianwei.c@digipen.edu) - Primary Author, 100%
 \brief     Declares and implements game-specific condition helpers for enemy AI decision logic.
 \details   Contains condition predicates used by the sandbox enemy AI to evaluate
            player distance, line-of-sight, attack readiness, and similar gameplay checks.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include "Composition/Composition.h"
#include "AI/BehaviorContext.h"
#include "Components/EnemyComponent.h"
#include "Components/EnemyDecisionTreeComponent.h"
#include "Component/TransformComponent.h"
#include "Components/EnemyTypeComponent.h"
#include "Components/PlayerComponent.h"
#include "Factory/Factory.h"
#include <cmath>

namespace mygame
{
    using GOC = Framework::GameObjectComposition;
    /*****************************************************************************************
      \brief Detection and chase retention radii for each enemy type.
      \details
      Melee enemies have a tighter detection radius but shorter retention range,
      making them aggressive only up close and quick to disengage.
      Ranged enemies detect slightly further and retain the chase longer.
      kDetectionRadius and kChaseRetentionRadius are aliases for the ranged values,
      used by EnemyActions.h for backwards-compatible projectile fire distance checks.
    *****************************************************************************************/
    static constexpr float kMeleeDetectionRadius = 0.3f;        //< Aggro radius for melee enemies; triggers chase when player is within this distance.
    static constexpr float kMeleeChaseRetentionRadius = 1.2f;   ///< Melee enemies disengage if the player exceeds this distance.
    static constexpr float kRangedDetectionRadius = 0.5f;       ///< Aggro radius for ranged enemies; slightly wider than melee.
    static constexpr float kRangedChaseRetentionRadius = 2.0f;  ///< Ranged enemies retain chase longer before disengaging.

    // Aliases so EnemyActions.h references compile without changes.
    static constexpr float kDetectionRadius = kRangedDetectionRadius;
    static constexpr float kChaseRetentionRadius = kRangedChaseRetentionRadius;
    /*****************************************************************************************
      \brief Condition predicate: returns true if the enemy should be actively chasing the player.
      \param ctx BehaviorContext containing the owner enemy and blackboard.
      \return True if the enemy has the player in aggro range or is still in chase retention.
      \details
      - Iterates the factory to locate the owner's AI/transform components and the player transform.
      - Selects detection and retention radii based on EnemyTypeComponent (melee vs ranged).
      - Sets hasSeenPlayer and syncs the blackboard when the player enters detection radius.
      - Clears hasSeenPlayer when the player moves beyond the retention radius.
      - Once aggroed, the enemy remains in chase state until the player exceeds retentionR,
        providing hysteresis to prevent rapid toggling between patrol and attack.
    *****************************************************************************************/
    inline bool HasTargetInRange(Framework::BehaviorContext& ctx)
    {
        if (!ctx.owner) return false;

        Framework::EnemyDecisionTreeComponent* ai = nullptr;
        Framework::TransformComponent* enemyTr = nullptr;
        Framework::TransformComponent* playerTr = nullptr;
        Framework::EnemyTypeComponent* typeComp = nullptr;

        for (auto& [id, gocPtr] : Framework::FACTORY->Objects())
        {
            if (!gocPtr) continue;
            Framework::GameObjectComposition* goc = gocPtr.get();

            if (goc == ctx.owner)
            {
                ai = goc->GetComponentType<Framework::EnemyDecisionTreeComponent>(
                    Framework::ComponentTypeId::CT_EnemyDecisionTreeComponent);
                enemyTr = goc->GetComponentType<Framework::TransformComponent>(
                    Framework::ComponentTypeId::CT_TransformComponent);
                typeComp = goc->GetComponentType<Framework::EnemyTypeComponent>(
                    Framework::ComponentTypeId::CT_EnemyTypeComponent);
            }

            if (goc->GetComponent(Framework::ComponentTypeId::CT_PlayerComponent) != nullptr)
            {
                playerTr = goc->GetComponentType<Framework::TransformComponent>(
                    Framework::ComponentTypeId::CT_TransformComponent);
            }

            if (ai && enemyTr && playerTr) break;
        }

        if (!ai || !enemyTr || !playerTr) return false;

        // Pick radii based on enemy type.
        const bool isRanged = typeComp &&
            typeComp->Etype == Framework::EnemyTypeComponent::EnemyType::ranged;

        const float detectR = isRanged ? kRangedDetectionRadius : kMeleeDetectionRadius;
        const float retentionR = isRanged ? kRangedChaseRetentionRadius : kMeleeChaseRetentionRadius;

        float dx = enemyTr->x - playerTr->x;
        float dy = enemyTr->y - playerTr->y;
        float disSq = dx * dx + dy * dy;

        // Initial aggro.
        if (disSq <= detectR * detectR)
        {
            ai->hasSeenPlayer = true;
            ai->chaseTimer = 0.0f;
            if (ctx.blackboard)
                ctx.blackboard->Set<bool>("hasSeenPlayer", true);
        }

        // De-aggro only past retention radius.
        if (ai->hasSeenPlayer && disSq > retentionR * retentionR)
        {
            ai->hasSeenPlayer = false;
            ai->chaseTimer = 0.0f;
            if (ctx.blackboard)
                ctx.blackboard->Set<bool>("hasSeenPlayer", false);
        }

        return ai->hasSeenPlayer;
    }
}
