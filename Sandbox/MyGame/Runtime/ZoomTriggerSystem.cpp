/*********************************************************************************************
 \file      ZoomTriggerSystem.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Implements the game-owned ZoomTriggerSystem for BloodyGoodCurry.

 \details   The system scans the live object list for the current player and active zoom
            trigger objects, checks AABB overlap using the latest physics positions, and
            applies the target camera view height through the engine RenderSystem.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "Runtime/ZoomTriggerSystem.h"

#include "Common/GameComponentIDs.h"
#include "Components/PlayerComponent.h"
#include "Components/ZoomTriggerComponent.h"
#include "Component/TransformComponent.h"
#include "Factory/Factory.h"
#include "Physics/Collision/Collision.h"
#include "Physics/Dynamics/RigidBodyComponent.h"
#include "Systems/RenderSystem.h"

namespace Framework
{
    /*************************************************************************************
     \brief  Evaluates all zoom triggers against the current player position.
     \param  dt Delta time in seconds. Unused because the system only reacts to overlap.
    *************************************************************************************/
    void ZoomTriggerSystem::Update(float dt)
    {
        (void)dt;

        if (!FACTORY)
            return;

        for (auto& [id, obj] : FACTORY->Objects())
        {
            (void)id;
            if (!obj)
                continue;

            auto* player = obj->GetComponentType<PlayerComponent>(mygame::CT_PlayerComponent());
            auto* playerTransform = obj->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
            auto* playerBody = obj->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent);
            if (!(player && playerTransform && playerBody))
                continue;

            const AABB playerBox(playerTransform->x, playerTransform->y, playerBody->width, playerBody->height);

            for (auto& [otherId, other] : FACTORY->Objects())
            {
                (void)otherId;
                if (!other || other.get() == obj.get())
                    continue;

                auto* trigger = other->GetComponentType<ZoomTriggerComponent>(mygame::CT_ZoomTriggerComponent());
                auto* triggerTransform = other->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
                auto* triggerBody = other->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent);
                if (!(trigger && triggerTransform && triggerBody))
                    continue;

                const AABB triggerBox(triggerTransform->x, triggerTransform->y, triggerBody->width, triggerBody->height);
                if (!Collision::CheckCollisionRectToRect(playerBox, triggerBox))
                    continue;

                if (trigger->triggered)
                    continue;

                trigger->triggered = true;
                if (auto* render = RenderSystem::Get())
                    render->SetCameraViewHeight(trigger->targetZoom);
            }
        }
    }
}
