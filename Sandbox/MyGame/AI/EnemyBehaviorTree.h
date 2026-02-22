#pragma once
#include "AI/BehaviorContext.h"
#include "AI/DecisionNode.h"
#include "AI/DecisionTree.h"
#include "Composition/Composition.h"
#include "Factory/Factory.h"
#include <memory>

namespace Framework
{
	
	std::unique_ptr<DecisionTree> BuildEnemyTree(GOC* enemy);
}
