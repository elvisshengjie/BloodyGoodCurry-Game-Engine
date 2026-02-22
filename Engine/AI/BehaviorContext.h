#pragma once
#include <functional>
#include "Blackboard.h"
class GOC;

using SpawnHitBoxFn
= std::function < void(GOC*, float, float, float, float, float, float, float)>;

using SpawnProjectileFn = std::function
<void(GOC*, float, float, float, float, float, float, float, float, float)>;

struct BehaviorContext
{
	float dt = 0.0f;
	GOC* onwer = nullptr;
	BlackBoard* blackboard = nullptr;
	SpawnHitBoxFn spawnHitBox;
	SpawnProjectileFn spawnProjectile;
};