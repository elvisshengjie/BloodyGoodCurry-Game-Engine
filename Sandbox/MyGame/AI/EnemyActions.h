/*********************************************************************************************
 \file      EnemyActions.h
 \par       SofaSpuds
 \author    Choo Jian Wei - Primary Author (100%)
 \brief     Declares and defines game-specific AI action helpers for enemy behaviour execution.
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
#include "Component/RenderComponent.h"
#include "Component/TransformComponent.h"
#include "Component/SpriteAnimationComponent.h"
#include "Component/AudioComponent.h"
#include "Components/PlayerComponent.h"
#include "Component/HitBoxComponent.h"
#include "Common/GameComponentIDs.h"
#include "Physics/System/Physics.h"
#include "Factory/Factory.h"
#include "../VfxPresets.hpp"
#include "../Audio/GameAudioSetup.h"
#include "EnemyConditions.h" 
#include <cmath>
#include <algorithm>
#include <array>
#include <utility>
#include <cctype>
#include <string>
#include <string_view>

namespace mygame
{

    static constexpr float kEnemyProjectileBaseSpeed = 0.6f;
    static constexpr float kRangedAttackFireDist = kDetectionRadius;  // 3.5f
    static constexpr float kMeleeAttackDist = 0.8f;
    /*****************************************************************************************
      \brief Performs a case-insensitive search for an animation by name.
      \param anim    SpriteAnimationComponent to search within.
      \param desired Target animation name to match.
      \return Index of the matching animation, or -1 if not found.
    *****************************************************************************************/

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
    /*****************************************************************************************
      \brief Sets the active animation on an object's SpriteAnimationComponent by name.
      \param goc  Game object composition that owns the animation component.
      \param name Name of the animation to activate (case-insensitive).
      \details No-op if the object, component, or named animation cannot be found,
               or if the animation is already active.
    *****************************************************************************************/
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
    /*****************************************************************************************
      \brief Returns the total playback duration in seconds for a named animation.
      \param goc  Game object composition that owns the animation component.
      \param name Exact name of the animation to query.
      \return Duration in seconds (totalFrames / fps), or 0.2f as a safe fallback.
    *****************************************************************************************/
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

    inline bool EqualsIgnoreCase(std::string_view a, std::string_view b)
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

    inline bool HasAnim(Framework::GOC* goc, std::string_view name)
    {
        if (!goc)
            return false;

        auto* anim = goc->GetComponentType<Framework::SpriteAnimationComponent>(
            Framework::ComponentTypeId::CT_SpriteAnimationComponent);
        return FindAnimationIndex(anim, name) >= 0;
    }

    inline std::string_view ActiveAnimName(Framework::GOC* goc)
    {
        if (!goc)
            return {};

        auto* anim = goc->GetComponentType<Framework::SpriteAnimationComponent>(
            Framework::ComponentTypeId::CT_SpriteAnimationComponent);
        const auto* active = anim ? anim->ActiveAnimation() : nullptr;
        return active ? std::string_view(active->name) : std::string_view{};
    }

    inline std::string_view ResolveHeiBangAttack2FollowupAnim(Framework::GOC* goc)
    {
        static constexpr std::array<std::string_view, 3> kCandidateNames{
            "attack2_laser", "attack2_beam", "laserbeam2"
        };

        for (const auto name : kCandidateNames)
        {
            if (HasAnim(goc, name))
                return name;
        }

        return {};
    }
    /*****************************************************************************************
      \brief Searches the factory object list for the first object with a PlayerComponent.
      \return Pointer to the player game object, or nullptr if none exists.
    *****************************************************************************************/
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
    /*****************************************************************************************
      \brief Applies a speed penalty to the enemy's rigidbody if a slow effect is active.
      \param enemy Enemy game object that owns EnemyComponent.
      \param rb    RigidBodyComponent to scale velocity on.
      \param dt    Delta time in seconds, used to tick down slowTimer.
      \details Scales velX and velY by slowMultiplier each frame the timer is active.
               Resets slowMultiplier to 1.0f and clears slowTimer when it expires.
    *****************************************************************************************/
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

    inline void FaceTargetHorizontally(
        Framework::GOC* enemy,
        Framework::EnemyDecisionTreeComponent* ai,
        float dx)
    {
        if (!enemy || !ai || std::fabs(dx) <= 0.001f)
            return;

        ai->facing = (dx < 0.0f) ? Framework::Facing::LEFT : Framework::Facing::RIGHT;

        auto* render = enemy->GetComponentType<Framework::RenderComponent>(
            Framework::ComponentTypeId::CT_RenderComponent);
        if (!render)
            return;

        const float width = std::fabs(render->w);
        if (width <= 0.0f)
            return;

        render->w = (ai->facing == Framework::Facing::LEFT) ? -width : width;
    }

    /*****************************************************************************************
      \brief AI action: moves the enemy back and forth along a fixed horizontal patrol range.
      \param ctx BehaviorContext containing owner, dt, and blackboard.
      \details
      - Initialises patrolOriginX/Y on first call.
      - Reverses direction at patrol range edges or on wall collision.
      - Pauses briefly at each turn-around point.
      - Respects knockback guard and applies slow effect each frame.
    *****************************************************************************************/
    inline void Patrol(Framework::BehaviorContext& ctx)
    {
        GOC* enemy = ctx.owner;
        if (!enemy) return;

        auto* rb = enemy->GetComponentType<Framework::RigidBodyComponent>(Framework::ComponentTypeId::CT_RigidBodyComponent);
        auto* tr = enemy->GetComponentType<Framework::TransformComponent>(Framework::ComponentTypeId::CT_TransformComponent);
        auto* ai = enemy->GetComponentType<Framework::EnemyDecisionTreeComponent>(Framework::ComponentTypeId::CT_EnemyDecisionTreeComponent);

        if (!rb || !tr || !ai) return;
        // KNOCKBACK GUARD
        if (ai->knockbackTimer > 0.0f)
        {
            ai->knockbackTimer -= ctx.dt;
            rb->velX = 0.0f;
            rb->velY = 0.0f;
            if (ai->knockbackTimer <= 0.0f)
                PlayAnim(enemy, "idle");
            return;
        }

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

    /*****************************************************************************************
      \brief AI action: chases the player and delivers a melee hitbox strike.
      \param ctx BehaviorContext containing owner, dt, spawnHitBox callback, and blackboard.
      \details
      - Accelerates toward the player until within kMeleeAttackDist.
      - Spawns a hitbox when attack_timer exceeds attack_speed.
      - Holds position and waits for the hitbox duration to expire before re-enabling input.
      - Tracks chase retention; clears hasSeenPlayer if the player is out of range too long.
      - Respects knockback guard and applies slow effect each frame.
    *****************************************************************************************/
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
        // KNOCKBACK GUARD
        if (ai->knockbackTimer > 0.0f)
        {
            ai->knockbackTimer -= ctx.dt;
            rb->velX = 0.0f;
            rb->velY = 0.0f;
            if (ai->knockbackTimer <= 0.0f)
                PlayAnim(enemy, "idle");
            return;
        }
        auto* trPlayer = player->GetComponentType<Framework::TransformComponent>(Framework::ComponentTypeId::CT_TransformComponent);
        if (!trPlayer) return;

        float dx = trPlayer->x - tr->x;
        float dy = trPlayer->y - tr->y;
        float distance = std::sqrt(dx * dx + dy * dy);

        std::string enemyName = enemy->GetObjectName();
        std::transform(enemyName.begin(), enemyName.end(), enemyName.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        const bool isHeiBang = (enemyName == "heibang");
        const bool isNancie = (enemyName == "nancie");
        static constexpr std::array<std::pair<float, float>, 3> kHeiBangAttackPoints{ {
            {0.704178f, -1.02655f},
            {1.21166f, -2.18211f},
            {1.61999f, -1.57658f}
        } };

        if (isNancie)
            FaceTargetHorizontally(enemy, ai, dx);

        if (attack->hitbox->active)
        {
            rb->velX = 0.0f;
            rb->velY = 0.0f;
            attack->hitboxElapsed += ctx.dt;
            if (attack->hitboxElapsed >= attack->hitbox->duration)
            {
                const std::string_view activeAnim = ActiveAnimName(enemy);
                if (isHeiBang && ai->currentPathIndex == 1 &&
                    !attack->attack2BeamPhaseActive &&
                    EqualsIgnoreCase(activeAnim, "attack2"))
                {
                    attack->attack2BeamPhaseActive = true;
                    attack->hitboxElapsed = 0.0f;
                    attack->hitbox->duration = 14.0f / 12.0f;
                    const glm::vec2 beamTarget{ trPlayer->x, trPlayer->y };
                    mygame::SpawnHeiBangAttack2BeamVfx(*enemy, beamTarget);
                    if (ctx.spawnHitBox)
                    {
                        const float beamDamageWidth = std::max(0.18f, rb->width * 1.15f);
                        const float beamDamageHeight = std::max(0.22f, rb->height * 1.15f);
                        ctx.spawnHitBox(enemy, beamTarget.x, beamTarget.y,
                            beamDamageWidth, beamDamageHeight,
                            static_cast<float>(attack->damage),
                            attack->hitbox->duration, 0.0f);
                    }
                    return;
                }

                attack->hitbox->active = false;
                attack->hitboxElapsed = 0.0f;
                attack->attack2BeamPhaseActive = false;
                PlayAnim(enemy, "idle");
                if (isHeiBang)
                    ai->currentPathIndex = (ai->currentPathIndex + 1) % kHeiBangAttackPoints.size();
            }
            return;
        }

        if (isHeiBang)
        {
            if (ai->currentPathIndex >= kHeiBangAttackPoints.size())
                ai->currentPathIndex = 0;

            const auto [targetX, targetY] = kHeiBangAttackPoints[ai->currentPathIndex];
            float pDx = targetX - tr->x;
            float pDy = targetY - tr->y;
            float pointDistance = std::sqrt(pDx * pDx + pDy * pDy);

            constexpr float dashSpeed = 2.6f;
            constexpr float pointArriveDist = 0.08f;
            if (pointDistance > pointArriveDist)
            {
                float pointNorm = (pointDistance > 0.001f) ? pointDistance : 1.0f;
                rb->velX = (pDx / pointNorm) * dashSpeed;
                rb->velY = (pDy / pointNorm) * dashSpeed;
                PlayAnim(enemy, "dash");
                ApplySlow(enemy, rb, ctx.dt);
                return;
            }

            rb->velX = 0.0f;
            rb->velY = 0.0f;
            ai->facing = (dx < 0.0f) ? Framework::Facing::LEFT : Framework::Facing::RIGHT;
            attack->attack_timer += ctx.dt;
            if (attack->attack_timer >= attack->attack_speed)
            {
                attack->attack_timer = 0.0f;
                if (ctx.spawnHitBox)
                {
                    attack->hitbox->active = true;
                    attack->hitboxElapsed = 0.0f;
                    attack->attack2BeamPhaseActive = false;
                    const float direction = (ai->facing == Framework::Facing::LEFT) ? -1.0f : 1.0f;
                    const float hbWidth = rb->width * 1.2f;
                    const float hbHeight = rb->height * 0.8f;
                    const float spawnX = tr->x + (direction * hbWidth * 0.25f);
                    const float spawnY = tr->y;

                    static constexpr std::array<std::string_view, 3> kHeiBangAttackAnims{
                        "attack1", "attack2", "attack3"
                    };
                    const std::string attackAnim = std::string(kHeiBangAttackAnims[ai->currentPathIndex]);
                    attack->hitbox->duration = GetAnimDuration(enemy, attackAnim);
                    ctx.spawnHitBox(enemy, spawnX, spawnY, hbWidth, hbHeight,
                        static_cast<float>(attack->damage),
                        attack->hitbox->duration, 0.0f);

                    if (audio)
                    {
                        GameAudio gameAudio(audio, GameAudio::Entity::Enemy);
                        gameAudio.PlayAttack(tr->x, tr->y);
                    }
                    PlayAnim(enemy, attackAnim);
                }
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
            if (isNancie)
                PlayAnim(enemy, "dash");
            else
                PlayAnim(enemy, "idle");
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
               
                std::string meleeAnim = "slashattack";
                if (isNancie)
                {
                    // alternate between slashattack1 and slamattack2 each hit
                    meleeAnim = (ai->currentPathIndex % 2 == 0) ? "slashattack1" : "slamattack2";
                    ai->currentPathIndex++;
                }

                attack->hitbox->duration = GetAnimDuration(enemy, meleeAnim);
                ctx.spawnHitBox(enemy, spawnX, spawnY, hbWidth, hbHeight,
                    static_cast<float>(attack->damage),
                    attack->hitbox->duration, 0.0f);

                if (audio)
                {
                    GameAudio gameAudio(audio, GameAudio::Entity::Enemy);
                    gameAudio.PlayAttack(tr->x, tr->y);
                }
                PlayAnim(enemy, meleeAnim);
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

    /*****************************************************************************************
      \brief AI action: maintains distance from the player and fires projectiles.
      \param ctx BehaviorContext containing owner, dt, spawnProjectile callback, and blackboard.
      \details
      - Retreats when too close (< minDist) and advances when too far (> maxDist).
      - Fires a projectile after attack_speed elapses if within kRangedAttackFireDist.
      - Projectile spawn is deferred to the end of the rangeattack animation via pendingProjectile.
      - Enters a retreat phase after each shot using retreatTimer.
      - Tracks chase retention; clears hasSeenPlayer if the player is out of range too long.
      - Respects knockback guard and applies slow effect each frame.
    *****************************************************************************************/
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
        // KNOCKBACK GUARD
        if (ai->knockbackTimer > 0.0f)
        {
            ai->knockbackTimer -= ctx.dt;
            rb->velX = 0.0f;
            rb->velY = 0.0f;
            if (ai->knockbackTimer <= 0.0f)
                PlayAnim(enemy, "idle");
            return;
        }
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
