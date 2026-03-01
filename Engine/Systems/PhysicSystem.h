/*********************************************************************************************
 \file      PhysicSystem.h
 \par       SofaSpuds
 \author    Ho Jun (h.jun@digipen.edu) - Primary Author, 100%
 \brief     Declares a lightweight 2D physics system for kinematic AABB movement.
 \details   Steps rigid bodies with velocity-based integration, resolves simple AABB
            collisions (axis-separated) against same-layer scene geometry, and handles
            zoom-trigger overlap checks. Designed as an engine subsystem driven by
            SystemManager (Initialize → Update(dt) → Shutdown).
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once
#include "Common/System.h"
#include "Physics/Collision/Collision.h"
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include "Composition/Composition.h"
#include "Systems/UniformGrid.h"

namespace Framework {

    /*************************************************************************************
      \class  PhysicSystem
      \brief  Minimal physics step for 2D games: kinematic update + simple collisions.
      \note   Uses the shared Factory/component model to iterate scene bodies.
    *************************************************************************************/
    class PhysicSystem : public Framework::ISystem {
    public:
        /*************************************************************************
          \brief  Construct the physics system.
        *************************************************************************/
        PhysicSystem();

        /*************************************************************************
          \brief  Initialize physics state/resources (no-op by default).
        *************************************************************************/
        void Initialize() override;

        /*************************************************************************
          \brief  Advance physics one frame (integrate + collide + hitboxes).
          \param  dt  Delta time in seconds.
        *************************************************************************/
        void Update(float dt) override;

        /*************************************************************************
          \brief  Release physics resources (no-op by default).
        *************************************************************************/
        void Shutdown() override;

        /*************************************************************************
          \brief  System name for diagnostics/profiling.
        *************************************************************************/
        std::string GetName() override { return "PhysicSystem"; }

    private:
        UniformGrid m_grid; 
    };

} // namespace Framework
