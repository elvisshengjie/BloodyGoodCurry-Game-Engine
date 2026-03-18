/*********************************************************************************************
 \file      HitBoxSystem.cpp
 \par       SofaSpuds
 \author    Ho Jun (h.jun@digipen.edu) - Author, adapted for game-side build ownership
 \brief     Game-side implementation of Framework::HitBoxSystem.
 \details   The public HitBoxSystem interface remains in Engine/, but BloodyGoodCurry now
            owns the concrete implementation so game combat behavior is not compiled into
            the engine target.
*********************************************************************************************/

#include "Systems/HitBoxSystem.h"
#include "Components/EnemyComponent.h"
#include "Components/EnemyDecisionTreeComponent.h"
#include "Components/EnemyHealthComponent.h"
#include "Components/EnemyTypeComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/PlayerHealthComponent.h"
#include "Composition/Component.h"
#include "Systems/LogicSystem.h"
#include "Component/HitBoxComponent.h"
#include "Component/SpriteAnimationComponent.h"
#include "Component/TransformComponent.h"
#include "Factory/Factory.h"
#include "Physics/Collision/Collision.h"
#include "Physics/Dynamics/RigidBodyComponent.h"

#include <iostream>
#include <cctype>
#include <string_view>
#include <cmath>
#include <glm/vec2.hpp>
#include "Common/CRTDebug.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif

namespace Framework
{
    static constexpr float kMeleeVsPhysicalBonus = 2.0f;
    static constexpr float kRangedVsRangedBonus = 2.0f;
    static constexpr float kNeutralMultiplier = 1.00f;
    static constexpr float kEnemyAggroScale = 1.5f;

    namespace
    {
        void EmitCombatAudio(const CombatAudioCallback& callback, GOC* source, CombatAudioEvent event)
        {
            if (callback && source)
                callback(source, event);
        }

        void EmitHitImpactVfx(const HitImpactVfxCallback& callback, const glm::vec2& worldPos)
        {
            if (callback)
                callback(worldPos);
        }

        int FindAnimationIndex(SpriteAnimationComponent* anim, std::string_view desired)
        {
            if (!anim)
                return -1;

            auto equalsIgnoreCase = [](std::string_view a, std::string_view b)
                {
                    if (a.size() != b.size())
                        return false;

                    for (std::size_t i = 0; i < a.size(); ++i)
                    {
                        if (std::tolower(static_cast<unsigned char>(a[i])) !=
                            std::tolower(static_cast<unsigned char>(b[i])))
                            return false;
                    }
                    return true;
                };

            for (std::size_t i = 0; i < anim->animations.size(); ++i)
            {
                if (equalsIgnoreCase(anim->animations[i].name, desired))
                    return static_cast<int>(i);
            }

            return -1;
        }

        void PlayAnimationIfAvailable(GOC* goc, std::string_view name)
        {
            if (!goc)
                return;

            auto* anim =
                goc->GetComponentType<SpriteAnimationComponent>(ComponentTypeId::CT_SpriteAnimationComponent);
            if (!anim)
                return;

            const int idx = FindAnimationIndex(anim, name);
            if (idx >= 0 && idx != anim->ActiveAnimationIndex())
            {
                anim->SetActiveAnimation(idx);
            }
        }

        float ComputeEnemyDamage(float baseDamage,
            HitBoxComponent::Team attackTeam,
            EnemyTypeComponent::EnemyType enemyType)
        {
            using Team = HitBoxComponent::Team;
            using EType = EnemyTypeComponent::EnemyType;

            if (enemyType == EType::neutral) return baseDamage;

            if (attackTeam == Team::Player)
            {
                if (enemyType == EType::physical) return baseDamage * kMeleeVsPhysicalBonus;
                if (enemyType == EType::ranged)   return baseDamage * kNeutralMultiplier;
            }

            if (attackTeam == Team::Thrown)
            {
                if (enemyType == EType::ranged)   return baseDamage * kRangedVsRangedBonus;
                if (enemyType == EType::physical) return baseDamage * kNeutralMultiplier;
            }

            return baseDamage;
        }
    }

    HitBoxSystem::HitBoxSystem(LogicSystem& logicRef)
        : logic(logicRef)
    {
    }

    HitBoxSystem::~HitBoxSystem()
    {
        Shutdown();
    }

    void HitBoxSystem::Initialize()
    {
        activeHitBoxes.clear();
    }

    void HitBoxSystem::Shutdown()
    {
        activeHitBoxes.clear();
    }

    void HitBoxSystem::SpawnHitBox(GameObjectComposition* attacker,
        float targetX, float targetY,
        float width, float height,
        float damage,
        float duration,
        HitBoxComponent::Team team, float soundDelay)
    {
        if (!attacker)
            return;

        auto newhitbox = std::make_unique<HitBoxComponent>();
        newhitbox->spawnX = targetX;
        newhitbox->spawnY = targetY;
        newhitbox->width = width;
        newhitbox->height = height;
        newhitbox->damage = damage;
        newhitbox->duration = duration;
        newhitbox->owner = attacker;
        newhitbox->team = team;
        newhitbox->soundDelay = soundDelay;

        if (attacker->GetComponentType<PlayerComponent>(ComponentTypeId::CT_PlayerComponent))
            newhitbox->team = HitBoxComponent::Team::Player;
        else if (attacker->GetComponentType<EnemyComponent>(ComponentTypeId::CT_EnemyComponent))
            newhitbox->team = HitBoxComponent::Team::Enemy;
        else
            newhitbox->team = HitBoxComponent::Team::Neutral;

        newhitbox->ActivateHurtBox();

        std::string teamStr;
        switch (newhitbox->team)
        {
        case HitBoxComponent::Team::Player:     teamStr = "Player";  break;
        case HitBoxComponent::Team::Enemy:      teamStr = "Enemy";   break;
        case HitBoxComponent::Team::Thrown:     teamStr = "Thrown";  break;
        case HitBoxComponent::Team::PlayerSlow: teamStr = "Slow";    break;
        case HitBoxComponent::Team::Neutral:    teamStr = "Neutral"; break;
        }

        std::cout << "HitBox spawned at (" << targetX << ", " << targetY
            << ") with team: " << teamStr << "\n";

        ActiveHitBox active;
        active.hitbox = std::move(newhitbox);
        active.ownerId = attacker->GetId();
        active.timer = duration;

        activeHitBoxes.push_back(std::move(active));
    }

    void HitBoxSystem::SpawnProjectile(GameObjectComposition* attacker,
        float targetX, float targetY,
        float dirX, float dirY,
        float speed,
        float width, float height,
        float damage,
        float duration,
        HitBoxComponent::Team team)
    {
        if (!attacker)
            return;

        float len = std::sqrt(dirX * dirX + dirY * dirY);
        if (len < 0.0001f)
            return;

        dirX /= len;
        dirY /= len;

        auto newhitbox = std::make_unique<HitBoxComponent>();
        newhitbox->spawnX = targetX;
        newhitbox->spawnY = targetY;
        newhitbox->width = width;
        newhitbox->height = height;
        newhitbox->damage = damage;
        newhitbox->duration = duration;
        newhitbox->owner = attacker;
        newhitbox->team = team;
        if (team == HitBoxComponent::Team::Neutral)
        {
            if (attacker->GetComponentType<PlayerComponent>(ComponentTypeId::CT_PlayerComponent))
                newhitbox->team = HitBoxComponent::Team::Thrown;
            else if (attacker->GetComponentType<EnemyComponent>(ComponentTypeId::CT_EnemyComponent))
                newhitbox->team = HitBoxComponent::Team::Enemy;
        }

        newhitbox->ActivateHurtBox();

        std::string teamStr;
        switch (newhitbox->team)
        {
        case HitBoxComponent::Team::Player:     teamStr = "Player";  break;
        case HitBoxComponent::Team::Enemy:      teamStr = "Enemy";   break;
        case HitBoxComponent::Team::Thrown:     teamStr = "Thrown";  break;
        case HitBoxComponent::Team::PlayerSlow: teamStr = "Slow";    break;
        case HitBoxComponent::Team::Neutral:    teamStr = "Neutral"; break;
        }

        std::cout << "HitBox spawned at (" << targetX << ", " << targetY
            << ") with team: " << teamStr << "\n";

        ActiveHitBox projectile;
        projectile.hitbox = std::move(newhitbox);
        projectile.ownerId = attacker->GetId();
        projectile.timer = duration;
        projectile.hitGraceTimer = 1.0f;
        projectile.velX = dirX * speed;
        projectile.velY = dirY * speed;
        projectile.isProjectile = true;

        activeHitBoxes.push_back(std::move(projectile));
    }

    void HitBoxSystem::Update(float dt)
    {
        if (!FACTORY)
            return;

        auto& layers = FACTORY->Layers();

        for (auto it = activeHitBoxes.begin(); it != activeHitBoxes.end();)
        {
            it->timer -= dt;
            auto* HB = it->hitbox.get();
            auto* attacker = FACTORY->GetObjectWithId(it->ownerId);

            if (!attacker || !HB || !HB->active)
            {
                it = activeHitBoxes.erase(it);
                continue;
            }
            if (!layers.IsLayerEnabled(attacker->GetLayerName()))
            {
                it = activeHitBoxes.erase(it);
                continue;
            }

            if (it->isProjectile || HB->team == HitBoxComponent::Team::Thrown)
            {
                it->hitbox->spawnX += it->velX * dt;
                it->hitbox->spawnY += it->velY * dt;
            }

            AABB hitboxAABB(HB->spawnX, HB->spawnY, HB->width, HB->height);

            bool hitAnything = false;
            bool hitEnemy = false;
            bool ineffectiveHit = false;

            if (it->isProjectile && it->hitGraceTimer > 0.0f)
                it->hitGraceTimer = std::max(0.0f, it->hitGraceTimer - dt);

            for (auto* obj : logic.LevelObjects())
            {
                if (!obj || obj == attacker)
                    continue;
                if (!layers.IsLayerEnabled(obj->GetLayerName()))
                    continue;

                auto* tr = obj->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);
                auto* rb = obj->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent);
                if (!(tr && rb))
                    continue;

                if (it->isProjectile && it->hitGraceTimer > 0.0f)
                {
                    const bool isPlayer = obj->GetComponentType<PlayerComponent>(
                        ComponentTypeId::CT_PlayerComponent) != nullptr;
                    const bool isEnemy = obj->GetComponentType<EnemyComponent>(
                        ComponentTypeId::CT_EnemyComponent) != nullptr;
                    if (!isPlayer && !isEnemy)
                        continue;
                }

                const bool targetIsEnemy = obj->GetComponentType<EnemyComponent>(
                    ComponentTypeId::CT_EnemyComponent) != nullptr;

                AABB targetAABB = targetIsEnemy
                    ? AABB(tr->x, tr->y, rb->width * kEnemyAggroScale, rb->height * kEnemyAggroScale)
                    : AABB(tr->x, tr->y, rb->width, rb->height);

                if (!Collision::CheckCollisionRectToRect(hitboxAABB, targetAABB))
                    continue;

                bool targetIsPlayer = obj->GetComponentType<PlayerComponent>(
                    ComponentTypeId::CT_PlayerComponent) != nullptr;
                bool sameTeam = (HB->team == HitBoxComponent::Team::Player && targetIsPlayer) ||
                    (HB->team == HitBoxComponent::Team::Enemy && targetIsEnemy);

                if (sameTeam)
                    continue;

                bool validTargetHit = false;

                if (auto* playerHealth = obj->GetComponentType<PlayerHealthComponent>(
                    ComponentTypeId::CT_PlayerHealthComponent))
                {
                    if (!playerHealth->isInvulnerable)
                    {
                        playerHealth->TakeDamage(static_cast<int>(HB->damage));
                        validTargetHit = true;

                        if (!playerHealth->isDead)
                        {
                            EmitCombatAudio(combatAudioCallback, obj, CombatAudioEvent::PlayerHurt);
                        }
                        else if (!playerHealth->deathSoundPlayed)
                        {
                            EmitCombatAudio(combatAudioCallback, obj, CombatAudioEvent::PlayerDeath);
                            playerHealth->deathSoundPlayed = true;
                        }
                    }
                }
                else if (auto* enemyHealth = obj->GetComponentType<EnemyHealthComponent>(
                    ComponentTypeId::CT_EnemyHealthComponent))
                {
                    if (enemyHealth->enemyHealth <= 0)
                    {
                        // already dead, skip
                    }
                    else
                    {
                        float finalDamage = HB->damage;
                        if (auto* typeComp = obj->GetComponentType<EnemyTypeComponent>(
                            ComponentTypeId::CT_EnemyTypeComponent))
                        {
                            finalDamage = ComputeEnemyDamage(HB->damage, HB->team, typeComp->Etype);
                        }

                        enemyHealth->TakeDamage(static_cast<int>(finalDamage));
                        validTargetHit = true;
                        hitEnemy = true;

                        if (HB->team == HitBoxComponent::Team::PlayerSlow)
                        {
                            if (auto* enemyComp = obj->GetComponentType<EnemyComponent>(
                                ComponentTypeId::CT_EnemyComponent))
                            {
                                enemyComp->slowTimer = 1.5f;  // slow lasts 1.5 seconds
                                enemyComp->slowMultiplier = 0.6f;  // enemy moves at 60% speed
                            }
                        }

                        EmitHitImpactVfx(hitImpactVfxCallback, glm::vec2(tr->x, tr->y));
                        EmitCombatAudio(combatAudioCallback, obj, CombatAudioEvent::EnemyHurt);
                    }
                }
                else
                {
                    validTargetHit = true;
                }

                if (validTargetHit)
                {
                    const bool isPlayer = obj->GetComponentType<PlayerComponent>(
                        ComponentTypeId::CT_PlayerComponent) != nullptr;
                    const bool isEnemy = obj->GetComponentType<EnemyComponent>(
                        ComponentTypeId::CT_EnemyComponent) != nullptr;

                    if (isPlayer || isEnemy)
                    {
                        // Resolve boss name to skip knockback for HeiBang and Nancie
                        std::string objName = obj->GetObjectName();
                        std::transform(objName.begin(), objName.end(), objName.begin(),
                            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        const bool isBoss = (objName == "heibang" || objName == "nancie");

                        if (!isBoss)
                        {
                            auto* attackerTr = attacker->GetComponentType<TransformComponent>(
                                ComponentTypeId::CT_TransformComponent);
                            if (attackerTr)
                            {
                                float dx = tr->x - attackerTr->x;
                                float dy = tr->y - attackerTr->y;
                                float len = std::sqrt(dx * dx + dy * dy);
                                if (len > 0.001f)
                                {
                                    dx /= len;
                                    dy /= len;
                                }

                                const float knockStrength = 1.5f;
                                rb->knockVelX = dx * knockStrength;
                                rb->knockVelY = dy * knockStrength * 0.4f;
                                rb->knockbackTime = 0.25f;
                            }

                            if (auto* anim = obj->GetComponentType<SpriteAnimationComponent>(
                                ComponentTypeId::CT_SpriteAnimationComponent))
                            {
                                const int idx = FindAnimationIndex(anim, "knockback");
                                if (idx >= 0)
                                    anim->SetActiveAnimation(idx);
                            }
                        }

                        if (isEnemy)
                        {
                            if (auto* dtComp = obj->GetComponentType<EnemyDecisionTreeComponent>(
                                ComponentTypeId::CT_EnemyDecisionTreeComponent))
                            {
                                if (!isBoss)
                                    dtComp->knockbackTimer = 0.5f;
                            }
                        }
                    }
                }

                if (validTargetHit)
                    hitAnything = true;
                break;
            }

            if (!HB->soundTriggered && HB->team == HitBoxComponent::Team::Player)
            {
                HB->soundDelay -= dt;
                if (HB->soundDelay <= 0.0f)
                {
                    if (hitEnemy)
                        EmitCombatAudio(combatAudioCallback, attacker, CombatAudioEvent::PlayerAttackHit);
                    if (ineffectiveHit)
                        EmitCombatAudio(combatAudioCallback, attacker, CombatAudioEvent::PlayerAttackBlocked);
                    if (!hitAnything)
                        EmitCombatAudio(combatAudioCallback, attacker, CombatAudioEvent::PlayerAttackMiss);
                }
                HB->soundTriggered = true;
            }

            if (hitAnything || it->timer <= 0.0f)
                it = activeHitBoxes.erase(it);
            else
                ++it;
        }
    }
}