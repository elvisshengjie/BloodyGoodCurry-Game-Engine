/*********************************************************************************************
 \file      PhysicSystem.cpp
 \par       SofaSpuds
 \author    Ho Jun (h.jun@digipen.edu) - Primary Author, 100%
 \brief     Lightweight 2D physics step: AABB moves/collisions plus zoom-trigger checks.
 \details   Updates Transform by RigidBody velocity (dt) with axis-separated AABB tests
            against same-layer rigidbodies (excluding zoom triggers), then checks zoom
            trigger overlap for eligible objects. Includes simple layer filtering and
            knockback decay.
 \copyright
            All content ?025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "PhysicSystem.h"
#include <iostream>
#include <algorithm>
#include <cmath>

#include "Component/SpriteAnimationComponent.h"
#include "Component/TransformComponent.h"
#include "Components/PlayerComponent.h"
#include "Factory/Factory.h"
#include "Physics/Dynamics/RigidBodyComponent.h"
#include "Common/CRTDebug.h"   // <- bring in DBG_NEW

#ifdef _DEBUG
#define new DBG_NEW       // <- redefine new AFTER all includes
#endif
namespace Framework {

    namespace
    {
        constexpr float kCollisionEpsilon = 0.0005f;

        bool EqualsIgnoreCase(std::string_view a, std::string_view b)
        {
            if (a.size() != b.size())
                return false;

            for (std::size_t i = 0; i < a.size(); ++i)
            {
                if (std::tolower(static_cast<unsigned char>(a[i])) !=
                    std::tolower(static_cast<unsigned char>(b[i])))
                {
                    return false;
                }
            }

            return true;
        }

        bool RangesOverlap(float minA, float maxA, float minB, float maxB)
        {
            return minA < maxB && maxA > minB;
        }

        bool IsPlayerBody(const GOC* obj)
        {
            return obj &&
                obj->GetComponentType<PlayerComponent>(ComponentTypeId::CT_PlayerComponent) != nullptr;
        }

        bool IsHeiBangDashing(const GOC* obj)
        {
            if (!obj || !EqualsIgnoreCase(obj->GetObjectName(), "heibang"))
                return false;

            auto* anim =
                obj->GetComponentType<SpriteAnimationComponent>(ComponentTypeId::CT_SpriteAnimationComponent);
            const auto* active = anim ? anim->ActiveAnimation() : nullptr;
            return active && EqualsIgnoreCase(active->name, "dash");
        }

        bool ShouldIgnoreBodyCollision(const GOC* a, const GOC* b)
        {
            return (IsHeiBangDashing(a) && IsPlayerBody(b)) ||
                (IsHeiBangDashing(b) && IsPlayerBody(a));
        }
    }

    /*************************************************************************************
      \brief  Construct the physics system.
    *************************************************************************************/
    PhysicSystem::PhysicSystem() {
    }

    /*************************************************************************************
      \brief  Initialize physics state/resources (currently no-op).
    *************************************************************************************/
    void PhysicSystem::Initialize() {
        // Intentionally empty; kept for symmetry and future extensions.
    }

    /*************************************************************************************
      \brief  Advance physics one step: move bodies and resolve simple AABB collisions.
      \param  dt  Delta time (seconds).
      \note   Movement is axis-separated: X and Y are tested independently for wall hits.
               Solid collisions apply to any same-layer RigidBodyComponent.
    *************************************************************************************/
    void PhysicSystem::Update(float dt)
    {
        auto& objects = FACTORY->Objects();

        // Build the uniform grid from the current frame snapshot so every body sees
        // all potential neighbors regardless of iteration order.
        m_grid.Clear();
        auto& layers = FACTORY->Layers();
        for (auto& [id, obj] : objects)
        {
            if (!obj)
                continue;

            const LayerKey objectLayer = layers.LayerKeyFor(obj->GetId());
            if (!layers.IsLayerEnabled(objectLayer))
                continue;

            auto* rb = obj->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent);
            auto* tr = obj->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
            if (!rb || !tr)
                continue;

            AABB box(tr->x, tr->y, rb->width, rb->height);
            m_grid.Insert(id, box);
        }
        // --- Kinematic step with AABB collisions against solid bodies on the same layer ----------

        for (auto& [id, obj] : objects)
        {
            if (!obj)
                continue;

            const LayerKey objectLayer = layers.LayerKeyFor(obj->GetId());
            if (!layers.IsLayerEnabled(objectLayer))
                continue;

            auto* rb = obj->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent);
            auto* tr = obj->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
            if (!rb || !tr)
                continue;

            // ------------------------------
            // START OF KNOCKBACK APPLICATION 
            // ------------------------------
            float totalVelX = rb->velX;
            float totalVelY = rb->velY;
            if (rb->knockbackTime > 0.0f)
            {
                totalVelX += rb->knockVelX;
                totalVelY += rb->knockVelY;
            }
            // ------------------------------
            // END OF KNOCKBACK APPLICATION
            // ------------------------------
            
            const float oldX = tr->x;
            const float oldY = tr->y;

            // Integrate proposed new position
            float newX = tr->x + totalVelX * dt;
            float newY = tr->y + totalVelY * dt;


            // Sweep all objects on the same layer, checking solid rigidbodies.
            
            // Replaced with a model.
            // Explanation: The old code USED TO check every other object in the world,
            // now, the logic will only check the objects that are near.
            std::vector<GOCId> candidates;

            // Query using the full swept bounds so edge/corner contacts are not missed.
            AABB broadphaseBox((oldX + newX) * 0.5f, (oldY + newY) * 0.5f,
                std::fabs(newX - oldX) + rb->width,
                std::fabs(newY - oldY) + rb->height);
            m_grid.Query(broadphaseBox, candidates);

            const float selfHalfW = rb->width * 0.5f;
            const float selfHalfH = rb->height * 0.5f;

            // Resolve X against all nearby solids first.
            for (GOCId otherId : candidates)
            {
                auto it = objects.find(otherId);
                if (it == objects.end())
                    continue;

                auto& otherObj = it->second;
                if (!otherObj || otherObj == obj)
                    continue;

                const LayerKey otherLayer = layers.LayerKeyFor(otherObj->GetId());
                if (!layers.IsLayerEnabled(otherLayer) || otherLayer != objectLayer)
                    continue;

                auto* rbO = otherObj->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent);
                auto* trO = otherObj->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
                if (!rbO || !trO)
                    continue;
                if (ShouldIgnoreBodyCollision(obj.get(), otherObj.get()))
                    continue;

                const float otherHalfW = rbO->width * 0.5f;
                const float otherHalfH = rbO->height * 0.5f;
                if (!RangesOverlap(oldY - selfHalfH, oldY + selfHalfH,
                    trO->y - otherHalfH, trO->y + otherHalfH))
                    continue;

                AABB otherBox(trO->x, trO->y, rbO->width, rbO->height);
                AABB proposedXBox(newX, oldY, rb->width, rb->height);
                if (!Collision::CheckCollisionRectToRect(proposedXBox, otherBox))
                    continue;

                if (totalVelX > 0.0f)
                {
                    const float otherLeft = trO->x - otherHalfW;
                    newX = std::min(newX, otherLeft - selfHalfW - kCollisionEpsilon);
                    rb->velX = 0.0f;
                    rb->knockVelX = 0.0f;
                }
                else if (totalVelX < 0.0f)
                {
                    const float otherRight = trO->x + otherHalfW;
                    newX = std::max(newX, otherRight + selfHalfW + kCollisionEpsilon);
                    rb->velX = 0.0f;
                    rb->knockVelX = 0.0f;
                }
            }

            // Resolve Y using the X-resolved position.
            for (GOCId otherId : candidates)
            {
                auto it = objects.find(otherId);
                if (it == objects.end())
                    continue;

                auto& otherObj = it->second;

                if (!otherObj || otherObj == obj)
                    continue;
                const LayerKey otherLayer = layers.LayerKeyFor(otherObj->GetId());
                if (!layers.IsLayerEnabled(otherLayer))
                    continue;
                if (!(otherLayer == objectLayer))
                    continue;

                auto* rbO = otherObj->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent);
                auto* trO = otherObj->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
                if (!rbO || !trO)
                    continue;
                if (ShouldIgnoreBodyCollision(obj.get(), otherObj.get()))
                    continue;

                const float otherHalfW = rbO->width * 0.5f;
                const float otherHalfH = rbO->height * 0.5f;
                if (!RangesOverlap(newX - selfHalfW, newX + selfHalfW,
                    trO->x - otherHalfW, trO->x + otherHalfW))
                    continue;

                AABB otherBox(trO->x, trO->y, rbO->width, rbO->height);
                AABB proposedYBox(newX, newY, rb->width, rb->height);
                if (!Collision::CheckCollisionRectToRect(proposedYBox, otherBox))
                    continue;

                if (totalVelY > 0.0f)
                {
                    const float otherBottom = trO->y - otherHalfH;
                    newY = std::min(newY, otherBottom - selfHalfH - kCollisionEpsilon);
                    rb->velY = 0.0f;
                    rb->knockVelY = 0.0f;
                }
                else if (totalVelY < 0.0f)
                {
                    const float otherTop = trO->y + otherHalfH;
                    newY = std::max(newY, otherTop + selfHalfH + kCollisionEpsilon);
                    rb->velY = 0.0f;
                    rb->knockVelY = 0.0f;
                }
            }
            // Commit final position
            tr->x = newX;
            tr->y = newY;

            // ------------------------------
            // KNOCKBACK DECAY
            // ------------------------------
            if (rb->knockbackTime > 0.0f)
            {
                rb->knockbackTime -= dt;

                // Optional damping for nicer feel
                rb->knockVelX *= 0.95f;
                rb->knockVelY *= 0.95f;

                if (rb->knockbackTime <= 0.0f)
                {
                    rb->knockVelX = 0.0f;
                    rb->knockVelY = 0.0f;
                }
            }
        }
    }

    /*************************************************************************************
      \brief  Release physics resources (currently no-op).
    *************************************************************************************/
    void PhysicSystem::Shutdown() {
        // Intentionally empty; add resource teardown when needed.
    }

} // namespace Framework

