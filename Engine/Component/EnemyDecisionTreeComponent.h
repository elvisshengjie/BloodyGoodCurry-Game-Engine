/*********************************************************************************************
 \file      EnemyDecisionTreeComponent.h
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Declares the EnemyDecisionTreeComponent class, which attaches an AI decision
            tree to an enemy game object.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include "Composition/Component.h"
#include "Memory/ComponentPool.h"
#include "Serialization/Serialization.h"
#include "Composition/Composition.h"
#include "AI/DecisionTree.h"
#include "AI/Blackboard.h"       // NEW
#include <iostream>
#include <memory>

#define NOMINMAX
#if defined(APIENTRY)
#  undef APIENTRY
#endif
#include <Windows.h>
#ifdef SendMessage
#  undef SendMessage
#endif

namespace Framework
{
    enum class Facing { LEFT, RIGHT };

    class EnemyDecisionTreeComponent : public GameComponent
    {
    public:
        std::unique_ptr<DecisionTree> tree;          ///< The decision tree controlling enemy behavior.
        std::unique_ptr<BlackBoard>   blackboard;    ///< Shared state store for this enemy's tree.

        float dir = 0.0f;
        float pauseTimer = 0.0f;
        float chaseSpeed = 0.0f;
        float chaseTimer = 0.0f;
        float maxChaseDuration = 3.0f;
        float retreatTimer = 0.0f;

        bool  rangedAttackActive = false;
        bool  rangedProjectileFired = false;
        float rangedAttackTimer = 0.0f;
        float rangedAttackDuration = 0.0f;

        bool  hasSeenPlayer = false;

        float prevX = 0.0f;
        float prevY = 0.0f;
        float stuckXTimer = 0.0f;
        float stuckYTimer = 0.0f;
        const float stuckThreshold = 0.2f;

        float patrolOriginX = 0.0f;
        float patrolOriginY = 0.0f;
        bool  patrolOriginSet = false;

        Facing facing = Facing::RIGHT;

        std::vector<int> currentPathNodeIDs;
        size_t           currentPathIndex = 0;

        EnemyDecisionTreeComponent() = default;

        void initialize() override
        {
            patrolOriginSet = false;
            patrolOriginX = 0.0f;
            patrolOriginY = 0.0f;
            chaseTimer = 0.0f;
            retreatTimer = 0.0f;
            pauseTimer = 0.0f;
            hasSeenPlayer = false;
            dir = 1.0f;
            prevX = prevY = 0.0f;
            stuckXTimer = stuckYTimer = 0.0f;

            // Reset tree and blackboard so they are rebuilt fresh on next update
            tree.reset();
            blackboard.reset();

            std::cout << "[EnemyDecisionTreeComponent] Initialized.\n";
        }

        void SendMessage(Message& m) override { (void)m; }
        void Serialize(ISerializer& s)  override { (void)s; }

        ComponentHandle Clone() const override
        {
            auto copy = ComponentPool<EnemyDecisionTreeComponent>::CreateTyped();
            copy->dir = dir;
            copy->pauseTimer = pauseTimer;
            copy->chaseSpeed = chaseSpeed;
            copy->chaseTimer = chaseTimer;
            copy->maxChaseDuration = maxChaseDuration;
            copy->retreatTimer = retreatTimer;
            copy->rangedAttackActive = rangedAttackActive;
            copy->rangedProjectileFired = rangedProjectileFired;
            copy->rangedAttackTimer = rangedAttackTimer;
            copy->rangedAttackDuration = rangedAttackDuration;
            copy->hasSeenPlayer = hasSeenPlayer;
            copy->currentPathNodeIDs = currentPathNodeIDs;
            copy->currentPathIndex = currentPathIndex;
            // tree and blackboard are NOT cloned — rebuilt lazily on first update
            return copy;
        }

        void ClearPath()
        {
            currentPathNodeIDs.clear();
            currentPathIndex = 0;
        }

        bool HasPath() const
        {
            return currentPathIndex < currentPathNodeIDs.size();
        }

        int GetCurrentNodeID() const
        {
            if (!HasPath()) return -1;
            return currentPathNodeIDs[currentPathIndex];
        }

        void AdvancePath()
        {
            if (HasPath()) ++currentPathIndex;
        }
    };
}