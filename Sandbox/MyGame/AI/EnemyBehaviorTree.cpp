/*********************************************************************************************
 \file      EnemyBehaviorTree.cpp
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu)

 \brief     Implements tree construction and per-frame update for default enemy AI.

 \details
            This is the only file that knows about both the engine AI layer and the
            gameplay layer. LogicSystem is bound as a callback here and never travels
            inside the tree.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "EnemyBehaviorTree.h"
#include "EnemyConditions.h"
#include "EnemyActions.h"
#include "AI/DecisionNode.h"
#include "Component/EnemyDecisionTreeComponent.h"
#include "Component/EnemyHealthComponent.h"
#include "Physics/Dynamics/RigidBodyComponent.h"
#include "Component/EnemyTypeComponent.h"
#include "Component/HitBoxComponent.h"
#include "Composition/Composition.h"  
#include "Systems/LogicSystem.h"
#include "Factory/Factory.h"
#include "Common/CRTDebug.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif

namespace Framework
{
    std::unique_ptr<DecisionTree> BuildEnemyTree(GOC* enemy)
    {
        if (!enemy) return nullptr;

        // Determine enemy type at build time
        bool isRanged = false;
        auto* typeComp = enemy->GetComponentType<EnemyTypeComponent>(
            ComponentTypeId::CT_EnemyTypeComponent);
        isRanged = typeComp && typeComp->Etype == EnemyTypeComponent::EnemyType::ranged;

        // Patrol leaf
        auto patrolLeaf = std::make_unique<DecisionNode>(
            nullptr, nullptr, nullptr,
            [](BehaviorContext& ctx) { Framework::Patrol(ctx); }
        );

        // Attack leaf
        auto attackLeaf = std::make_unique<DecisionNode>(
            nullptr, nullptr, nullptr,
            [isRanged](BehaviorContext& ctx)
            {
                if (isRanged) Framework::RangedAttack(ctx);
                else Framework::MeleeAttack(ctx);
            }
        );

        // Root: condition → attack : patrol
        auto root = std::make_unique<DecisionNode>(
            [](BehaviorContext& ctx)
            {
                return Framework::HasTargetInRange(ctx);
            },
            std::move(attackLeaf),
            std::move(patrolLeaf),
            nullptr
        );

        return std::make_unique<DecisionTree>(std::move(root));
    }
}