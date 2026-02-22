#pragma once
#include "AI/BehaviorContext.h"
#include "Composition/Composition.h"
#include "Component/EnemyDecisionTreeComponent.h"
#include "Component/EnemyAttackComponent.h"
#include "Component/EnemyTypeComponent.h"
#include "Component/EnemyHealthComponent.h"
#include "Physics/Dynamics/RigidBodyComponent.h"
#include "Component/TransformComponent.h"
#include "Component/SpriteAnimationComponent.h"
#include "Component/AudioComponent.h"
#include "Component/PlayerComponent.h"
#include "Component/HitBoxComponent.h"
#include "Physics/System/Physics.h"
#include "Factory/Factory.h"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <string_view>

namespace Framework
{
    
    inline int FindAnimationIndex(SpriteAnimationComponent* anim, std::string_view desired)
    {
        if (!anim) return -1;
        for (std::size_t i = 0; i < anim->animations.size(); ++i)
        {
            const auto& name = anim->animations[i].name;
            if (name.size() != desired.size()) continue;

            bool match = true;
            for (std::size_t j = 0; j < name.size(); ++j)
            {
                if (std::tolower(static_cast<unsigned char>(name[j])) !=
                    std::tolower(static_cast<unsigned char>(desired[j])))
                {
                    match = false;
                    break;
                }
            }
            if (match) return static_cast<int>(i);
        }
        return -1;
    }

    inline void PlayAnim(Framework::GOC* goc, std::string_view name)
    {
        if (!goc) return;
        auto* anim = goc->GetComponentType<SpriteAnimationComponent>(ComponentTypeId::CT_SpriteAnimationComponent);
        if (!anim) return;
        int idx = FindAnimationIndex(anim, name);
        if (idx >= 0 && idx != anim->ActiveAnimationIndex())
            anim->SetActiveAnimation(idx);
    }

    inline float GetAnimDuration(Framework::GOC* goc, const std::string& name)
    {
        if (!goc) return 0.2f;
        auto* anim = goc->GetComponentType<SpriteAnimationComponent>(ComponentTypeId::CT_SpriteAnimationComponent);
        if (!anim) return 0.2f;

        for (const auto& a : anim->animations)
            if (a.name == name)
                return a.config.totalFrames / a.config.fps;

        return 0.2f;
    }

    inline GameObjectComposition* FindPlayer()
    {
        for (auto& pair : FACTORY->Objects())
        {
            if (!pair.second) continue;
            GOC* goc = pair.second.get();
            if (goc->GetComponent(ComponentTypeId::CT_PlayerComponent) != nullptr)
                return goc;
        }
        return nullptr;
    }
    

    // ------------------------ PATROL ------------------------
    inline void Patrol(BehaviorContext& ctx)
    {
        GOC* enemy = ctx.owner;
        if (!enemy) return;

        auto* rb = enemy->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent);
        auto* tr = enemy->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
        auto* ai = enemy->GetComponentType<EnemyDecisionTreeComponent>(ComponentTypeId::CT_EnemyDecisionTreeComponent);

        if (!rb || !tr || !ai) return;

        if (!ai->patrolOriginSet)
        {
            ai->patrolOriginX = tr->x;
            ai->patrolOriginY = tr->y;
            ai->patrolOriginSet = true;
            if (ai->dir == 0.0f) ai->dir = 1.0f;
        }

        constexpr float patrolSpeed = 0.6f;
        constexpr float patrolRange = 10.0f;
        constexpr float pauseDuration = 2.0f;

        float leftEdge = ai->patrolOriginX - patrolRange;
        float rightEdge = ai->patrolOriginX + patrolRange;

        if (ai->pauseTimer > 0.0f)
        {
            ai->pauseTimer -= ctx.dt;
            rb->velX = rb->velY = 0.0f;
            return;
        }

        rb->velX = patrolSpeed * ai->dir;
        rb->velY = 0.0f;

        float futureX = tr->x + rb->velX * ctx.dt;
        AABB futureBox(futureX, tr->y, rb->width, rb->height);

        bool collisionDetected = false;
        for (auto& pair : FACTORY->Objects())
        {
            if (!pair.second) continue;
            GOC* goc = pair.second.get();

            auto* rbO = goc->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent);
            auto* trO = goc->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
            if (!rbO || !trO) continue;

            std::string name = goc->GetObjectName();
            std::transform(name.begin(), name.end(), name.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (name == "rect")
            {
                AABB wallBox(trO->x, trO->y, rbO->width, rbO->height);
                if (Collision::CheckCollisionRectToRect(futureBox, wallBox))
                {
                    collisionDetected = true;
                    break;
                }
            }
        }

        if (collisionDetected || futureX <= leftEdge || futureX >= rightEdge)
        {
            ai->dir *= -1.0f;
            ai->pauseTimer = pauseDuration;
            rb->velX = 0.0f;
        }

        ai->prevX = tr->x;
        PlayAnim(enemy, "idle");
    }

    // ------------------------ MELEE ATTACK ------------------------
    inline void MeleeAttack(BehaviorContext& ctx)
    {
        GameObjectComposition* enemy = ctx.owner;
        if (!enemy) return;

        auto* attack = enemy->GetComponentType<EnemyAttackComponent>(ComponentTypeId::CT_EnemyAttackComponent);
        auto* rb = enemy->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent);
        auto* tr = enemy->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
        auto* ai = enemy->GetComponentType<EnemyDecisionTreeComponent>(ComponentTypeId::CT_EnemyDecisionTreeComponent);
        auto* audio = enemy->GetComponentType<AudioComponent>(ComponentTypeId::CT_AudioComponent);

        auto* player = FindPlayer();
        if (!attack || !rb || !tr || !ai || !player) return;

        auto* trPlayer = player->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
        if (!trPlayer) return;

        float dx = trPlayer->x - tr->x;
        float dy = trPlayer->y - tr->y;
        float distance = std::sqrt(dx * dx + dy * dy);

        constexpr float speed = 1.0f;
        constexpr float accel = 2.0f;
        constexpr float stopDist = 0.1f;

        if (distance > stopDist)
        {
            float norm = (distance > 0.001f) ? distance : 1.0f;
            float targetVX = (dx / norm) * speed;
            float targetVY = (dy / norm) * speed;
            rb->velX += (targetVX - rb->velX) * std::min(accel * ctx.dt, 1.0f);
            rb->velY += (targetVY - rb->velY) * std::min(accel * ctx.dt, 1.0f);
        }
        else
        {
            rb->velX *= 0.5f;
            rb->velY *= 0.5f;
        }

        ai->facing = (dx < 0.0f) ? Facing::LEFT : Facing::RIGHT;
        attack->attack_timer += ctx.dt;

        if (attack->attack_timer >= attack->attack_speed && !attack->hitbox->active && distance < 0.8f)
        {
            attack->attack_timer = 0.0f;
            attack->hitbox->active = true;

            float direction = (ai->facing == Facing::LEFT) ? -1.0f : 1.0f;
            float hbWidth = rb->width * 1.2f;
            float hbHeight = rb->height * 0.8f;
            float spawnX = tr->x + (direction * hbWidth * 0.25f);
            float spawnY = tr->y;

            attack->hitbox->duration = GetAnimDuration(enemy, "slashattack");

            ctx.spawnHitBox(enemy, spawnX, spawnY, hbWidth, hbHeight,
                static_cast<float>(attack->damage),
                attack->hitbox->duration, 0.0f);

            if (audio) audio->TriggerSound("EnemyAttack");
            PlayAnim(enemy, "slashattack");
        }

        if (attack->hitbox->active)
        {
            attack->hitboxElapsed += ctx.dt;
            if (attack->hitboxElapsed >= attack->hitbox->duration)
            {
                attack->hitbox->active = false;
                attack->hitboxElapsed = 0.0f;
                PlayAnim(enemy, "idle");
            }
        }

        if (distance > 0.5f)
        {
            ai->chaseTimer += ctx.dt;
            if (ai->chaseTimer >= ai->maxChaseDuration)
            {
                ai->hasSeenPlayer = false;
                ai->chaseTimer = 0.0f;
                ctx.blackboard->Set<bool>("hasSeenPlayer", false);
            }
        }
        else
        {
            ai->chaseTimer = 0.0f;
            ai->hasSeenPlayer = true;
        }
    }

    // ------------------------ RANGED ATTACK ------------------------
    inline void RangedAttack(BehaviorContext& ctx)
    {
        GOC* enemy = ctx.owner;
        if (!enemy) return;

        auto* attack = enemy->GetComponentType<EnemyAttackComponent>(ComponentTypeId::CT_EnemyAttackComponent);
        auto* rb = enemy->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent);
        auto* tr = enemy->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
        auto* ai = enemy->GetComponentType<EnemyDecisionTreeComponent>(ComponentTypeId::CT_EnemyDecisionTreeComponent);
        auto* audio = enemy->GetComponentType<AudioComponent>(ComponentTypeId::CT_AudioComponent);

        auto* player = FindPlayer();
        if (!attack || !rb || !tr || !ai || !player) return;

        auto* trPlayer = player->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
        if (!trPlayer) return;

        float dx = trPlayer->x - tr->x;
        float dy = trPlayer->y - tr->y;
        float distance = std::sqrt(dx * dx + dy * dy);
        float norm = distance > 0.001f ? distance : 1.0f;
        float dirX = dx / norm;
        float dirY = dy / norm;

        constexpr float speed = 1.0f;
        constexpr float retreatSpeed = 0.45f;
        constexpr float minDist = 0.5f;
        constexpr float maxDist = 1.2f;
        constexpr float retreatDuration = 3.0f;

        float& retreatTimer = ai->retreatTimer;

        if (retreatTimer > 0.0f)
        {
            retreatTimer -= ctx.dt;
            rb->velX = -dirX * retreatSpeed;
            rb->velY = -dirY * retreatSpeed;
        }
        else if (distance < minDist)
        {
            rb->velX = ((rand() % 100) < 20) ? -dirX * retreatSpeed : rb->velX * 0.5f;
        }
        else if (distance > maxDist)
        {
            rb->velX = dirX * speed;
        }
        else
        {
            rb->velX *= 0.85f;
        }

        ai->facing = (dx < 0.0f) ? Facing::LEFT : Facing::RIGHT;
        attack->attack_timer += ctx.dt;

        if (attack->attack_timer >= attack->attack_speed && retreatTimer <= 0.0f && distance < 3.5f)
        {
            attack->attack_timer = 0.0f;

            float spawnX = tr->x + dirX * (std::max(rb->width, rb->height) * 0.5f + 0.1f);
            float spawnY = tr->y + dirY * (std::max(rb->width, rb->height) * 0.5f + 0.1f);

            ctx.spawnProjectile(enemy, spawnX, spawnY, dirX, dirY, 0.2f, 0.3f, 0.15f,
                static_cast<float>(attack->damage), 3.0f);

            if (audio) audio->TriggerSound("EnemyAttack");
            PlayAnim(enemy, "rangeattack");
            retreatTimer = retreatDuration;
        }

        if (attack->attack_timer > 0.5f)
            PlayAnim(enemy, "idle");

        if (distance > 4.0f)
        {
            ai->chaseTimer += ctx.dt;
            if (ai->chaseTimer >= ai->maxChaseDuration)
            {
                ai->hasSeenPlayer = false;
                ai->chaseTimer = 0.0f;
                ctx.blackboard->Set<bool>("hasSeenPlayer", false);
            }
        }
        else
        {
            ai->chaseTimer = 0.0f;
            ai->hasSeenPlayer = true;
        }
    }
}