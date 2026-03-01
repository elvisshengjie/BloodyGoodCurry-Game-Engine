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
#include "Composition/Composition.h"              // defines Framework::GOC
#include "AI/BehaviorContext.h"
#include "Component/EnemyDecisionTreeComponent.h"
#include "Component/TransformComponent.h"
#include "Component/PlayerComponent.h"
#include "Factory/Factory.h"
#include <cmath>

namespace Framework
{
    using GOC = GameObjectComposition;
    inline bool HasTargetInRange(BehaviorContext& ctx)
    {
        if (!ctx.owner) return false;

        EnemyDecisionTreeComponent* ai = nullptr;
        TransformComponent* enemyTr = nullptr;
        TransformComponent* playerTr = nullptr;

        for (auto& [id, gocPtr] : FACTORY->Objects())
        {
            if (!gocPtr) continue;
            Framework::GameObjectComposition* goc = gocPtr.get(); // Full type

            // Is this our enemy?
            if (goc == ctx.owner)
            {
                ai = goc->GetComponentType<EnemyDecisionTreeComponent>(
                    ComponentTypeId::CT_EnemyDecisionTreeComponent);
                enemyTr = goc->GetComponentType<TransformComponent>(
                    ComponentTypeId::CT_TransformComponent);
            }

            // Is this the player?
            if (goc->GetComponent(ComponentTypeId::CT_PlayerComponent) != nullptr)
            {
                playerTr = goc->GetComponentType<TransformComponent>(
                    ComponentTypeId::CT_TransformComponent);
            }

            if (ai && enemyTr && playerTr) break;
        }

        if (!ai || !enemyTr || !playerTr) return false;

        float dx = enemyTr->x - playerTr->x;
        float dy = enemyTr->y - playerTr->y;
        float disSq = dx * dx + dy * dy;

        constexpr float detectionRadius = 0.2f;
        if (disSq <= detectionRadius * detectionRadius)
        {
            ai->hasSeenPlayer = true;
            ai->chaseTimer = 0.0f;
            if (ctx.blackboard)
                ctx.blackboard->Set<bool>("hasSeenPlayer", true);
        }

        return ai->hasSeenPlayer;
    }
}
