/*********************************************************************************************
 \file      EnemyDecisionTreeComponent.h
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Declares the EnemyDecisionTreeComponent class, which attaches an AI decision
            tree to an enemy game object. This component governs high-level enemy
            behavior such as idle, chase, or patrol states using decision-tree logic.

 \details
            EnemyDecisionTreeComponent integrates the AI/DecisionTree system into the
            engine's ECS architecture. When initialized, it automatically constructs a
            default decision tree for the owning GameObjectComposition via
            CreateDefaultEnemyTree(). The component stores additional runtime data such
            as movement direction, chase timers, and flags indicating whether the player
            has been seen.

            Responsibilities:
            - Owns and updates an AI DecisionTree instance.
            - Tracks state data like chase direction, pause timers, and player detection.
            - Provides a framework for extensible enemy AI logic.

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
#include "AI/Blackboard.h"
#include <iostream>

#if defined(_WIN32)
#define NOMINMAX
#if defined(APIENTRY)
#  undef APIENTRY
#endif
#include <Windows.h>
#ifdef SendMessage
#  undef SendMessage
#endif
#endif

namespace Framework {
    enum class Facing { LEFT, RIGHT };
    /*****************************************************************************************
      \brief Creates a default decision tree for an enemy.
      \param enemy  Pointer to the enemy GameObjectComposition.
      \return A unique_ptr to a new DecisionTree instance configured for default AI behavior.
    *****************************************************************************************/
    //std::unique_ptr<DecisionTree> CreateDefaultEnemyTree(GOC* enemy, LogicSystem* logic);

    /*****************************************************************************************
      \class EnemyDecisionTreeComponent
      \brief Component responsible for managing the AI decision tree of an enemy.

      This component attaches a DecisionTree to an enemy entity, initializing it with
      default behaviors defined in AI/DecisionTreeDefault.h. It tracks state variables
      such as movement direction, chase duration, and player detection flags.
    *****************************************************************************************/
    class EnemyDecisionTreeComponent : public GameComponent
    {
    public:
        float dir = 1.0f;                    ///< Movement direction (1.0 for right, -1.0 for left).
        float pauseTimer = 0.0f;             ///< Timer used for brief pauses between AI actions.
        float chaseSpeed = 0.0f;             ///< Current speed while chasing the player.
        float chaseTimer = 0.0f;             ///< Accumulated time spent in chase mode.
        float maxChaseDuration = 3.0f;       ///< Maximum allowed chase time before reset.
        float retreatTimer = 0.0f;           ///< Retreat Timer
        
        bool  pendingProjectile = false;
        float pendingProjectileDirX = 0.0f;
        float pendingProjectileDirY = 0.0f;
        float pendingProjectileSpawnX = 0.0f;
        float pendingProjectileSpawnY = 0.0f;

        bool rangedAttackActive = false;     ///< True while a ranged attack animation is playing.
        bool rangedProjectileFired = false;  ///< True once the ranged projectile has been spawned.
        float rangedAttackTimer = 0.0f;      ///< Timer tracking ranged attack animation elapsed time.
        float rangedAttackDuration = 0.0f;   ///< Cached duration for the ranged attack animation.
        bool hasSeenPlayer = false;          ///< Tracks whether the enemy has detected the player.
        float prevX = 0.0f;
        float prevY = 0.0f;
        float stuckXTimer = 0.0f;  // how long X has been stuck
        float stuckYTimer = 0.0f;  // how long Y has been stuck
        const float stuckThreshold = 0.2f; // seconds before trying alternative axis
        float patrolOriginX = 0.0f; //patrol origin x
        float patrolOriginY = 0.0f;// patrol origin y
        bool patrolOriginSet = false; // set patrol origin
        Facing facing = Facing::RIGHT;

        // ---------------- Navigation State ----------------
        std::vector<int> currentPathNodeIDs;  ///< Current computed path (node IDs)
        size_t currentPathIndex = 0;          ///< Index into current path

        EnemyDecisionTreeComponent() = default;

        /*************************************************************************************
          \brief Initializes the decision tree for this enemy component.
          \details
              - Retrieves the owning GameObjectComposition.
              - Constructs a default DecisionTree using CreateDefaultEnemyTree().
              - Logs a debug message upon successful initialization.
        *************************************************************************************/
        void initialize() override
        {
            patrolOriginSet = false;
            patrolOriginX = 0.0f;
            patrolOriginY = 0.0f;
            pendingProjectile = false;
            pendingProjectileDirX = pendingProjectileDirY = 0.0f;
            pendingProjectileSpawnX = pendingProjectileSpawnY = 0.0f;
            chaseTimer = 0.0f;
            retreatTimer = 0.0f;
            pauseTimer = 0.0f;
            hasSeenPlayer = false;
            dir = 1.0f;
            prevX = prevY = 0.0f;
            stuckXTimer = stuckYTimer = 0.0f;

            std::cout << "[EnemyDecisionTreeComponent] State initialized.\n";   
        }

        /*************************************************************************************
          \brief Handles incoming messages sent to this component.
          \param m  Reference to the incoming message.
          \note  Currently unused but kept for future AI message handling.
        *************************************************************************************/
        void SendMessage(Message& m) override { (void)m; }

        /*************************************************************************************
          \brief Serializes the component data.
          \param s  Reference to the serializer.
          \note  Currently a placeholder; no serializable data yet.
        *************************************************************************************/
        void Serialize(ISerializer& s) override { (void)s; }

        /*************************************************************************************
          \brief Creates a deep copy of this component.
          \return A unique_ptr holding a cloned EnemyDecisionTreeComponent.
        *************************************************************************************/
        ComponentHandle Clone() const override
        {
            auto copy = ComponentPool<EnemyDecisionTreeComponent>::CreateTyped();
            copy->dir = dir;
            copy->pauseTimer = pauseTimer;
            copy->chaseSpeed = chaseSpeed;
            copy->chaseTimer = chaseTimer;
            copy->maxChaseDuration = maxChaseDuration;
            copy->rangedAttackActive = rangedAttackActive;
            copy->rangedProjectileFired = rangedProjectileFired;
            copy->pendingProjectile = pendingProjectile;
            copy->pendingProjectileDirX = pendingProjectileDirX;
            copy->pendingProjectileDirY = pendingProjectileDirY;
            copy->pendingProjectileSpawnX = pendingProjectileSpawnX;
            copy->pendingProjectileSpawnY = pendingProjectileSpawnY;
            copy->rangedAttackTimer = rangedAttackTimer;
            copy->rangedAttackDuration = rangedAttackDuration;
            copy->hasSeenPlayer = hasSeenPlayer;
            copy->currentPathNodeIDs = currentPathNodeIDs;
            copy->currentPathIndex = currentPathIndex;

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
            if (!HasPath())
                return -1;
          return currentPathNodeIDs[currentPathIndex];
        }

        void AdvancePath()
        {
            if (HasPath())
                ++currentPathIndex;
        }

    };
}
