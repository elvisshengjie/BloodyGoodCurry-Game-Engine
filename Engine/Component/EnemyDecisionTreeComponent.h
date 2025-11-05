/*********************************************************************************************
\file      EnemyDecisionTreeComponent.h
\par       SofaSpuds
\author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

\brief     Declaration and implementation of the EnemyDecisionTreeComponent class. This component
           provides AI behavior for enemy entities using a DecisionTree structure.

\details
           The EnemyDecisionTreeComponent manages an enemy's AI logic through a DecisionTree,
           allowing modular and dynamic behavior evaluation each frame. Key responsibilities
           include:
           - Initializing a default decision tree for the enemy on component initialization.
           - Managing timers and state variables such as direction, chase speed, and player
             detection flags.
           - Supporting cloning of the component for multiple enemy instances.

\note      The DecisionTree is managed via a std::unique_ptr to ensure unique ownership
           and automatic cleanup. This component does not handle collision or attack logic;
           it strictly governs AI decision-making.

\copyright
           All content © 2025 DigiPen Institute of Technology Singapore.
           All rights reserved.
*********************************************************************************************/
#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include "AI/DecisionTreeDefault.h"
#include "Composition/Composition.h"
#include "AI/DecisionTree.h"
#include<iostream>
#define NOMINMAX
#if defined(APIENTRY)
#  undef APIENTRY
#endif
#include <Windows.h>
#ifdef SendMessage
#undef SendMessage
#endif
namespace Framework {
    /*****************************************************************************************
    \brief
    Creates a default decision tree for a given enemy entity.

    \param enemy
    Pointer to the enemy GameObjectComponent (GOC).

    \return
    A std::unique_ptr to a new DecisionTree instance initialized for the enemy.
    *****************************************************************************************/
	std::unique_ptr<DecisionTree> CreateDefaultEnemyTree(GOC* enemy);
	/*****************************************************************************************
    \class EnemyDecisionTreeComponent
    \brief
    AI component that manages enemy behavior using a decision tree.

    \details
    Provides modular, runtime evaluation of enemy actions through a DecisionTree. Stores
    state variables such as direction, chase timers, and player detection flags to support
    dynamic behavior. Can be cloned for multiple enemy instances.
    *****************************************************************************************/
	class EnemyDecisionTreeComponent : public GameComponent
	{
	public:
		std::unique_ptr<DecisionTree> tree;
        float dir = 1.0f;
        float pauseTimer = 0.0f;
		float chaseSpeed = 0.0f;
		float chaseTimer = 0.0f;
		float maxChaseDuration = 3.0f;
		bool hasSeenPlayer = false;
		 /*****************************************************************************************
        \brief
        Initializes the component and its decision tree.

        \details
        Called once when the component is added to a GameObject. Creates a default
        decision tree for the owning enemy entity and outputs initialization status to
        the console.
        *****************************************************************************************/
		void initialize() override
		{
			GOC* ownerGOC = GetOwner();
			if (ownerGOC) {
				tree = Framework::CreateDefaultEnemyTree(ownerGOC);
				std::cout << "[EnemyDecisionTreeComponent] Tree initialized.\n";
			}
		}
		/*****************************************************************************************
        \brief
        Handles incoming messages.

        \param m
        Message to process.

        \details
        Currently empty; required by GameComponent interface.
        *****************************************************************************************/
		void SendMessage(Message& m) override { (void)m; }
		/*****************************************************************************************
        \brief
        Serializes or deserializes component data.

        \param s
        Serializer instance.

        \details
        Currently empty; required by GameComponent interface.
        *****************************************************************************************/
		void Serialize(ISerializer& s) override { (void)s; }
		/*****************************************************************************************
        \brief
        Creates a clone of this component.

        \return
        A std::unique_ptr to a new EnemyDecisionTreeComponent instance.

        \details
        Allows the component to be duplicated for use on multiple enemy entities.
        *****************************************************************************************/
		std::unique_ptr<GameComponent> Clone() const override
		{
			auto copy = std::make_unique<EnemyDecisionTreeComponent>();
			return copy;
		}
	};
}
