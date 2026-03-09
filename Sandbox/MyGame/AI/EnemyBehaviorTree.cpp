/*********************************************************************************************
 \file      EnemyBehaviorTree.cpp
 \par       SofaSpuds
 \author     jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

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
#include "Components/EnemyDecisionTreeComponent.h"
#include "Components/EnemyHealthComponent.h"
#include "Physics/Dynamics/RigidBodyComponent.h"
#include "Components/EnemyTypeComponent.h"
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
    /*****************************************************************************************
      \brief Wraps an AI action with a health guard so dead enemies skip execution.
      \tparam T    Callable type matching void(Framework::BehaviorContext&).
      \param action Action to wrap.
      \return Lambda that checks EnemyHealthComponent before forwarding to the action.
      \details Used by all leaf nodes in both melee and ranged trees to avoid running
               movement or attack logic on enemies that have already died.
    *****************************************************************************************/
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
        /*****************************************************************************************
          \brief Constructs the decision tree for a melee-type enemy.
          \param enemy Owner game object composition the tree will be bound to.
          \return Owning pointer to the built DecisionTree, or nullptr if enemy is null.
          \details
          Tree structure:
          - Root: HasTargetInRange?
            - YES : MeleeAttack (AliveGuarded)
            - NO  : Patrol      (AliveGuarded)
        *****************************************************************************************/
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

        /*****************************************************************************************
          \brief Constructs the decision tree for a ranged-type enemy.
          \param enemy Owner game object composition the tree will be bound to.
          \return Owning pointer to the built DecisionTree, or nullptr if enemy is null.
          \details
          Tree structure:
          - Root: HasTargetInRange?
            - YES : RangedAttack (AliveGuarded)
            - NO  : Patrol       (AliveGuarded)
        *****************************************************************************************/
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
    /*****************************************************************************************
      \struct EnemyTreeRegistrar
      \brief Static registrar that inserts enemy tree factory functions at program startup.
      \details
      Registers "enemy_melee" and "enemy_ranged" keys into BehaviorTreeComponent::Registry()
      so the engine can construct the correct tree from a string key stored in level data,
      without this file needing to be called explicitly.
    *****************************************************************************************/
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
