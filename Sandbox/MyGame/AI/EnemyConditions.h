/*********************************************************************************************
 \file      EnemyConditions.h
 \par       SofaSpuds
 \author
 \brief     Declares game-specific condition helpers for enemy AI decision logic.
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

    static constexpr float kMeleeDetectionRadius = 0.3f;        // detect player very close
    static constexpr float kMeleeChaseRetentionRadius = 1.2f;   // stops chasing sooner
    static constexpr float kRangedDetectionRadius = 0.5f;       // detect player only nearby
    static constexpr float kRangedChaseRetentionRadius = 2.0f;  // stop chasing sooner

    // Aliases so EnemyActions.h references compile without changes.
    static constexpr float kDetectionRadius = kRangedDetectionRadius;
    static constexpr float kChaseRetentionRadius = kRangedChaseRetentionRadius;

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
