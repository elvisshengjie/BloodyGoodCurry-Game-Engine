/*********************************************************************************************
 \file      BehaviorContext.h
 \par       SofaSpuds
 \author
 \brief     Defines the runtime context passed into behavior execution.
 \details   Bundles the owner object, delta time, shared blackboard, and gameplay callback
            hooks used by behavior-tree and decision logic during evaluation.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

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
