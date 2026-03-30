/*********************************************************************************************
 \file      EnemyActions.h
 \par       SofaSpuds
 \author    Choo Jian Wei (jianwei.c@digipen.edu) - Primary Author (80%)
            yimo.kong ( yimo.kong@digipen.edu) - Author, 20%
 \brief     Declares and defines game-specific AI action helpers for enemy behaviour execution.
 \details   Provides reusable helpers and action routines used by the sandbox enemy AI layer to
            drive movement, attacks, animation selection, and special-case boss behaviour.
            The file now covers generic melee/ranged enemies plus custom handling for HeiBang's
            dash-and-laser attack cycle and Nancie's facing/orientation fixes.

 \changelog
            Applied slowTimer/slowMultiplier from EnemyComponent to all
            movement velocity sets in Patrol, MeleeAttack, RangedAttack.
            Added HeiBang boss attack-point routing, attack2 beam follow-up support,
            and Nancie horizontal facing correction.
            Replaced ApplySlow (per-frame velocity multiply) with TickSlowTimer/GetSlowScale.
            TickSlowTimer is called once per function entry to tick the timer exactly once
            per frame. GetSlowScale is a pure read used at velocity assignment sites.

 \copyright
            All content 2025 DigiPen Institute of Technology Singapore.
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
    static constexpr float kHeiBangBeamMaxRange = 2.70f;
    static constexpr float kHeiBangBeamHitboxLengthScale = 1.8f;
    static constexpr float kHeiBangBeamTelegraphDuration = 0.08f;
    static constexpr float kHeiBangBeamPhaseDuration = 14.0f / 9.0f;
    static constexpr float kHeiBangBeamDamageDuration =
        kHeiBangBeamPhaseDuration - kHeiBangBeamTelegraphDuration;
    static constexpr float kHeiBangBeamGrowthDuration = 0.16f;
    static constexpr int kHeiBangAttack3BurstFrame = 11;
    static constexpr int kHeiBangAttack3ProjectileCount = 12;
    static constexpr float kHeiBangAttack3ProjectileSpeed = 0.52f;
    static constexpr float kHeiBangAttack3ProjectileLifetime = 2.4f;
    static constexpr float kHeiBangAttack3ProjectileWidth = 0.12f;
    static constexpr float kHeiBangAttack3ProjectileHeight = 0.12f;
    static constexpr float kHeiBangAttack3ProjectileSpawnRadius = 0.20f;
    static constexpr float kHeiBangPostAttackPauseDuration = 4.0f;

    /*****************************************************************************************
      \brief Spawns or updates HeiBang's single beam hitbox so it only grows in length.
      \details Keeps one rotated hitbox alive for the beam's active window and stretches it
               outward from the mouth as the visible laser extends.
    *****************************************************************************************/
    inline void SpawnHeiBangBeamDamageHitBox(Framework::GOC* enemy,
        Framework::EnemyAttackComponent* attack,
        const Framework::BehaviorContext& ctx)
    {
        if (!(enemy && attack && attack->hitbox && ctx.spawnHitBox))
            return;

        const float damageElapsed = attack->hitboxElapsed - kHeiBangBeamTelegraphDuration;
        if (damageElapsed <= 0.0f)
            return;

        const float growth =
            std::clamp(damageElapsed / std::max(kHeiBangBeamGrowthDuration, 0.0001f), 0.0f, 1.0f);
        if (growth <= 0.0f)
            return;

        const glm::vec2 beamStart{ attack->attack2BeamStartX, attack->attack2BeamStartY };
        const glm::vec2 fullBeamDelta{
            attack->attack2BeamTargetX - beamStart.x,
            attack->attack2BeamTargetY - beamStart.y
        };
        const glm::vec2 currentBeamDelta = fullBeamDelta * growth;
        const float beamThickness = std::max(attack->attack2BeamThickness, 0.08f);
        const float currentBeamLength = std::sqrt(
            currentBeamDelta.x * currentBeamDelta.x +
            currentBeamDelta.y * currentBeamDelta.y);
        if (currentBeamLength <= 0.0001f)
            return;

        const glm::vec2 beamDir = currentBeamDelta / currentBeamLength;
        const glm::vec2 beamCenter = beamStart + (beamDir * (currentBeamLength * 0.5f));
        const float beamRotation = std::atan2(beamDir.y, beamDir.x);
        const float remainingDuration =
            std::max(kHeiBangBeamPhaseDuration - attack->hitboxElapsed, std::max(ctx.dt, 0.016f));

        if (!attack->attack2BeamDamageSpawned || !attack->attack2BeamRuntimeHitbox)
        {
            attack->attack2BeamRuntimeHitbox = ctx.spawnHitBox(
                enemy,
                beamCenter.x,
                beamCenter.y,
                currentBeamLength,
                beamThickness,
                static_cast<float>(attack->damage),
                remainingDuration,
                0.0f,
                beamRotation,
                false);
            attack->attack2BeamDamageSpawned = (attack->attack2BeamRuntimeHitbox != nullptr);
            return;
        }

        attack->attack2BeamRuntimeHitbox->spawnX = beamCenter.x;
        attack->attack2BeamRuntimeHitbox->spawnY = beamCenter.y;
        attack->attack2BeamRuntimeHitbox->width = currentBeamLength;
        attack->attack2BeamRuntimeHitbox->height = beamThickness;
        attack->attack2BeamRuntimeHitbox->rotation = beamRotation;
        attack->attack2BeamRuntimeHitbox->duration = remainingDuration;
        attack->attack2BeamRuntimeHitbox->active = true;
    }

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

    /*****************************************************************************************
      \brief Performs a case-insensitive string comparison.
      \param a Left-hand string.
      \param b Right-hand string.
      \return True when both strings match ignoring ASCII case.
    *****************************************************************************************/
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

    /*****************************************************************************************
      \brief Checks whether the object owns an animation clip with the given name.
      \param goc  Game object composition to inspect.
      \param name Animation name to search for.
      \return True when the clip exists on the object's SpriteAnimationComponent.
    *****************************************************************************************/
    inline bool HasAnim(Framework::GOC* goc, std::string_view name)
    {
        if (!goc)
            return false;

        auto* anim = goc->GetComponentType<Framework::SpriteAnimationComponent>(
            Framework::ComponentTypeId::CT_SpriteAnimationComponent);
        return FindAnimationIndex(anim, name) >= 0;
    }

    /*****************************************************************************************
      \brief Retrieves the active animation name for an object, if any.
      \param goc Game object composition to inspect.
      \return A string_view into the active animation name, or an empty view when unavailable.
    *****************************************************************************************/
    inline std::string_view ActiveAnimName(Framework::GOC* goc)
    {
        if (!goc)
            return {};

        auto* anim = goc->GetComponentType<Framework::SpriteAnimationComponent>(
            Framework::ComponentTypeId::CT_SpriteAnimationComponent);
        const auto* active = anim ? anim->ActiveAnimation() : nullptr;
        return active ? std::string_view(active->name) : std::string_view{};
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
      \brief Ticks the slow timer down by dt. Call exactly once per enemy per frame,
             at the top of the AI function before any early returns.
      \param enemy Enemy game object that owns EnemyComponent.
      \param dt    Delta time in seconds.
    *****************************************************************************************/
    inline void TickSlowTimer(Framework::GOC* enemy, float dt)
    {
        auto* enemyComp = enemy->GetComponentType<Framework::EnemyComponent>(CT_EnemyComponent());
        if (!enemyComp) return;
        if (enemyComp->slowTimer > 0.0f)
        {
            enemyComp->slowTimer = std::max(0.0f, enemyComp->slowTimer - dt);
            if (enemyComp->slowTimer <= 0.0f)
                enemyComp->slowMultiplier = 1.0f;
        }
    }

    /*****************************************************************************************
      \brief Returns the current slow speed scale for this enemy.
             Pure read — no side effects. Use at velocity assignment sites.
      \param enemy Enemy game object that owns EnemyComponent.
      \return slowMultiplier while slow is active, 1.0f otherwise.
    *****************************************************************************************/
    inline float GetSlowScale(Framework::GOC* enemy)
    {
        auto* enemyComp = enemy->GetComponentType<Framework::EnemyComponent>(CT_EnemyComponent());
        if (!enemyComp || enemyComp->slowTimer <= 0.0f) return 1.0f;
        return enemyComp->slowMultiplier;
    }

    /*****************************************************************************************
      \brief Returns the wind-up duration for the standard timed melee attack.
      \param attackDuration Total attack animation duration in seconds.
      \return Delay before the melee damage window becomes active.
    *****************************************************************************************/
    inline float TimedMeleeHitboxDelay(float attackDuration)
    {
        return std::clamp(attackDuration * 0.4f, 0.12f, 0.32f);
    }

    /*****************************************************************************************
      \brief Returns the active damage window for the standard timed melee attack.
      \param attackDuration Total attack animation duration in seconds.
      \return Hitbox lifetime in seconds.
    *****************************************************************************************/
    inline float TimedMeleeHitboxDuration(float attackDuration)
    {
        return std::clamp(attackDuration * 0.25f, 0.08f, 0.18f);
    }

    /*****************************************************************************************
      \brief Rotates a sprite-flip style enemy to face its target on the X axis.
      \param enemy Enemy object whose render width should be mirrored.
      \param ai    Decision-tree state storing the resolved facing direction.
      \param dx    Horizontal delta from enemy to target.
      \details Used for enemies such as Nancie whose visual orientation is driven by
               the sign of RenderComponent::w rather than a dedicated rotation value.
    *****************************************************************************************/
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
      \brief Checks whether a proposed enemy position would overlap a wall object.
      \param enemy Enemy being moved.
      \param rb    Enemy rigid body used for AABB dimensions.
      \param nextX Proposed world X center.
      \param nextY Proposed world Y center.
      \return True if the future AABB would overlap any solid "rect" object.
    *****************************************************************************************/
    inline bool WouldCollideWithSolid(
        Framework::GOC* enemy,
        Framework::RigidBodyComponent* rb,
        float nextX,
        float nextY)
    {
        if (!(enemy && rb && Framework::FACTORY))
            return true;

        const auto& layers = Framework::FACTORY->Layers();
        const Framework::LayerKey enemyLayer = layers.LayerKeyFor(enemy->GetId());
        if (!layers.IsLayerEnabled(enemyLayer))
            return true;

        const Framework::AABB futureBox(nextX, nextY, rb->width, rb->height);
        for (auto& pair : Framework::FACTORY->Objects())
        {
            if (!pair.second)
                continue;

            GOC* goc = pair.second.get();
            if (!goc || goc == enemy)
                continue;

            const Framework::LayerKey otherLayer = layers.LayerKeyFor(goc->GetId());
            if (!layers.IsLayerEnabled(otherLayer) || !(otherLayer == enemyLayer))
                continue;

            auto* rbO = goc->GetComponentType<Framework::RigidBodyComponent>(
                Framework::ComponentTypeId::CT_RigidBodyComponent);
            auto* trO = goc->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            if (!rbO || !trO)
                continue;
            if (rbO->width <= 0.0f || rbO->height <= 0.0f)
                continue;

            const Framework::AABB solidBox(trO->x, trO->y, rbO->width, rbO->height);
            if (Framework::Collision::CheckCollisionRectToRect(futureBox, solidBox))
                return true;
        }

        return false;
    }

    /*****************************************************************************************
      \brief Checks whether a projectile-sized box would hit a world blocker at a position.
      \param enemy     Enemy requesting the query.
      \param target    Intended player target to ignore for blocker checks.
      \param nextX     Proposed projectile center X.
      \param nextY     Proposed projectile center Y.
      \param width     Projectile width.
      \param height    Projectile height.
      \return True if an environment collider would consume the projectile before it reaches target.
    *****************************************************************************************/
    inline bool WouldProjectileHitBlocker(
        Framework::GOC* enemy,
        Framework::GOC* target,
        float nextX,
        float nextY,
        float width,
        float height)
    {
        if (!(enemy && Framework::FACTORY))
            return true;

        const auto& layers = Framework::FACTORY->Layers();
        const Framework::LayerKey enemyLayer = layers.LayerKeyFor(enemy->GetId());
        if (!layers.IsLayerEnabled(enemyLayer))
            return true;

        const Framework::AABB futureBox(nextX, nextY, width, height);
        for (auto& pair : Framework::FACTORY->Objects())
        {
            if (!pair.second)
                continue;

            GOC* goc = pair.second.get();
            if (!goc || goc == enemy || goc == target)
                continue;

            const Framework::LayerKey otherLayer = layers.LayerKeyFor(goc->GetId());
            if (!layers.IsLayerEnabled(otherLayer) || !(otherLayer == enemyLayer))
                continue;

            auto* rbO = goc->GetComponentType<Framework::RigidBodyComponent>(
                Framework::ComponentTypeId::CT_RigidBodyComponent);
            auto* trO = goc->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            if (!rbO || !trO)
                continue;
            if (rbO->width <= 0.0f || rbO->height <= 0.0f)
                continue;

            const bool isPlayer = goc->GetComponentType<Framework::PlayerComponent>(
                Framework::ComponentTypeId::CT_PlayerComponent) != nullptr;
            const bool isEnemy = goc->GetComponentType<Framework::EnemyComponent>(
                Framework::ComponentTypeId::CT_EnemyComponent) != nullptr;
            if (isPlayer || isEnemy)
                continue;

            const Framework::AABB solidBox(trO->x, trO->y, rbO->width, rbO->height);
            if (Framework::Collision::CheckCollisionRectToRect(futureBox, solidBox))
                return true;
        }

        return false;
    }

    /*****************************************************************************************
      \brief Samples along the enemy's firing lane to see if a projectile would be blocked.
      \param enemy   Enemy attempting the shot.
      \param player  Intended player target.
      \param spawnX  Projectile start center X.
      \param spawnY  Projectile start center Y.
      \param dirX    Normalized fire direction X.
      \param dirY    Normalized fire direction Y.
      \param distanceToTarget Distance from enemy to player.
      \param width   Projectile width.
      \param height  Projectile height.
      \return True when the lane is clear enough for the projectile to reach the player.
    *****************************************************************************************/
    inline bool HasClearProjectileLane(
        Framework::GOC* enemy,
        Framework::GOC* player,
        float spawnX,
        float spawnY,
        float dirX,
        float dirY,
        float distanceToTarget,
        float width,
        float height)
    {
        const float laneDistance = std::max(0.0f, distanceToTarget);
        const float sampleSpacing = std::max(width, height) * 0.75f;
        const int sampleCount = std::max(1, static_cast<int>(std::ceil(
            laneDistance / std::max(sampleSpacing, 0.02f))));

        for (int i = 0; i <= sampleCount; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(sampleCount);
            const float sampleX = spawnX + (dirX * laneDistance * t);
            const float sampleY = spawnY + (dirY * laneDistance * t);
            if (WouldProjectileHitBlocker(enemy, player, sampleX, sampleY, width, height))
                return false;
        }

        return true;
    }

    /*****************************************************************************************
      \brief AI action: moves the enemy back and forth along a fixed horizontal patrol range.
      \param ctx BehaviorContext containing owner, dt, and blackboard.
      \details
      - Initialises patrolOriginX/Y on first call.
      - Reverses direction at patrol range edges or on wall collision.
      - Pauses briefly at each turn-around point.
      - Respects knockback guard. Slow applied via TickSlowTimer/GetSlowScale.
    *****************************************************************************************/
    inline void Patrol(Framework::BehaviorContext& ctx)
    {
        GOC* enemy = ctx.owner;
        if (!enemy) return;

        auto* rb = enemy->GetComponentType<Framework::RigidBodyComponent>(Framework::ComponentTypeId::CT_RigidBodyComponent);
        auto* tr = enemy->GetComponentType<Framework::TransformComponent>(Framework::ComponentTypeId::CT_TransformComponent);
        auto* ai = enemy->GetComponentType<Framework::EnemyDecisionTreeComponent>(Framework::ComponentTypeId::CT_EnemyDecisionTreeComponent);

        if (!rb || !tr || !ai) return;
        TickSlowTimer(enemy, ctx.dt);

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

        rb->velX = patrolSpeed * ai->dir * GetSlowScale(enemy);
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
    }

    /*****************************************************************************************
      \brief AI action: chases the player and delivers a melee hitbox strike.
      \param ctx BehaviorContext containing owner, dt, spawnHitBox callback, and blackboard.
      \details
      - Accelerates toward the player until within kMeleeAttackDist.
      - Spawns a hitbox when attack_timer exceeds attack_speed.
      - Holds position and waits for the hitbox duration to expire before re-enabling input.
      - Tracks chase retention; clears hasSeenPlayer if the player is out of range too long.
      - HeiBang overrides the generic chase with scripted dash points and an attack2 beam phase.
      - Nancie updates sprite facing to track the player before movement/attack decisions.
      - Respects knockback guard. Slow applied via TickSlowTimer/GetSlowScale.
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
        TickSlowTimer(enemy, ctx.dt);

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
        const bool useTimedMeleeAttack = !isHeiBang && !isNancie && HasAnim(enemy, "slashattack");
        const float chaseRetentionRadius = isHeiBang
            ? kHeiBangChaseRetentionRadius
            : kChaseRetentionRadius;

        static constexpr std::array<std::pair<float, float>, 3> kHeiBangAttackPoints{ {
            {0.704178f, -1.02655f},
            {1.21166f,  -2.18211f},
            {1.61999f,  -1.57658f}
        } };

        if (isNancie)
            FaceTargetHorizontally(enemy, ai, dx);

        if (useTimedMeleeAttack && ai->meleeAttackActive)
        {
            rb->velX = 0.0f;
            rb->velY = 0.0f;

            if (ai->pendingMeleeHitbox)
                FaceTargetHorizontally(enemy, ai, dx);

            ai->meleeAttackElapsed += ctx.dt;

            if (ai->pendingMeleeHitbox && ai->meleeAttackElapsed >= ai->meleeHitboxDelay)
            {
                ai->pendingMeleeHitbox = false;

                if (ctx.spawnHitBox)
                {
                    const float direction = (ai->facing == Framework::Facing::LEFT) ? -1.0f : 1.0f;
                    const float hbWidth = rb->width * 1.2f;
                    const float hbHeight = rb->height * 0.8f;
                    const float spawnX = tr->x + (direction * hbWidth * 0.25f);
                    const float spawnY = tr->y;

                    ctx.spawnHitBox(enemy, spawnX, spawnY, hbWidth, hbHeight,
                        static_cast<float>(attack->damage),
                        ai->meleeHitboxDuration, 0.0f, 0.0f, true);

                    if (audio)
                    {
                        GameAudio gameAudio(audio, GameAudio::Entity::Enemy);
                        gameAudio.PlayAttack(tr->x, tr->y);
                    }
                }
            }

            if (ai->meleeAttackElapsed >= ai->meleeAttackDuration)
            {
                ai->meleeAttackActive = false;
                ai->pendingMeleeHitbox = false;
                ai->meleeAttackElapsed = 0.0f;
                ai->meleeAttackDuration = 0.0f;
                ai->meleeHitboxDelay = 0.0f;
                ai->meleeHitboxDuration = 0.0f;
                PlayAnim(enemy, "idle");
            }
            return;
        }

        if (attack->hitbox->active)
        {
            rb->velX = 0.0f;
            rb->velY = 0.0f;
            attack->hitboxElapsed += ctx.dt;

            if (isHeiBang && ai->currentPathIndex == 2 && !attack->attack3VolleySpawned &&
                EqualsIgnoreCase(ActiveAnimName(enemy), "attack3"))
            {
                auto* anim = enemy->GetComponentType<Framework::SpriteAnimationComponent>(
                    Framework::ComponentTypeId::CT_SpriteAnimationComponent);
                const auto* activeAnimData = anim ? anim->ActiveAnimation() : nullptr;
                const int currentFrame = activeAnimData ? activeAnimData->currentFrame : -1;
                if (currentFrame >= kHeiBangAttack3BurstFrame && ctx.spawnProjectile)
                {
                    constexpr float kStartAngle = 0.78539816339f; // 45 degrees
                    constexpr float kAngleStep = 6.28318530718f /
                        static_cast<float>(kHeiBangAttack3ProjectileCount);
                    for (int i = 0; i < kHeiBangAttack3ProjectileCount; ++i)
                    {
                        const float angle = kStartAngle + static_cast<float>(i) * kAngleStep;
                        const float dirX = std::cos(angle);
                        const float dirY = std::sin(angle);
                        const float spawnX = tr->x + dirX * kHeiBangAttack3ProjectileSpawnRadius;
                        const float spawnY = tr->y + dirY * kHeiBangAttack3ProjectileSpawnRadius;
                        ctx.spawnProjectile(enemy,
                            spawnX, spawnY,
                            dirX, dirY,
                            kHeiBangAttack3ProjectileSpeed,
                            kHeiBangAttack3ProjectileWidth, kHeiBangAttack3ProjectileHeight,
                            static_cast<float>(attack->damage),
                            kHeiBangAttack3ProjectileLifetime);
                    }
                    attack->attack3VolleySpawned = true;
                }
            }

            if (isHeiBang && attack->attack2BeamPhaseActive &&
                attack->hitboxElapsed >= kHeiBangBeamTelegraphDuration)
            {
                SpawnHeiBangBeamDamageHitBox(enemy, attack, ctx);
            }

            if (attack->hitboxElapsed >= attack->hitbox->duration)
            {
                const std::string_view activeAnim = ActiveAnimName(enemy);
                if (isHeiBang && ai->currentPathIndex == 1 &&
                    !attack->attack2BeamPhaseActive &&
                    EqualsIgnoreCase(activeAnim, "attack2"))
                {
                    attack->attack2BeamPhaseActive = true;
                    attack->attack2BeamDamageSpawned = false;
                    attack->hitboxElapsed = 0.0f;
                    attack->hitbox->duration = kHeiBangBeamPhaseDuration;

                    auto* enemyRender = enemy->GetComponentType<Framework::RenderComponent>(
                        Framework::ComponentTypeId::CT_RenderComponent);
                    const float ownerWidth = enemyRender
                        ? std::fabs(enemyRender->w * tr->scaleX)
                        : 0.3f;
                    const float ownerHeight = enemyRender
                        ? std::fabs(enemyRender->h * tr->scaleY)
                        : 0.3f;
                    const float facingSign = (enemyRender && enemyRender->w < 0.0f) ? -1.0f : 1.0f;
                    const glm::vec2 beamStart{
                        tr->x + (facingSign * ownerWidth * 0.15f),
                        tr->y + (ownerHeight * 0.14f)
                    };

                    glm::vec2 beamDelta{ trPlayer->x - beamStart.x, trPlayer->y - beamStart.y };
                    float beamDistance = std::sqrt(beamDelta.x * beamDelta.x + beamDelta.y * beamDelta.y);
                    if (beamDistance < 0.0001f)
                    {
                        beamDelta = { facingSign, 0.0f };
                        beamDistance = 1.0f;
                    }

                    if (beamDistance > kHeiBangBeamMaxRange)
                        beamDelta *= (kHeiBangBeamMaxRange / beamDistance);

                    const glm::vec2 beamTarget{ beamStart.x + beamDelta.x, beamStart.y + beamDelta.y };
                    const glm::vec2 beamHitTarget{
                        beamStart.x + (beamDelta.x * kHeiBangBeamHitboxLengthScale),
                        beamStart.y + (beamDelta.y * kHeiBangBeamHitboxLengthScale)
                    };
                    const glm::vec2 beamCenter{
                        (beamStart.x + beamTarget.x) * 0.5f,
                        (beamStart.y + beamTarget.y) * 0.5f
                    };
                    // The beam sprite sheet has large transparent padding, so keep the gameplay
                    // thickness tied to the visible orange core instead of the full square frame.
                    const float beamThickness = std::max(0.08f, ownerHeight * 0.28f);
                    attack->attack2BeamStartX = beamStart.x;
                    attack->attack2BeamStartY = beamStart.y;
                    attack->attack2BeamTargetX = beamHitTarget.x;
                    attack->attack2BeamTargetY = beamHitTarget.y;
                    attack->attack2BeamThickness = beamThickness;
                    attack->hitbox->spawnX = beamCenter.x;
                    attack->hitbox->spawnY = beamCenter.y;
                    attack->hitbox->width = std::fabs(beamTarget.x - beamStart.x) + beamThickness;
                    attack->hitbox->height = std::fabs(beamTarget.y - beamStart.y) + beamThickness;
                    mygame::SpawnHeiBangAttack2BeamVfx(*enemy, beamTarget);
                    return;
                }

                attack->hitbox->active = false;
                attack->hitboxElapsed = 0.0f;
                attack->attack2BeamPhaseActive = false;
                attack->attack2BeamDamageSpawned = false;
                attack->attack2BeamStartX = 0.0f;
                attack->attack2BeamStartY = 0.0f;
                attack->attack2BeamTargetX = 0.0f;
                attack->attack2BeamTargetY = 0.0f;
                attack->attack2BeamThickness = 0.0f;
                attack->attack2BeamRuntimeHitbox = nullptr;
                attack->attack3VolleySpawned = false;
                PlayAnim(enemy, "idle");
                if (isHeiBang)
                {
                    const std::size_t nextPathIndex =
                        (ai->currentPathIndex + 1) % kHeiBangAttackPoints.size();
                    ai->currentPathIndex = nextPathIndex;
                    if (nextPathIndex == 0)
                        ai->pauseTimer = kHeiBangPostAttackPauseDuration;
                }
            }
            return;
        }

        // --- HeiBang scripted dash-to-point behaviour ---
        if (isHeiBang)
        {
            if (ai->pauseTimer > 0.0f)
            {
                ai->pauseTimer = std::max(0.0f, ai->pauseTimer - ctx.dt);
                rb->velX = 0.0f;
                rb->velY = 0.0f;
                PlayAnim(enemy, "idle");
                return;
            }

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
                rb->velX = (pDx / pointNorm) * dashSpeed * GetSlowScale(enemy);
                rb->velY = (pDy / pointNorm) * dashSpeed * GetSlowScale(enemy);
                PlayAnim(enemy, "dash");
                return;
            }

            rb->velX = 0.0f;
            rb->velY = 0.0f;
            ai->facing = (dx < 0.0f) ? Framework::Facing::LEFT : Framework::Facing::RIGHT;
            PlayAnim(enemy, "idle");
            attack->attack_timer += ctx.dt;
            if (attack->attack_timer >= attack->attack_speed)
            {
                attack->attack_timer = 0.0f;
                if (ctx.spawnHitBox)
                {
                    attack->hitbox->active = true;
                    attack->hitboxElapsed = 0.0f;
                    attack->attack2BeamPhaseActive = false;
                    attack->attack2BeamDamageSpawned = false;
                    attack->attack2BeamRuntimeHitbox = nullptr;
                    attack->attack3VolleySpawned = false;
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
                    if (!(isHeiBang && ai->currentPathIndex == 1))
                    {
                        ctx.spawnHitBox(enemy, spawnX, spawnY, hbWidth, hbHeight,
                            static_cast<float>(attack->damage),
                            attack->hitbox->duration, 0.0f, 0.0f, true);
                    }

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

        // --- Generic melee chase ---
        float speed = 1.0f;
        float accel = 2.0f;
        float stopDist = 0.1f;

        // Make Nancie a bit faster
        if (isNancie)
        {
            speed *= 1.5f; // 50% faster
            accel *= 1.2f; // slightly snappier acceleration
        }

        if (distance > stopDist)
        {
            float norm = (distance > 0.001f) ? distance : 1.0f;
            float targetVX = (dx / norm) * speed * GetSlowScale(enemy);
            float targetVY = (dy / norm) * speed * GetSlowScale(enemy);
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

        if (!useTimedMeleeAttack)
            ai->facing = (dx < 0.0f) ? Framework::Facing::LEFT : Framework::Facing::RIGHT;

        attack->attack_timer += ctx.dt;
        if (attack->attack_timer >= attack->attack_speed &&
            !attack->hitbox->active &&
            !ai->meleeAttackActive &&
            distance < kMeleeAttackDist)
        {
            attack->attack_timer = 0.0f;

            if (useTimedMeleeAttack)
            {
                FaceTargetHorizontally(enemy, ai, dx);
                rb->velX = 0.0f;
                rb->velY = 0.0f;

                ai->meleeAttackActive = true;
                ai->pendingMeleeHitbox = true;
                ai->meleeAttackElapsed = 0.0f;
                ai->meleeAttackDuration = std::max(GetAnimDuration(enemy, "slashattack"), 0.2f);
                ai->meleeHitboxDelay = TimedMeleeHitboxDelay(ai->meleeAttackDuration);
                ai->meleeHitboxDuration = TimedMeleeHitboxDuration(ai->meleeAttackDuration);

                PlayAnim(enemy, "slashattack");
            }
            else if (ctx.spawnHitBox)
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
                    attack->hitbox->duration, 0.0f, 0.0f, true);

                if (audio)
                {
                    GameAudio gameAudio(audio, GameAudio::Entity::Enemy);
                    gameAudio.PlayAttack(tr->x, tr->y);
                }
                PlayAnim(enemy, meleeAnim);
            }
        }

        if (distance > chaseRetentionRadius)
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

    /*****************************************************************************************
      \brief AI action: maintains distance from the player and fires projectiles.
      \param ctx BehaviorContext containing owner, dt, spawnProjectile callback, and blackboard.
      \details
      - Retreats when too close (< minDist) and advances when too far (> maxDist).
      - Fires a projectile after attack_speed elapses if within kRangedAttackFireDist.
      - Projectile spawn is deferred to the end of the rangeattack animation via pendingProjectile.
      - Enters a retreat phase after each shot using retreatTimer.
      - Tracks chase retention; clears hasSeenPlayer if the player is out of range too long.
      - Respects knockback guard. Slow applied via TickSlowTimer/GetSlowScale.
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
        TickSlowTimer(enemy, ctx.dt);

        if (!ai->patrolOriginSet)
        {
            ai->patrolOriginX = tr->x;
            ai->patrolOriginY = tr->y;
            ai->patrolOriginSet = true;
            ai->prevX = tr->x;
            ai->prevY = tr->y;
            if (ai->dir == 0.0f)
                ai->dir = 1.0f;
        }

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

        ai->rangedMovePauseTimer = std::max(0.0f, ai->rangedMovePauseTimer - ctx.dt);
        ai->rangedDirectionLockTimer = std::max(0.0f, ai->rangedDirectionLockTimer - ctx.dt);
        ai->rangedRepositionTimer = std::max(0.0f, ai->rangedRepositionTimer - ctx.dt);

        if (ai->rangedAttackActive)
        {
            rb->velX = 0.0f;
            rb->velY = 0.0f;
            FaceTargetHorizontally(enemy, ai, ai->pendingProjectileDirX);
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
                ai->rangedMovePauseTimer = 0.18f;
                PlayAnim(enemy, "idle");
            }
            return;
        }

        float norm = distance > 0.001f ? distance : 1.0f;
        float dirX = dx / norm;
        float dirY = dy / norm;

        constexpr float speed = 0.65f;
        constexpr float retreatSpeed = 0.22f;
        constexpr float strafeSpeed = 0.30f;
        constexpr float minDist = 0.5f;
        constexpr float maxDist = 1.2f;
        constexpr float retreatDuration = 0.65f;
        constexpr float diagonalMix = 0.55f;
        constexpr float stuckMoveThreshold = 0.0025f;
        constexpr float directionLockDuration = 0.35f;
        constexpr float projectileWidth = 0.15f;
        constexpr float projectileHeight = 0.08f;
        constexpr float boxedInPauseDuration = 0.12f;
        constexpr float boxedInRepositionDuration = 0.55f;

        float& retreatTimer = ai->retreatTimer;
        const float slowScale = GetSlowScale(enemy);
        FaceTargetHorizontally(enemy, ai, dx);

        const float projectileSpawnOffset = std::max(rb->width, rb->height);
        const float projectileSpawnX = tr->x + dirX * projectileSpawnOffset;
        const float projectileSpawnY = tr->y + dirY * projectileSpawnOffset;
        const bool projectileLaneBlocked =
            distance < kRangedAttackFireDist &&
            !HasClearProjectileLane(enemy, player,
                projectileSpawnX, projectileSpawnY,
                dirX, dirY, distance,
                projectileWidth, projectileHeight);

        const bool wantsRetreat = retreatTimer > 0.0f || distance < minDist;
        const bool wantsApproach = !wantsRetreat &&
            (distance > maxDist ||
                projectileLaneBlocked ||
                ai->rangedRepositionTimer > 0.0f);
        const bool wantsStrafe = !wantsRetreat && !wantsApproach;

        const float movedX = tr->x - ai->prevX;
        const float movedY = tr->y - ai->prevY;
        const float movedDistance = std::sqrt(movedX * movedX + movedY * movedY);
        if (movedDistance < stuckMoveThreshold)
        {
            ai->stuckXTimer += ctx.dt;
            ai->stuckYTimer += ctx.dt;
        }
        else
        {
            ai->stuckXTimer = 0.0f;
            ai->stuckYTimer = 0.0f;
        }

        if (wantsStrafe &&
            ai->rangedDirectionLockTimer <= 0.0f &&
            std::max(ai->stuckXTimer, ai->stuckYTimer) >= ai->stuckThreshold)
        {
            ai->dir *= -1.0f;
            ai->rangedDirectionLockTimer = directionLockDuration;
            ai->stuckXTimer = 0.0f;
            ai->stuckYTimer = 0.0f;
        }

        auto tryDirectionalMove = [&](float rawDirX, float rawDirY, float moveSpeed)
            {
                const float rawLen = std::sqrt(rawDirX * rawDirX + rawDirY * rawDirY);
                if (rawLen <= 0.0001f)
                    return false;

                const float velX = (rawDirX / rawLen) * moveSpeed * slowScale;
                const float velY = (rawDirY / rawLen) * moveSpeed * slowScale;
                const float nextX = tr->x + velX * ctx.dt;
                const float nextY = tr->y + velY * ctx.dt;
                if (WouldCollideWithSolid(enemy, rb, nextX, nextY))
                    return false;

                rb->velX = velX;
                rb->velY = velY;
                return true;
            };

        if (retreatTimer > 0.0f)
            retreatTimer = std::max(0.0f, retreatTimer - ctx.dt);

        if (ai->rangedMovePauseTimer > 0.0f)
        {
            rb->velX = 0.0f;
            rb->velY = 0.0f;
        }
        else
        {
            const float strafeDirX = -dirY * ai->dir;
            const float strafeDirY = dirX * ai->dir;

            auto tryMovementPlan = [&](float activeStrafeDirX, float activeStrafeDirY)
                {
                    if (wantsRetreat)
                    {
                        return
                            tryDirectionalMove(-dirX + activeStrafeDirX * diagonalMix,
                                -dirY + activeStrafeDirY * diagonalMix, retreatSpeed) ||
                            tryDirectionalMove(-dirX, -dirY, retreatSpeed) ||
                            tryDirectionalMove(activeStrafeDirX, activeStrafeDirY, strafeSpeed);
                    }

                    if (wantsApproach)
                    {
                        return
                            tryDirectionalMove(dirX + activeStrafeDirX * diagonalMix,
                                dirY + activeStrafeDirY * diagonalMix, speed) ||
                            tryDirectionalMove(dirX, dirY, speed) ||
                            tryDirectionalMove(activeStrafeDirX, activeStrafeDirY, strafeSpeed);
                    }

                    return tryDirectionalMove(activeStrafeDirX, activeStrafeDirY, strafeSpeed);
                };

            bool moved = tryMovementPlan(strafeDirX, strafeDirY);
            if (!moved && ai->rangedDirectionLockTimer <= 0.0f)
            {
                const float originalDir = ai->dir;
                ai->dir = -originalDir;
                ai->rangedDirectionLockTimer = directionLockDuration;
                ai->stuckXTimer = 0.0f;
                ai->stuckYTimer = 0.0f;
                moved = tryMovementPlan(-strafeDirX, -strafeDirY);
                if (!moved)
                    ai->dir = originalDir;
            }

            if (!moved)
            {
                rb->velX = 0.0f;
                rb->velY = 0.0f;
                if (wantsStrafe)
                {
                    ai->rangedMovePauseTimer = std::max(ai->rangedMovePauseTimer, boxedInPauseDuration);
                    ai->rangedRepositionTimer = std::max(ai->rangedRepositionTimer, boxedInRepositionDuration);
                    ai->rangedDirectionLockTimer = std::max(ai->rangedDirectionLockTimer, directionLockDuration);
                    ai->stuckXTimer = 0.0f;
                    ai->stuckYTimer = 0.0f;
                }
            }
        }

        attack->attack_timer += ctx.dt;

        if (attack->attack_timer >= attack->attack_speed &&
            retreatTimer <= 0.0f &&
            distance < kRangedAttackFireDist &&
            !projectileLaneBlocked)
        {
            attack->attack_timer = 0.0f;

            ai->pendingProjectileDirX = dirX;
            ai->pendingProjectileDirY = dirY;
            ai->pendingProjectileSpawnX = projectileSpawnX;
            ai->pendingProjectileSpawnY = projectileSpawnY;
            ai->pendingProjectile = true;
            FaceTargetHorizontally(enemy, ai, ai->pendingProjectileDirX);

            if (audio)
            {
                GameAudio gameAudio(audio, GameAudio::Entity::Enemy);
                gameAudio.PlayAttack(tr->x, tr->y);
            }
            PlayAnim(enemy, "rangeattack");
            ai->rangedAttackActive = true;
            ai->rangedAttackTimer = 0.0f;
            ai->rangedAttackDuration = GetAnimDuration(enemy, "rangeattack");
            rb->velX = 0.0f;
            rb->velY = 0.0f;
            ai->rangedMovePauseTimer = 0.0f;
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

        ai->prevX = tr->x;
        ai->prevY = tr->y;
    }
}
