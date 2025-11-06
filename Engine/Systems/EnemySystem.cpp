/*********************************************************************************************
 \file      EnemySystem.cpp
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu)
 \brief     Skeleton system for enemy lifecycle control (init, per-frame update, draw, shutdown).
 \details   This module is the staging point for enemy-related logic. It currently stubs out
            the standard system hooks and keeps a pointer to the active window for any
            resolution-dependent logic you may add later.

            Typical responsibilities once implemented:
              - Spawning/despawning waves or individual enemies.
              - Driving enemy state machines / behavior trees (patrol, chase, attack, flee).
              - Coordinating with physics (RigidBody), perception (raycasts, FOV cones),
                and combat subsystems (HitBox/HurtBox, health).
              - Rendering debug overlays (paths, targets, aggro radii) in draw().
              - Saving/loading enemy state across levels.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "EnemySystem.h"
#include <iostream>

using namespace Framework;

EnemySystem::EnemySystem(gfx::Window& window) : window(&window) {}

void EnemySystem::Initialize()
{
    

}

void EnemySystem::Update(float dt)
{
    (void)dt;
}
void EnemySystem::draw(){}

/*****************************************************************************************
  \brief Optional debug visualization hook for enemies.
         Keep this lightweight�avoid gameplay mutations here.
*****************************************************************************************/
void EnemySystem::draw()
{
    // TODO: Draw debug paths, aggro circles, target lines, etc.
}

/*****************************************************************************************
  \brief Tear down any resources allocated by Initialize() or Update().
         Ensure all enemy-owned objects are released cleanly.
*****************************************************************************************/
void EnemySystem::Shutdown()
{

}


