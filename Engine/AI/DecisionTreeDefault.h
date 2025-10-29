#pragma once
#include "Composition/Composition.h"
#include "DecisionTree.h"
#include "DecisionNode.h"
#include "Component/EnemyDecisionTreeComponent.h"
#include "Component/EnemyHealthComponent.h"
#include "Component/EnemyAttackComponent.h"
#include "Component/TransformComponent.h"
#include <cstdio>
namespace Framework {extern GOC* PLAYER; DecisionNode* CreateDefaultEnemyTree(GOC* enemy);}