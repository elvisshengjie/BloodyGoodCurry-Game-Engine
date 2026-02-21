#pragma once
#include "AI/BehaviorContext.h"
#include "Component/EnemyDecisionTreeComponent.h"
#include "Component/TransformComponent.h"
#include "Component/PlayerComponent.h"
#include "Factory/Factory.h"          // declares extern GameObjectFactory* FACTORY
#include <cmath>

namespace EnemyConditions
{
    inline bool HasTargetInRange(BehaviorContext& ctx)
    {
        if (!ctx.owner) return false;
        EnemyDecisionTreeComponent* ai = nullptr;
        TransformComponent* enemyTr = nullptr;
        TransformComponent* playerTr = nullptr;
        for (auto& [id, gocPtr] : FACTORY->Objects())
        {
            if (!gocPtr) continue;
            GOC* goc = gocPtr.get();

            if (goc == ctx.owner)
            {
               auto ai = goc->GetComponentType<EnemyDecisionTreeComponent>(
                    ComponentTypeId::CT_EnemyDecisionTreeComponent);
                auto enemyTr = goc->GetComponentType<TransformComponent>(
                    ComponentTypeId::CT_TransformComponent);
            }

            if (goc->GetComponent(ComponentTypeId::CT_PlayerComponent))
            {
                auto playerTr = goc->GetComponentType<TransformComponent>(
                    ComponentTypeId::CT_TransformComponent);
            }

            if (ai && enemyTr && playerTr) break;
        }

        if (!ai || !enemyTr || !playerTr) return false;

        float dx = enemyTr->x - playerTr->x;
        float dy = enemyTr->y - playerTr->y;
        float distSq = dx * dx + dy * dy;

        constexpr float detectionRadius = 0.2f;
        if (distSq <= detectionRadius * detectionRadius)
        {
            ai->hasSeenPlayer = true;
            ai->chaseTimer = 0.0f;
            ctx.blackboard->Set<bool>("hasSeenPlayer", true);
        }

        return ai->hasSeenPlayer;
    }
}