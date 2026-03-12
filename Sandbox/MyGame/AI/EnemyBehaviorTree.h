/*********************************************************************************************
 \file      EnemyBehaviorTree.h
 \par       SofaSpuds
 \author    Choo Jian Wei (jianwei.c@digipen.edu) - Primary Author, 100%
 \brief     Declares the sandbox enemy behaviour-tree construction helpers.
 \details   Exposes the game-layer functions that assemble and configure enemy
            behaviour trees using the engine AI framework.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include "AI/BehaviorContext.h"
#include "AI/DecisionNode.h"
#include "AI/DecisionTree.h"
#include "Composition/Composition.h"
#include "Factory/Factory.h"
#include <memory>

namespace mygame
{
	
	std::unique_ptr<Framework::DecisionTree> BuildMeleeEnemyTree(Framework::GOC* enemy);
	std::unique_ptr<Framework::DecisionTree> BuildRangedEnemyTree(Framework::GOC* enemy);
	std::unique_ptr <Framework::DecisionTree>BuildNancieTree(Framework::GOC* enemy);
}
