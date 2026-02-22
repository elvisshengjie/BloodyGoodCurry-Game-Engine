#pragma once
#include <functional>
#include "Blackboard.h"
#include "Composition/Composition.h"

struct BehaviorContext
{
    float dt = 0.0f;
    Framework::GOC* owner = nullptr;  // Use full type
    BlackBoard* blackboard = nullptr;
    std::function<void(Framework::GOC*, float, float, float, float, float, float, float)> spawnHitBox;
    std::function<void(Framework::GOC*, float, float, float, float, float, float, float, float, float)> spawnProjectile;
};