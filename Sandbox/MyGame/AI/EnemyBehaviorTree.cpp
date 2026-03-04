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
#include "Component/BehaviorTreeComponent.h"
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

namespace mygame
{
        // --------------------------------------------------------
        // Shared Alive Guard Wrapper
        // --------------------------------------------------------
        template<typename T>
        auto AliveGuardedAction(T&& action)
        {
            return [action = std::forward<T>(action)](Framework::BehaviorContext& ctx)
                {
                    auto* healthComp = ctx.owner->GetComponentType<Framework::EnemyHealthComponent>(
                        Framework::ComponentTypeId::CT_EnemyHealthComponent);

                    if (!healthComp || healthComp->enemyHealth <= 0)
                        return;

                    action(ctx);
                };
        }
        // ========================================================
        // MELEE TREE
        // ========================================================
        std::unique_ptr<Framework::DecisionTree> BuildMeleeEnemyTree(GOC* enemy)
        {
            if (!enemy) return nullptr;

            auto patrolLeaf = std::make_unique<Framework::DecisionNode>(
                nullptr, nullptr, nullptr,
                AliveGuardedAction([](Framework::BehaviorContext& ctx)
                {
                        Patrol(ctx);
                })
            );

            auto attackLeaf = std::make_unique<Framework::DecisionNode>(
                nullptr, nullptr, nullptr,
                AliveGuardedAction([](Framework::BehaviorContext& ctx)
                {
                        MeleeAttack(ctx);
                })
            );

            auto root = std::make_unique<Framework::DecisionNode>(
                [](Framework::BehaviorContext& ctx)
                {
                    return HasTargetInRange(ctx);
                },
                std::move(attackLeaf),
                std::move(patrolLeaf),
                nullptr
            );

            return std::make_unique<Framework::DecisionTree>(std::move(root));
        }

        // ========================================================
        // RANGED TREE
        // ========================================================
        std::unique_ptr<Framework::DecisionTree> BuildRangedEnemyTree(GOC* enemy)
        {
            if (!enemy) return nullptr;

            auto patrolLeaf = std::make_unique<Framework::DecisionNode>(
                nullptr, nullptr, nullptr,
                AliveGuardedAction([](Framework::BehaviorContext& ctx)
                    {
                        Patrol(ctx);
                    })
            );

            auto attackLeaf = std::make_unique<Framework::DecisionNode>(
                nullptr, nullptr, nullptr,
                AliveGuardedAction([](Framework::BehaviorContext& ctx)
                    {
                        RangedAttack(ctx);
                    })
            );

            auto root = std::make_unique<Framework::DecisionNode>(
                [](Framework::BehaviorContext& ctx)
                {
                    return HasTargetInRange(ctx);
                },
                std::move(attackLeaf),
                std::move(patrolLeaf),
                nullptr
            );

            return std::make_unique<Framework::DecisionTree>(std::move(root));
        }
}

namespace 
{
    struct EnemyTreeRegistrar
    {
        EnemyTreeRegistrar()
        {
            Framework::BehaviorTreeComponent::Registry()["enemy_melee"] =
                [](Framework::GOC* owner)
                { return mygame::BuildMeleeEnemyTree(owner); };

            Framework::BehaviorTreeComponent::Registry()["enemy_ranged"] =
                [](Framework::GOC* owner)
                { return mygame::BuildRangedEnemyTree(owner); };
        }
    };
    const EnemyTreeRegistrar gRegistrar;
}