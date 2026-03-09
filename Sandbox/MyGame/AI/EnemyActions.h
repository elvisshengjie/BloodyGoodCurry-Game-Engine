/*********************************************************************************************
 \file      EnemyActions.h
 \par       SofaSpuds
 \author    Choo Jian Wei - Primary Author (100%)
 \brief     Declares game-specific AI action helpers for enemy behaviour execution.
 \details   Provides small action routines used by the sandbox enemy AI layer to
            drive movement, attacks, and state changes through the engine AI context.

 \changelog
            Applied slowTimer/slowMultiplier from EnemyComponent to all
            movement velocity sets in Patrol, MeleeAttack, RangedAttack.

 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include "AI/BehaviorContext.h"
#include "Composition/Composition.h"
#include "Components/EnemyDecisionTreeComponent.h"
#include "Components/EnemyAttackComponent.h"
#include "Components/EnemyTypeComponent.h"
#include "Components/EnemyHealthComponent.h"
#include "Physics/Dynamics/RigidBodyComponent.h"
#include "Component/TransformComponent.h"
#include "Component/SpriteAnimationComponent.h"
#include "Component/AudioComponent.h"
#include "Components/PlayerComponent.h"
#include "Component/HitBoxComponent.h"
#include "Common/GameComponentIDs.h"
#include "Physics/System/Physics.h"
#include "Factory/Factory.h"
#include "../Audio/GameAudioSetup.h"
#include "EnemyConditions.h" 
#include <cmath>
#include <algorithm>
#include <cctype>
#include <string_view>

namespace mygame
{

    static constexpr float kEnemyProjectileBaseSpeed = 0.6f;
    static constexpr float kRangedAttackFireDist = kDetectionRadius;  // 3.5f
    static constexpr float kMeleeAttackDist = 0.8f;

    inline int FindAnimationIndex(Framework::SpriteAnimationComponent* anim, std::string_view desired)
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
        auto* anim = goc->GetComponentType<Framework::SpriteAnimationComponent>
            (Framework::ComponentTypeId::CT_SpriteAnimationComponent);
        if (!anim) return;
        int idx = FindAnimationIndex(anim, name);
        if (idx >= 0 && idx != anim->ActiveAnimationIndex())
            anim->SetActiveAnimation(idx);
    }

    inline float GetAnimDuration(Framework::GOC* goc, const std::string& name)
    {
        if (!goc) return 0.2f;
        auto* anim = goc->GetComponentType<Framework::SpriteAnimationComponent>
            (Framework::ComponentTypeId::CT_SpriteAnimationComponent);
        if (!anim) return 0.2f;

        for (const auto& a : anim->animations)
            if (a.name == name)
                return a.config.totalFrames / a.config.fps;

        return 0.2f;
    }

    inline Framework::GOC* FindPlayer()
    {
        for (auto& pair : Framework::FACTORY->Objects())
        {
            if (!pair.second) continue;
            GOC* goc = pair.second.get();
            if (goc->GetComponent(Framework::ComponentTypeId::CT_PlayerComponent) != nullptr)
                return goc;
        }
        return nullptr;
    }

    inline void ApplySlow(Framework::GOC* enemy, Framework::RigidBodyComponent* rb, float dt)
    {
        auto* enemyComp = enemy->GetComponentType<Framework::EnemyComponent>(CT_EnemyComponent());
        if (!enemyComp) return;

        if (enemyComp->slowTimer > 0.0f)
        {
            enemyComp->slowTimer -= dt;

            rb->velX *= enemyComp->slowMultiplier;
            rb->velY *= enemyComp->slowMultiplier;

            if (enemyComp->slowTimer <= 0.0f)
            {
                enemyComp->slowTimer = 0.0f;
                enemyComp->slowMultiplier = 1.0f;
            }
        }
    }



    // ------------------------ PATROL ------------------------
    inline void Patrol(Framework::BehaviorContext& ctx)
    {
        GOC* enemy = ctx.owner;
        if (!enemy) return;

        auto* rb = enemy->GetComponentType<Framework::RigidBodyComponent>(Framework::ComponentTypeId::CT_RigidBodyComponent);
        auto* tr = enemy->GetComponentType<Framework::TransformComponent>(Framework::ComponentTypeId::CT_TransformComponent);
        auto* ai = enemy->GetComponentType<Framework::EnemyDecisionTreeComponent>(Framework::ComponentTypeId::CT_EnemyDecisionTreeComponent);

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
        Framework::AABB futureBox(futureX, tr->y, rb->width, rb->height);

        bool collisionDetected = false;
        for (auto& pair : Framework::FACTORY->Objects())
        {
            if (!pair.second) continue;
            GOC* goc = pair.second.get();

            auto* rbO = goc->GetComponentType<Framework::RigidBodyComponent>(Framework::ComponentTypeId::CT_RigidBodyComponent);
            auto* trO = goc->GetComponentType<Framework::TransformComponent>(Framework::ComponentTypeId::CT_TransformComponent);
            if (!rbO || !trO) continue;

            std::string name = goc->GetObjectName();
            std::transform(name.begin(), name.end(), name.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (name == "rect")
            {
                Framework::AABB wallBox(trO->x, trO->y, rbO->width, rbO->height);
                if (Framework::Collision::CheckCollisionRectToRect(futureBox, wallBox))
                {
                    collisionDetected = true;
                    break;
                }
            }
        }

        if ((collisionDetected || futureX <= leftEdge || futureX >= rightEdge)
            && ai->pauseTimer <= 0.0f)
        {
            ai->dir *= -1.0f;
            ai->pauseTimer = pauseDuration;
            rb->velX = 0.0f;
        }

        ai->prevX = tr->x;
        PlayAnim(enemy, "idle");
        ApplySlow(enemy, rb, ctx.dt);
    }

    // ------------------------ MELEE ATTACK ------------------------
    inline void MeleeAttack(Framework::BehaviorContext& ctx)
    {
        Framework::GameObjectComposition* enemy = ctx.owner;
        if (!enemy) return;

        auto* attack = enemy->GetComponentType<Framework::EnemyAttackComponent>(Framework::ComponentTypeId::CT_EnemyAttackComponent);
        auto* rb = enemy->GetComponentType<Framework::RigidBodyComponent>(Framework::ComponentTypeId::CT_RigidBodyComponent);
        auto* tr = enemy->GetComponentType<Framework::TransformComponent>(Framework::ComponentTypeId::CT_TransformComponent);
        auto* ai = enemy->GetComponentType<Framework::EnemyDecisionTreeComponent>(Framework::ComponentTypeId::CT_EnemyDecisionTreeComponent);
        auto* audio = enemy->GetComponentType<Framework::AudioComponent>(Framework::ComponentTypeId::CT_AudioComponent);
        auto* player = FindPlayer();
        if (!attack || !rb || !tr || !ai || !player) return;

        auto* trPlayer = player->GetComponentType<Framework::TransformComponent>(Framework::ComponentTypeId::CT_TransformComponent);
        if (!trPlayer) return;

        float dx = trPlayer->x - tr->x;
        float dy = trPlayer->y - tr->y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (attack->hitbox->active)
        {
            rb->velX = 0.0f;
            rb->velY = 0.0f;
            attack->hitboxElapsed += ctx.dt;
            if (attack->hitboxElapsed >= attack->hitbox->duration)
            {
                attack->hitbox->active = false;
                attack->hitboxElapsed = 0.0f;
                PlayAnim(enemy, "idle");
            }
            return;
        }

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

        ai->facing = (dx < 0.0f) ? Framework::Facing::LEFT : Framework::Facing::RIGHT;
        attack->attack_timer += ctx.dt;
        if (attack->attack_timer >= attack->attack_speed && !attack->hitbox->active && distance < kMeleeAttackDist)
        {
            attack->attack_timer = 0.0f;
            if (ctx.spawnHitBox)
            {
                attack->hitbox->active = true;
                attack->hitboxElapsed = 0.0f;
                rb->velX = 0.0f;
                rb->velY = 0.0f;
                float direction = (ai->facing == Framework::Facing::LEFT) ? -1.0f : 1.0f;
                float hbWidth = rb->width * 1.2f;
                float hbHeight = rb->height * 0.8f;
                float spawnX = tr->x + (direction * hbWidth * 0.25f);
                float spawnY = tr->y;

                attack->hitbox->duration = GetAnimDuration(enemy, "slashattack");
                ctx.spawnHitBox(enemy, spawnX, spawnY, hbWidth, hbHeight,
                    static_cast<float>(attack->damage),
                    attack->hitbox->duration, 0.0f);

                if (audio)
                {
                    GameAudio gameAudio(audio, GameAudio::Entity::Enemy);
                    gameAudio.PlayAttack(tr->x, tr->x);
                }
                PlayAnim(enemy, "slashattack");
            }
        }

        if (distance > kChaseRetentionRadius)
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
        ApplySlow(enemy, rb, ctx.dt);
    }

    // ------------------------ RANGED ATTACK ------------------------
    inline void RangedAttack(Framework::BehaviorContext& ctx)
    {
        GOC* enemy = ctx.owner;
        if (!enemy) return;

        auto* attack = enemy->GetComponentType<Framework::EnemyAttackComponent>(Framework::ComponentTypeId::CT_EnemyAttackComponent);
        auto* rb = enemy->GetComponentType<Framework::RigidBodyComponent>(Framework::ComponentTypeId::CT_RigidBodyComponent);
        auto* tr = enemy->GetComponentType<Framework::TransformComponent>(Framework::ComponentTypeId::CT_TransformComponent);
        auto* ai = enemy->GetComponentType<Framework::EnemyDecisionTreeComponent>(Framework::ComponentTypeId::CT_EnemyDecisionTreeComponent);
        auto* audio = enemy->GetComponentType<Framework::AudioComponent>(Framework::ComponentTypeId::CT_AudioComponent);

        auto* player = FindPlayer();
        if (!attack || !rb || !tr || !ai || !player) return;

        auto* trPlayer = player->GetComponentType<Framework::TransformComponent>
            (Framework::ComponentTypeId::CT_TransformComponent);
        if (!trPlayer) return;

        float dx = trPlayer->x - tr->x;
        float dy = trPlayer->y - tr->y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (ai->rangedAttackActive)
        {
            rb->velX = 0.0f;
            rb->velY = 0.0f;
            ai->rangedAttackTimer += ctx.dt;
            if (ai->rangedAttackTimer >= ai->rangedAttackDuration)
            {
                if (ai->pendingProjectile)
                {
                    ctx.spawnProjectile(enemy,
                        ai->pendingProjectileSpawnX, ai->pendingProjectileSpawnY,
                        ai->pendingProjectileDirX, ai->pendingProjectileDirY,
                        kEnemyProjectileBaseSpeed, 0.15f, 0.08f,
                        static_cast<float>(attack->damage), 3.0f);
                    ai->pendingProjectile = false;
                }
                ai->rangedAttackActive = false;
                ai->rangedAttackTimer = 0.0f;
                ai->rangedAttackDuration = 0.0f;
                PlayAnim(enemy, "idle");
            }
            return;
        }

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
            rb->velX = ((rand() % 100) < 20) ? -dirX * retreatSpeed: rb->velX * 0.5f;
        }
        else if (distance > maxDist)
        {
            rb->velX = dirX * speed;
        }
        else
        {
            rb->velX *= 0.85f;
        }

        ai->facing = (dx < 0.0f) ? Framework::Facing::LEFT : Framework::Facing::RIGHT;
        attack->attack_timer += ctx.dt;

        if (attack->attack_timer >= attack->attack_speed && retreatTimer <= 0.0f && distance < kRangedAttackFireDist)
        {
            attack->attack_timer = 0.0f;

            ai->pendingProjectileDirX = dirX;
            ai->pendingProjectileDirY = dirY;
            ai->pendingProjectileSpawnX = tr->x + dirX * (std::max(rb->width, rb->height));
            ai->pendingProjectileSpawnY = tr->y + dirY * (std::max(rb->width, rb->height));
            ai->pendingProjectile = true;

            if (audio)
            {
                GameAudio gameAudio(audio, GameAudio::Entity::Enemy);
                gameAudio.PlayAttack(tr->x, tr->y);
            }
            PlayAnim(enemy, "rangeattack");
            ai->rangedAttackActive = true;
            ai->rangedAttackTimer = 0.0f;
            ai->rangedAttackDuration = GetAnimDuration(enemy, "rangeattack");
            std::cout << "[RangedAttack] animDuration=" << ai->rangedAttackDuration << "\n";
            rb->velX = 0.0f;
            rb->velY = 0.0f;
            retreatTimer = retreatDuration;
        }

        if (attack->attack_timer > 0.5f)
            PlayAnim(enemy, "idle");

        if (distance > kChaseRetentionRadius)
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
        ApplySlow(enemy, rb, ctx.dt);
    }
}