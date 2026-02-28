#include "Systems/LogicSystem.h"
#include "Factory/Factory.h"
#include "Core/PathUtils.h"
#include "Systems/RenderSystem.h"
#include "Systems/ParticleSystem.h"
#include "Systems/VfxHelpers.h"

#include "Component/AudioComponent.h"
#include "Component/EnemyComponent.h"
#include "Component/EnemyHealthComponent.h"
#include "Component/GateTargetComponent.h"
#include "Component/PlayerAttackComponent.h"
#include "Component/PlayerHealthComponent.h"
#include "Component/RenderComponent.h"
#include "Component/SpriteAnimationComponent.h"
#include "Component/TransformComponent.h"
#include "Component/HitBoxComponent.h"
#include "Physics/Collision/Collision.h"
#include "Physics/Dynamics/RigidBodyComponent.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {
    Framework::LogicSystem* gLogicSystem = nullptr;
    bool gPendingGateTransition = false;

    enum class PlayerAnimState { Idle, Run, Attack1, Attack2, Attack3, Throw, Knockback, Death };

    struct PlayerAnimConfig {
        int cols{ 1 };
        int rows{ 1 };
        int frames{ 1 };
        float fps{ 1.0f };
    };

    struct PendingThrow {
        bool active{ false };
        float spawnX{ 0.0f };
        float spawnY{ 0.0f };
        float dirX{ 0.0f };
        float dirY{ 0.0f };
    };

    struct PlayerControllerState {
        PlayerAnimState animState{ PlayerAnimState::Idle };
        int frame{ 0 };
        float frameClock{ 0.0f };
        float attackTimer{ 0.0f };
        int comboStep{ 0 };
        float knockbackAnimTimer{ 0.0f };
        PendingThrow pendingThrow{};
        float throwCooldownTimer{ 0.0f };
        bool throwRequestQueued{ false };
        float runParticleTimer{ 0.0f };
    };

    std::unordered_map<Framework::GOCId, PlayerControllerState> gPlayerStates;

    bool EqualsIgnoreCase(std::string_view a, std::string_view b)
    {
        if (a.size() != b.size())
            return false;
        for (std::size_t i = 0; i < a.size(); ++i)
        {
            unsigned char c1 = static_cast<unsigned char>(a[i]);
            unsigned char c2 = static_cast<unsigned char>(b[i]);
            if (std::tolower(c1) != std::tolower(c2))
                return false;
        }
        return true;
    }

    int AnimationIndexForState(const Framework::SpriteAnimationComponent* comp, PlayerAnimState state)
    {
        if (!comp)
            return -1;

        std::string_view desired = "idle";
        switch (state)
        {
        case PlayerAnimState::Run: desired = "run"; break;
        case PlayerAnimState::Attack1: desired = "attack1"; break;
        case PlayerAnimState::Attack2: desired = "attack2"; break;
        case PlayerAnimState::Attack3: desired = "attack3"; break;
        case PlayerAnimState::Throw: desired = "throw"; break;
        case PlayerAnimState::Knockback: desired = "knockback"; break;
        case PlayerAnimState::Death: desired = "death"; break;
        case PlayerAnimState::Idle:
        default: desired = "idle"; break;
        }

        for (std::size_t i = 0; i < comp->animations.size(); ++i)
        {
            if (EqualsIgnoreCase(comp->animations[i].name, desired))
                return static_cast<int>(i);
        }

        return -1;
    }

    PlayerAnimConfig ConfigFromSpriteSheet(const Framework::SpriteAnimationComponent* comp, PlayerAnimState state)
    {
        PlayerAnimConfig cfg{};
        const int index = AnimationIndexForState(comp, state);
        if (index < 0 || index >= static_cast<int>(comp->animations.size()))
            return cfg;

        const auto& sheet = comp->animations[static_cast<std::size_t>(index)];
        cfg.cols = std::max(1, sheet.config.columns);
        cfg.rows = std::max(1, sheet.config.rows);
        cfg.frames = std::max(1, sheet.config.totalFrames);
        cfg.fps = sheet.config.fps > 0.0f ? sheet.config.fps : 1.0f;
        return cfg;
    }

    bool IsAttackState(PlayerAnimState state)
    {
        return state == PlayerAnimState::Attack1 ||
            state == PlayerAnimState::Attack2 ||
            state == PlayerAnimState::Attack3 ||
            state == PlayerAnimState::Throw;
    }

    PlayerAnimState AttackStateForIndex(int comboIndex)
    {
        const int wrapped = ((comboIndex - 1) % 3 + 3) % 3;
        switch (wrapped)
        {
        case 0: return PlayerAnimState::Attack1;
        case 1: return PlayerAnimState::Attack2;
        default: return PlayerAnimState::Attack3;
        }
    }

    float AttackDurationForState(Framework::GameObjectComposition* player, PlayerAnimState state)
    {
        auto* animComp = player
            ? player->GetComponentType<Framework::SpriteAnimationComponent>(Framework::ComponentTypeId::CT_SpriteAnimationComponent)
            : nullptr;

        const PlayerAnimConfig cfg = ConfigFromSpriteSheet(animComp, state);
        return static_cast<float>(cfg.frames) / cfg.fps;
    }

    void SetAnimState(Framework::GameObjectComposition* player, PlayerControllerState& state, PlayerAnimState next)
    {
        if (state.animState != next)
        {
            state.animState = next;
            state.frame = 0;
            state.frameClock = 0.0f;
        }

        auto* anim = player->GetComponentType<Framework::SpriteAnimationComponent>(Framework::ComponentTypeId::CT_SpriteAnimationComponent);
        const int index = AnimationIndexForState(anim, state.animState);
        if (anim && index >= 0 && index != anim->ActiveAnimationIndex())
            anim->SetActiveAnimation(index);
    }

    int CountAliveEnemies()
    {
        if (!gLogicSystem || !gLogicSystem->Factory())
            return 0;

        int count = 0;
        for (auto const& [id, ptr] : gLogicSystem->Factory()->Objects())
        {
            (void)id;
            auto* obj = ptr.get();
            if (!obj)
                continue;

            if (!obj->GetComponentType<Framework::EnemyComponent>(Framework::ComponentTypeId::CT_EnemyComponent))
                continue;

            auto* health = obj->GetComponentType<Framework::EnemyHealthComponent>(Framework::ComponentTypeId::CT_EnemyHealthComponent);
            if (!health || !health->isDead)
                ++count;
        }

        return count;
    }

    bool HasRemainingEnemies()
    {
        return CountAliveEnemies() > 0;
    }

    void GameDirector_Init(Framework::GameObjectComposition*)
    {
        std::cout << "[Behaviour] GameDirector init\n";
    }

    void GameDirector_Update(Framework::GameObjectComposition*, float)
    {
        (void)CountAliveEnemies();
    }

    void GameDirector_End(Framework::GameObjectComposition*) {}

    void PlayerController_Init(Framework::GameObjectComposition* obj)
    {
        if (!obj)
            return;
        gPlayerStates[obj->GetId()] = PlayerControllerState{};
        std::cout << "[Behaviour] PlayerController init\n";
    }

    void PlayerController_Update(Framework::GameObjectComposition* obj, float dt)
    {
        if (!gLogicSystem || !obj)
            return;

        auto& input = gLogicSystem->Input();
        auto& state = gPlayerStates[obj->GetId()];

        auto* tr = obj->GetComponentType<Framework::TransformComponent>(Framework::ComponentTypeId::CT_TransformComponent);
        auto* rc = obj->GetComponentType<Framework::RenderComponent>(Framework::ComponentTypeId::CT_RenderComponent);
        auto* rb = obj->GetComponentType<Framework::RigidBodyComponent>(Framework::ComponentTypeId::CT_RigidBodyComponent);
        auto* attack = obj->GetComponentType<Framework::PlayerAttackComponent>(Framework::ComponentTypeId::CT_PlayerAttackComponent);
        auto* audio = obj->GetComponentType<Framework::AudioComponent>(Framework::ComponentTypeId::CT_AudioComponent);
        auto* health = obj->GetComponentType<Framework::PlayerHealthComponent>(Framework::ComponentTypeId::CT_PlayerHealthComponent);

        if (!(tr && rc && rb && attack && health) || health->isDead)
            return;

        auto mouse = input.Manager().GetMouseState();
        float mouseWorldX = 0.0f;
        float mouseWorldY = 0.0f;
        bool mouseInsideViewport = false;
        float aimDirX = 0.0f;
        float aimDirY = 0.0f;

        if (auto* rs = Framework::RenderSystem::Get())
        {
            if (rs->ScreenToWorld(mouse.x, mouse.y, mouseWorldX, mouseWorldY, mouseInsideViewport) && mouseInsideViewport)
            {
                const float dx = mouseWorldX - tr->x;
                const float dy = mouseWorldY - tr->y;
                const float lenSq = dx * dx + dy * dy;
                if (lenSq > 1e-6f)
                {
                    const float invLen = 1.0f / std::sqrt(lenSq);
                    aimDirX = dx * invLen;
                    aimDirY = dy * invLen;
                }
                if (aimDirX >= 0.0f) rc->w = std::abs(rc->w);
                else rc->w = -std::abs(rc->w);
            }
        }

        if (rb->knockbackTime > 0.0f)
            rb->knockbackTime = std::max(0.0f, rb->knockbackTime - dt);
        if (state.knockbackAnimTimer > 0.0f)
            state.knockbackAnimTimer = std::max(0.0f, state.knockbackAnimTimer - dt);

        const bool isKnockback = rb->knockbackTime > 0.0f || state.knockbackAnimTimer > 0.0f;
        const bool isThrowing = state.animState == PlayerAnimState::Throw;

        if (rb->lungeTime > 0.0f)
        {
            rb->lungeTime -= dt;
            if (rb->lungeTime <= 0.0f)
            {
                rb->velX = 0.0f;
                rb->lungeTime = 0.0f;
            }
        }
        else if (!isKnockback && !isThrowing)
        {
            const float forwardX = (rc->w >= 0.0f) ? 1.0f : -1.0f;
            float speedModifier = 1.0f;
            if ((input.IsKeyHeld(GLFW_KEY_D) && forwardX < 0) || (input.IsKeyHeld(GLFW_KEY_A) && forwardX > 0))
                speedModifier = 0.75f;

            if (input.IsKeyHeld(GLFW_KEY_D)) rb->velX = std::max(rb->velX, 1.f * speedModifier);
            if (input.IsKeyHeld(GLFW_KEY_A)) rb->velX = std::min(rb->velX, -1.f * speedModifier);
            if (!input.IsKeyHeld(GLFW_KEY_A) && !input.IsKeyHeld(GLFW_KEY_D)) rb->velX *= rb->dampening;

            if (input.IsKeyHeld(GLFW_KEY_W)) rb->velY = std::max(rb->velY, 1.f);
            if (input.IsKeyHeld(GLFW_KEY_S)) rb->velY = std::min(rb->velY, -1.f);
            if (!input.IsKeyHeld(GLFW_KEY_W) && !input.IsKeyHeld(GLFW_KEY_S)) rb->velY *= rb->dampening;
        }
        else if (isThrowing && !isKnockback)
        {
            rb->velX = 0.0f;
            rb->velY = 0.0f;
        }

        const bool wantRun = input.IsKeyHeld(GLFW_KEY_A) || input.IsKeyHeld(GLFW_KEY_D) ||
            input.IsKeyHeld(GLFW_KEY_W) || input.IsKeyHeld(GLFW_KEY_S) ||
            input.IsKeyHeld(GLFW_KEY_LEFT) || input.IsKeyHeld(GLFW_KEY_RIGHT) ||
            input.IsKeyHeld(GLFW_KEY_UP) || input.IsKeyHeld(GLFW_KEY_DOWN);

        state.runParticleTimer = std::max(0.0f, state.runParticleTimer - dt);
        const bool isMoving = std::fabs(rb->velX) > 0.01f || std::fabs(rb->velY) > 0.01f;
        if (wantRun && isMoving && state.runParticleTimer <= 0.0f)
        {
            if (auto* particleSystem = Framework::ParticleSystem::Instance())
            {
                const float facingDir = (rc->w >= 0.0f) ? 1.0f : -1.0f;
                particleSystem->SpawnRunParticles({ tr->x, tr->y }, facingDir);
            }
            state.runParticleTimer = 0.08f;
        }

        attack->Update(dt, tr);

        if (state.throwCooldownTimer > 0.0f)
            state.throwCooldownTimer = std::max(0.0f, state.throwCooldownTimer - dt);

        const bool knockbackActive = rb->knockbackTime > 0.0f || state.knockbackAnimTimer > 0.0f;
        if (input.IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT) || (knockbackActive && input.IsMouseHeld(GLFW_MOUSE_BUTTON_RIGHT)))
            state.throwRequestQueued = true;
        if (input.IsMouseReleased(GLFW_MOUSE_BUTTON_RIGHT))
            state.throwRequestQueued = false;

        if (input.IsMousePressed(GLFW_MOUSE_BUTTON_LEFT) && (aimDirX != 0.0f || aimDirY != 0.0f))
        {
            float dirX = (mouseWorldX > tr->x) ? 1.0f : -1.0f;
            rb->velX = dirX * 0.1f;
            rb->lungeTime = 0.15f;

            const float offset = 0.05f;
            const float halfW = std::abs(rc->w) * 0.5f;
            const float halfH = rc->h * 0.5f;
            const float hitX = tr->x + aimDirX * (halfW + offset);
            const float hitY = tr->y + aimDirY * (halfH + offset);

            if (gLogicSystem->hitBoxSystem)
            {
                gLogicSystem->hitBoxSystem->SpawnHitBox(obj,
                    hitX, hitY,
                    0.1f, 0.1f,
                    1.0f, 0.2f,
                    Framework::HitBoxComponent::Team::Player);
            }

            state.comboStep = (state.comboStep % 3) + 1;
            const auto comboState = AttackStateForIndex(state.comboStep);
            SetAnimState(obj, state, comboState);
            state.attackTimer = AttackDurationForState(obj, comboState);
        }
        else if (state.throwRequestQueued)
        {
            const bool canThrow = state.throwCooldownTimer <= 0.0f && !state.pendingThrow.active &&
                !IsAttackState(state.animState) && !knockbackActive;

            if (canThrow && (aimDirX != 0.0f || aimDirY != 0.0f))
            {
                const float offset = 0.05f;
                const float halfW = std::abs(rc->w) * 0.5f;
                const float halfH = rc->h * 0.5f;
                state.pendingThrow.active = true;
                state.pendingThrow.spawnX = tr->x + aimDirX * (halfW + offset);
                state.pendingThrow.spawnY = tr->y + aimDirY * (halfH + offset);
                state.pendingThrow.dirX = aimDirX;
                state.pendingThrow.dirY = aimDirY;

                SetAnimState(obj, state, PlayerAnimState::Throw);
                state.attackTimer = AttackDurationForState(obj, PlayerAnimState::Throw);
                state.throwCooldownTimer = std::max(state.throwCooldownTimer, state.attackTimer);
                state.throwRequestQueued = false;
            }
        }

        if (health->playerHealth <= 0)
        {
            state.knockbackAnimTimer = 0.0f;
            SetAnimState(obj, state, PlayerAnimState::Death);
        }
        else if (rb->knockbackTime > 0.0f)
        {
            if (state.animState != PlayerAnimState::Knockback)
            {
                state.pendingThrow.active = false;
                state.attackTimer = 0.0f;
            }
            if (state.knockbackAnimTimer <= 0.0f)
                state.knockbackAnimTimer = AttackDurationForState(obj, PlayerAnimState::Knockback);
            SetAnimState(obj, state, PlayerAnimState::Knockback);
        }
        else if (state.knockbackAnimTimer > 0.0f)
        {
            SetAnimState(obj, state, PlayerAnimState::Knockback);
        }
        else if (IsAttackState(state.animState))
        {
            state.attackTimer -= dt;
            if (state.attackTimer <= 0.0f)
            {
                state.attackTimer = 0.0f;
                SetAnimState(obj, state, wantRun ? PlayerAnimState::Run : PlayerAnimState::Idle);
            }
        }
        else
        {
            SetAnimState(obj, state, wantRun ? PlayerAnimState::Run : PlayerAnimState::Idle);
        }

        auto* animComp = obj->GetComponentType<Framework::SpriteAnimationComponent>(Framework::ComponentTypeId::CT_SpriteAnimationComponent);
        const PlayerAnimConfig cfg = ConfigFromSpriteSheet(animComp, state.animState);
        state.frameClock += dt * cfg.fps;
        while (state.frameClock >= 1.0f)
        {
            state.frameClock -= 1.0f;
            state.frame = (state.frame + 1) % std::max(1, cfg.frames);
        }

        if (state.pendingThrow.active && state.attackTimer <= 0.0f)
        {
            if (gLogicSystem->hitBoxSystem)
            {
                gLogicSystem->hitBoxSystem->SpawnProjectile(obj,
                    state.pendingThrow.spawnX, state.pendingThrow.spawnY,
                    state.pendingThrow.dirX, state.pendingThrow.dirY,
                    0.8f,
                    0.1f, 0.1f,
                    1.0f, 5.f, Framework::HitBoxComponent::Team::Thrown);
            }
            if (audio)
                audio->TriggerSound("GrappleShoot");
            state.pendingThrow.active = false;
        }
    }

    void PlayerController_End(Framework::GameObjectComposition* obj)
    {
        if (!obj)
            return;
        gPlayerStates.erase(obj->GetId());
    }

    void CombatDirector_Init(Framework::GameObjectComposition*)
    {
        std::cout << "[Behaviour] CombatDirector init\n";
    }

    void CombatDirector_Update(Framework::GameObjectComposition*, float)
    {
        if (!gLogicSystem || !gLogicSystem->Factory())
            return;

        auto* player = gLogicSystem->FindAnyAlivePlayer();
        if (!player)
            return;

        auto* attack = player->GetComponentType<Framework::PlayerAttackComponent>(Framework::ComponentTypeId::CT_PlayerAttackComponent);
        if (!attack || !attack->hitbox || !attack->hitbox->active)
            return;

        Framework::AABB playerHitBox(
            attack->hitbox->spawnX,
            attack->hitbox->spawnY,
            attack->hitbox->width,
            attack->hitbox->height);

        for (auto* obj : gLogicSystem->LevelObjects())
        {
            if (!obj || obj->GetObjectName() != "Enemy")
                continue;

            auto* rb = obj->GetComponentType<Framework::RigidBodyComponent>(Framework::ComponentTypeId::CT_RigidBodyComponent);
            auto* tr = obj->GetComponentType<Framework::TransformComponent>(Framework::ComponentTypeId::CT_TransformComponent);
            if (!(rb && tr))
                continue;

            Framework::AABB enemyBox(tr->x, tr->y, rb->width, rb->height);
            if (Framework::Collision::CheckCollisionRectToRect(playerHitBox, enemyBox))
            {
                attack->hitbox->DeactivateHurtBox();
                break;
            }
        }
    }

    void CombatDirector_End(Framework::GameObjectComposition*) {}

    void VfxCleanup_Init(Framework::GameObjectComposition*)
    {
        std::cout << "[Behaviour] VfxCleanup init\n";
    }

    void VfxCleanup_Update(Framework::GameObjectComposition*, float)
    {
        if (!gLogicSystem || !gLogicSystem->Factory())
            return;

        std::vector<Framework::GameObjectComposition*> finishedVfx;

        for (auto const& [id, ptr] : gLogicSystem->Factory()->Objects())
        {
            (void)id;
            auto* obj = ptr.get();
            if (!obj || !Framework::IsImpactVfxObject(obj))
                continue;

            auto* anim = obj->GetComponentType<Framework::SpriteAnimationComponent>(Framework::ComponentTypeId::CT_SpriteAnimationComponent);
            if (!anim)
                continue;

            if (auto* active = anim->ActiveAnimation())
            {
                if (!active->config.loop && active->name == "impact")
                {
                    const int total = std::max(1, active->config.totalFrames);
                    const int start = std::clamp(active->config.startFrame, 0, total - 1);
                    const int end = (active->config.endFrame >= 0)
                        ? std::clamp(active->config.endFrame, start, total - 1)
                        : total - 1;

                    const int current = std::clamp(active->currentFrame, 0, total - 1);
                    if (current >= end)
                        finishedVfx.push_back(obj);
                }
            }
        }

        for (auto* vfx : finishedVfx)
            gLogicSystem->Factory()->Destroy(vfx);
    }

    void VfxCleanup_End(Framework::GameObjectComposition*) {}

    void GateLogic_Init(Framework::GameObjectComposition*)
    {
        gPendingGateTransition = false;
        std::cout << "[Behaviour] GateLogic init\n";
    }

    void GateLogic_Update(Framework::GameObjectComposition* gateObject, float)
    {
        if (!gLogicSystem || !gateObject || gPendingGateTransition)
            return;

        if (HasRemainingEnemies())
            return;

        auto* player = gLogicSystem->FindAnyAlivePlayer();
        if (!player)
            return;

        auto* gateTr = gateObject->GetComponentType<Framework::TransformComponent>(Framework::ComponentTypeId::CT_TransformComponent);
        auto* gateRb = gateObject->GetComponentType<Framework::RigidBodyComponent>(Framework::ComponentTypeId::CT_RigidBodyComponent);
        auto* playerTr = player->GetComponentType<Framework::TransformComponent>(Framework::ComponentTypeId::CT_TransformComponent);
        auto* playerRb = player->GetComponentType<Framework::RigidBodyComponent>(Framework::ComponentTypeId::CT_RigidBodyComponent);
        auto* playerHealth = player->GetComponentType<Framework::PlayerHealthComponent>(Framework::ComponentTypeId::CT_PlayerHealthComponent);

        if (!gateTr || !gateRb || !playerTr || !playerRb || (playerHealth && playerHealth->isDead))
            return;

        Framework::AABB gateBox(gateTr->x, gateTr->y, gateRb->width, gateRb->height);
        Framework::AABB playerBox(playerTr->x, playerTr->y, playerRb->width, playerRb->height);

        if (!Framework::Collision::CheckCollisionRectToRect(playerBox, gateBox))
            return;

        auto* target = gateObject->GetComponentType<Framework::GateTargetComponent>(Framework::ComponentTypeId::CT_GateTargetComponent);
        if (!target || target->levelPath.empty())
            return;

        std::filesystem::path targetPath(target->levelPath);
        if (!targetPath.is_absolute())
            targetPath = Framework::ResolveDataPath(targetPath);

        gPendingGateTransition = true;
        gLogicSystem->LoadLevel(targetPath);
    }

    void GateLogic_End(Framework::GameObjectComposition*)
    {
        gPendingGateTransition = false;
    }
}

namespace mygame {

    void BindBehaviourContext(Framework::LogicSystem& logic)
    {
        gLogicSystem = &logic;
    }

    void RegisterGameBehaviourFunctions(Framework::LogicSystem& logic)
    {
        logic.RegisterBehaviour("GameDirector", { GameDirector_Init, GameDirector_Update, GameDirector_End });
        logic.RegisterBehaviour("PlayerController", { PlayerController_Init, PlayerController_Update, PlayerController_End });
        logic.RegisterBehaviour("CombatDirector", { CombatDirector_Init, CombatDirector_Update, CombatDirector_End });
        logic.RegisterBehaviour("VfxCleanup", { VfxCleanup_Init, VfxCleanup_Update, VfxCleanup_End });
        logic.RegisterBehaviour("GateLogic", { GateLogic_Init, GateLogic_Update, GateLogic_End });
    }

}
