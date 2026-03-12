/*********************************************************************************************
 \file      BehaviorContext.h
 \par       SofaSpuds
 \author    Choo Jian Wei (jianwei.c@digipen.edu) - Primary Author, 100%
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
namespace Framework
{
    /*****************************************************************************************
      \struct BehaviorContext
      \brief  Runtime context bundle passed into every behavior tree node during evaluation.
      \details
      Aggregates everything a behavior action or condition needs to execute without
      holding long-term references. Passed by reference from the tree's per-frame
      tick down through every node evaluation.
    *****************************************************************************************/
    struct BehaviorContext
    {
        float dt = 0.0f;
        Framework::GOC* owner = nullptr;  // Use full type
        BlackBoard* blackboard = nullptr;
        std::function<void(Framework::GOC*, float, float, float, float, float, float, float)> spawnHitBox;
        std::function<void(Framework::GOC*, float, float, float, float, float, float, float, float, float)> spawnProjectile;
    };
}
