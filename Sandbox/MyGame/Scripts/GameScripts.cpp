/*********************************************************************************************
 \file      GameScripts.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Implements sandbox/game-layer behaviours (scripts) registered into the engine’s
            LogicSystem via function-pointer tables (Init/Update/End).

 \details
            This file defines several behaviours used by level objects through
            BehaviourComponent.behaviourKey:
            - GameDirector    : Optional global gameplay director (debug/utility).
            - PlayerController: Player input, movement, combat combo/throw, animation state,
                               and run VFX spawning.
            - CombatDirector  : Centralized hitbox vs enemy checks for melee interactions.
            - VfxCleanup      : Cleans up one-shot impact VFX objects when animation finishes.
            - GateLogic       : Handles level transition when player collides with gate and
                               all enemies are cleared.

            A small amount of shared state is stored in file-scope statics:
            - gLogicSystem            : Bound from LogicSystem to access input/factory/level ops.
            - gPendingGateTransition  : Prevents repeated level load triggers.
            - gPlayerStates           : Per-player controller state keyed by object ID.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "Systems/LogicSystem.h"
#include "Systems/HitBoxSystem.h"
#include "Factory/Factory.h"
#include "Core/PathUtils.h"
#include "Systems/RenderSystem.h"
#include "Systems/ParticleSystem.h"
#include "../VfxPresets.hpp"
#include "../ParticlePresets.hpp"
#include "../Audio/GameAudioSetup.h"

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
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace {

    /*****************************************************************************************
      \brief Pointer to the engine LogicSystem, bound once by the game layer.

      \details
      Behaviours use this pointer to access:
      - Input manager
      - Factory object list / spawning / destroying
      - Level load / query helpers (e.g., FindAnyAlivePlayer)
      This is set in mygame::BindBehaviourContext().
    *****************************************************************************************/
    Framework::LogicSystem* gLogicSystem = nullptr;

    /*****************************************************************************************
      \brief Guards gate-triggered transitions to prevent multiple LoadLevel calls.
    *****************************************************************************************/
    bool gPendingGateTransition = false;

    /*****************************************************************************************
      \enum PlayerAnimState
      \brief High-level animation state machine used by PlayerController.
    *****************************************************************************************/
    enum class PlayerAnimState { Idle, Run, Attack1, Attack2, Attack3, Throw, Knockback, Death };

    /*****************************************************************************************
      \struct PlayerAnimConfig
      \brief Minimal extracted sprite-sheet configuration needed for timing and framing.
    *****************************************************************************************/
    struct PlayerAnimConfig {
        int cols{ 1 };      ///< Sprite-sheet columns
        int rows{ 1 };      ///< Sprite-sheet rows
        int frames{ 1 };    ///< Total frames for the animation
        float fps{ 1.0f };  ///< Frames per second for playback
    };

    /*****************************************************************************************
      \struct PendingThrow
      \brief Captures a queued projectile spawn (position + direction) to be fired later.

      \details
      Used so projectile spawns can be synchronized to the end of the Throw animation.
    *****************************************************************************************/
    struct PendingThrow {
        bool active{ false };   ///< Whether a throw is queued
        float spawnX{ 0.0f };   ///< Projectile spawn world X
        float spawnY{ 0.0f };   ///< Projectile spawn world Y
        float dirX{ 0.0f };     ///< Normalized aim direction X
        float dirY{ 0.0f };     ///< Normalized aim direction Y
    };

    /*****************************************************************************************
      \struct PlayerControllerState
      \brief Per-player runtime state for movement/combat/animation control.

      \details
      Stored in gPlayerStates keyed by object ID so multiple players can be supported.
    *****************************************************************************************/
    struct PlayerControllerState {
        PlayerAnimState animState{ PlayerAnimState::Idle }; ///< Current animation state
        int frame{ 0 };                                     ///< Current frame index (local)
        float frameClock{ 0.0f };                            ///< Accumulator for frame advance
        float attackTimer{ 0.0f };                           ///< Remaining time for attack anim
        int comboStep{ 0 };                                  ///< 1..3 combo cycle step
        float knockbackAnimTimer{ 0.0f };                    ///< Remaining knockback anim time
        PendingThrow pendingThrow{};                         ///< Queued throw spawn data
        float throwCooldownTimer{ 0.0f };                    ///< Cooldown gate for throw
        bool throwRequestQueued{ false };                    ///< RMB held/queued request
        float runParticleTimer{ 0.0f };                      ///< Timer for run particle cadence
        float footstepTimer{ 0.0f };                         ///< Timer for footstep sound cadence
        std::unique_ptr<mygame::GameAudio> audio;           ///< Game-side audio facade
        float lastAimDirX{ 1.0f };                          ///< Current aim direction x for player [Default right]
        float lastAimDirY{ 0.0f };                          ///< Current aim direction y for player
        bool usingControllerLast{ false };                  ///< Checks if player is using controller or not
        float lastMouseX{ 0.0f };                           ///< To store mouse's X coordinates
        float lastMouseY{ 0.0f };                           ///< To store mouse's Y coordinates
    };

    /*****************************************************************************************
      \brief Global map of per-player controller state, keyed by object ID.
    *****************************************************************************************/
    std::unordered_map<Framework::GOCId, PlayerControllerState> gPlayerStates;

    /*****************************************************************************************
      \brief Case-insensitive string equality check for ASCII strings.
      \param a First string view.
      \param b Second string view.
      \return True if same length and equal ignoring case.
    *****************************************************************************************/
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

    /*****************************************************************************************
      \brief Safe wrapper for GetComponentType: treats nullptr and (T*)-1 as invalid.
      \details Some frameworks or bugs can return sentinel pointer values (e.g., -1).
               Dereferencing those causes AVs reading 0xFFFFFFFFFFFFFFFF.  This helper
               centralizes detection and logs sentinel occurrences so they can be traced.
    *****************************************************************************************/
    template <typename T>
    T* SafeGetComponent(Framework::GameObjectComposition* obj, Framework::ComponentTypeId id)
    {
        if (!obj)
            return nullptr;
        T* p = obj->GetComponentType<T>(id);
        if (!p)
            return nullptr;
        const uintptr_t v = reinterpret_cast<uintptr_t>(p);
        if (v == static_cast<uintptr_t>(-1))
        {
            std::cerr << "[Warning] SafeGetComponent detected sentinel (0xFFFFFFFFFFFFFFFF) for component id "
                << static_cast<int>(id) << " on object " << obj->GetId() << "\n";
            return nullptr;
        }
        return p;
    }

    /*****************************************************************************************
      \brief Finds the animation index in SpriteAnimationComponent for a given PlayerAnimState.
      \param comp SpriteAnimationComponent to search.
      \param state Desired player state.
      \return Animation index if found, otherwise -1.

      \details
      This maps PlayerAnimState → animation name string:
      idle/run/attack1/attack2/attack3/throw/knockback/death.
    *****************************************************************************************/
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

    /*****************************************************************************************
      \brief Extracts columns/rows/frames/fps for the given state from SpriteAnimationComponent.
      \param comp SpriteAnimationComponent containing animation definitions.
      \param state Desired player animation state.
      \return PlayerAnimConfig with safe defaults if animation is missing.

      \details
      Defaults are kept conservative (1 frame @ 1 fps) so timing logic remains stable even
      if an animation entry is missing or misconfigured.
    *****************************************************************************************/
    PlayerAnimConfig ConfigFromSpriteSheet(const Framework::SpriteAnimationComponent* comp, PlayerAnimState state)
    {
        PlayerAnimConfig cfg{};
        if (!comp)
            return cfg;

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

    /*****************************************************************************************
      \brief Returns true if the state is one of the attack-related states.
    *****************************************************************************************/
    bool IsAttackState(PlayerAnimState state)
    {
        return state == PlayerAnimState::Attack1 ||
            state == PlayerAnimState::Attack2 ||
            state == PlayerAnimState::Attack3 ||
            state == PlayerAnimState::Throw;
    }

    /*****************************************************************************************
      \brief Converts a 1..N combo step into Attack1/Attack2/Attack3, wrapping by 3.
      \param comboIndex Combo step (1-based).
      \return Corresponding attack animation state.
    *****************************************************************************************/
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

    /*****************************************************************************************
      \brief Computes the duration in seconds for a given attack state based on sprite-sheet fps.
      \param player Player object composition.
      \param state Attack/throw state to time.
      \return Duration in seconds (frames / fps), using safe defaults if config missing.
    *****************************************************************************************/
    float AttackDurationForState(Framework::GameObjectComposition* player, PlayerAnimState state)
    {
        auto* animComp = player
            ? SafeGetComponent<Framework::SpriteAnimationComponent>(player, Framework::ComponentTypeId::CT_SpriteAnimationComponent)
            : nullptr;

        const PlayerAnimConfig cfg = ConfigFromSpriteSheet(animComp, state);
        return static_cast<float>(cfg.frames) / cfg.fps;
    }

    /*****************************************************************************************
      \brief Updates the controller state to a new animation state and applies it to the component.
      \param player Player object composition.
      \param state PlayerControllerState to mutate.
      \param next Next desired PlayerAnimState.

      \details
      - Resets local frame timers on state transition.
      - Locates the corresponding animation entry by name and sets it active if needed.
    *****************************************************************************************/
    void SetAnimState(Framework::GameObjectComposition* player, PlayerControllerState& state, PlayerAnimState next)
    {
        if (state.animState != next)
        {
            state.animState = next;
            state.frame = 0;
            state.frameClock = 0.0f;
        }

        auto* anim = SafeGetComponent<Framework::SpriteAnimationComponent>(player, Framework::ComponentTypeId::CT_SpriteAnimationComponent);
        const int index = AnimationIndexForState(anim, state.animState);
        if (anim && index >= 0)
        {
            const int activeIdx = anim->ActiveAnimationIndex();
            if (index != activeIdx)
                anim->SetActiveAnimation(index);
        }
    }

    /*****************************************************************************************
      \brief Counts the number of enemy objects that are currently alive.
      \return Number of enemies with EnemyComponent and not marked dead.

      \details
      Iterates the factory object list and filters by EnemyComponent + EnemyHealthComponent.
    *****************************************************************************************/
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

    /*****************************************************************************************
      \brief True if at least one enemy is still alive in the current level.
    *****************************************************************************************/
    bool HasRemainingEnemies()
    {
        return CountAliveEnemies() > 0;
    }

    /*****************************************************************************************
      \brief GameDirector behaviour: Init hook (optional global director).
    *****************************************************************************************/
    void GameDirector_Init(Framework::GameObjectComposition*)
    {
        std::cout << "[Behaviour] GameDirector init\n";
    }

    /*****************************************************************************************
      \brief GameDirector behaviour: Update hook.
      \details Currently polls enemy count (can be extended for objectives/UI).
    *****************************************************************************************/
    void GameDirector_Update(Framework::GameObjectComposition*, float)
    {
        (void)CountAliveEnemies();
    }

    /*****************************************************************************************
      \brief GameDirector behaviour: End hook.
    *****************************************************************************************/
    void GameDirector_End(Framework::GameObjectComposition*) {}

    /*****************************************************************************************
      \brief PlayerController behaviour: Init hook.
      \param obj Player object composition.

      \details
      Initializes per-player controller state in gPlayerStates.
    *****************************************************************************************/
    void PlayerController_Init(Framework::GameObjectComposition* obj)
    {
        if (!obj)
            return;
        auto& state = gPlayerStates[obj->GetId()];
        state = PlayerControllerState{};

        if (auto* audio = SafeGetComponent<Framework::AudioComponent>(
            obj, Framework::ComponentTypeId::CT_AudioComponent))
        {
            state.audio = std::make_unique<mygame::GameAudio>(
                audio, mygame::GameAudio::Entity::Player);
        }
        std::cout << "[Behaviour] PlayerController init\n";
    }
    /*****************************************************************************************
      \brief PlayerController behaviour: Update hook (movement, combat, animation, VFX).
      \param obj Player object composition.
      \param dt  Delta time in seconds.

      \details
      Responsibilities:
      - Read input (WASD + mouse).
      - Convert mouse screen position to world space for aiming.
      - Update rigidbody velocity with dampening and lunge/knockback constraints.
      - Spawn melee hitboxes on LMB (combo attack 1..3).
      - Queue and spawn projectile on RMB (throw) with cooldown.
      - Drive animation state machine (idle/run/attacks/throw/knockback/death).
      - Spawn run particles on movement cadence.
    *****************************************************************************************/
    void PlayerController_Update(Framework::GameObjectComposition* obj, float dt)
    {
        if (!gLogicSystem || !obj)
            return;

        auto& input = gLogicSystem->Input(); 
        auto& state = gPlayerStates[obj->GetId()];

        auto* tr = SafeGetComponent<Framework::TransformComponent>(obj, Framework::ComponentTypeId::CT_TransformComponent);
        auto* rc = SafeGetComponent<Framework::RenderComponent>(obj, Framework::ComponentTypeId::CT_RenderComponent);
        auto* rb = SafeGetComponent<Framework::RigidBodyComponent>(obj, Framework::ComponentTypeId::CT_RigidBodyComponent);
        auto* attack = SafeGetComponent<Framework::PlayerAttackComponent>(obj, Framework::ComponentTypeId::CT_PlayerAttackComponent);
        auto* audio = SafeGetComponent<Framework::AudioComponent>(obj, Framework::ComponentTypeId::CT_AudioComponent);
        if (!state.audio && audio)
        {
            state.audio = std::make_unique<mygame::GameAudio>(
                audio, mygame::GameAudio::Entity::Player);
        }
        auto* health = SafeGetComponent<Framework::PlayerHealthComponent>(obj, Framework::ComponentTypeId::CT_PlayerHealthComponent);

        if (!(tr && rc && rb && attack && health) || health->isDead)
            return;

        auto mouse = input.Manager().GetMouseState();
        float mouseWorldX = 0.0f;
        float mouseWorldY = 0.0f;
        bool mouseInsideViewport = false;
        float aimDirX = 0.0f;
        float aimDirY = 0.0f;

        /*************************************************************************************
          \brief For controller aiming
          \details Also flips the sprite horizontally by flipping RenderComponent width sign.
        **************************************************************************************/
        float stickX = input.Manager().GetGamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_X);
        float stickY = input.Manager().GetGamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_Y);

        // Invert the Y because gamepad Y is usually opposite of screen Y, but remove if it isnt.
        stickY = -stickY;

        const float stickDeadzone = 0.2f;
        bool controllerActive = false;

        // --------------------------------------------------
 // Controller Aim
 // --------------------------------------------------
        if (std::fabs(stickX) > stickDeadzone || std::fabs(stickY) > stickDeadzone)
        {
            const float len = std::sqrt(stickX * stickX + stickY * stickY);
            if (len > 0.0001f)
            {
                aimDirX = stickX / len;
                aimDirY = stickY / len;

                state.lastAimDirX = aimDirX;
                state.lastAimDirY = aimDirY;

                state.usingControllerLast = true;
                controllerActive = true;
            }
        }

        // --------------------------------------------------
        // Mouse Aim (only if controller not actively moving)
        // --------------------------------------------------
        bool mouseMoved = (mouse.x != state.lastMouseX || mouse.y != state.lastMouseY);

        if (!controllerActive && mouseMoved)
        {
            if (auto* rs = Framework::RenderSystem::Get())
            {
                if (rs->ScreenToWorld(mouse.x, mouse.y,
                    mouseWorldX, mouseWorldY,
                    mouseInsideViewport) && mouseInsideViewport)
                {
                    const float dx = mouseWorldX - tr->x;
                    const float dy = mouseWorldY - tr->y;
                    const float lenSq = dx * dx + dy * dy;

                    if (lenSq > 1e-6f)
                    {
                        const float invLen = 1.0f / std::sqrt(lenSq);
                        aimDirX = dx * invLen;
                        aimDirY = dy * invLen;

                        state.lastAimDirX = aimDirX;
                        state.lastAimDirY = aimDirY;
                        state.usingControllerLast = false;
                    }
                }
            }
        }

        // If nothing changed this frame, keep last direction
        if (!controllerActive && !mouseMoved)
        {
            aimDirX = state.lastAimDirX;
            aimDirY = state.lastAimDirY;
        }
        state.lastMouseX = mouse.x;
        state.lastMouseY = mouse.y;
        
        /*************************************************************************************
          \brief Flips the sprite based on final aim direction, for both mouse and controller
        **************************************************************************************/
        if (std::fabs(aimDirX) > 0.001f)
        {
            if (aimDirX >= 0.0f)
                rc->w = std::abs(rc->w);
            else
                rc->w = -std::abs(rc->w);
        }

        /*************************************************************************************
          \brief Decrement knockback timers (physics + animation).
        **************************************************************************************/
        if (rb->knockbackTime > 0.0f)
            rb->knockbackTime = std::max(0.0f, rb->knockbackTime - dt);
        if (state.knockbackAnimTimer > 0.0f)
            state.knockbackAnimTimer = std::max(0.0f, state.knockbackAnimTimer - dt);

        const bool isKnockback = rb->knockbackTime > 0.0f || state.knockbackAnimTimer > 0.0f;
        const bool isThrowing = state.animState == PlayerAnimState::Throw;
        const bool isMeleeAttacking = state.animState == PlayerAnimState::Attack1 ||
            state.animState == PlayerAnimState::Attack2 ||
            state.animState == PlayerAnimState::Attack3;

        /*************************************************************************************
          \brief Movement integration (normal movement > locked during melee/throw/knockback).
        **************************************************************************************/
        if (rb->lungeTime > 0.0f)
        {
            rb->lungeTime -= dt;
            if (rb->lungeTime <= 0.0f)
            {
                rb->velX = 0.0f;
                rb->lungeTime = 0.0f;
            }
        }
        else if (!isKnockback && !isThrowing && !isMeleeAttacking)
        {
            const float forwardX = (rc->w >= 0.0f) ? 1.0f : -1.0f;
            float speedModifier = 1.0f;
            if ((input.MoveRight() && forwardX < 0) || (input.MoveLeft() && forwardX > 0))
                speedModifier = 0.75f;

            if (input.MoveRight()) rb->velX = std::max(rb->velX, 1.f * speedModifier);
            if (input.MoveLeft()) rb->velX = std::min(rb->velX, -1.f * speedModifier);
            if (!input.MoveLeft() && !input.MoveRight()) rb->velX *= rb->dampening;

            if (input.MoveUp()) rb->velY = std::max(rb->velY, 1.f);
            if (input.MoveDown()) rb->velY = std::min(rb->velY, -1.f);
            if (!input.MoveUp() && !input.MoveDown()) rb->velY *= rb->dampening;
        }
        else if ((isThrowing || isMeleeAttacking) && !isKnockback)
        {
            rb->velX = 0.0f;
            rb->velY = 0.0f;
        }

        /*************************************************************************************
          \brief Run/idle intent used for animation and run particle spawning.
        **************************************************************************************/
        /*const bool wantRun = input.IsKeyHeld(GLFW_KEY_A) || input.IsKeyHeld(GLFW_KEY_D) ||
            input.IsKeyHeld(GLFW_KEY_W) || input.IsKeyHeld(GLFW_KEY_S) ||
            input.IsKeyHeld(GLFW_KEY_LEFT) || input.IsKeyHeld(GLFW_KEY_RIGHT) ||
            input.IsKeyHeld(GLFW_KEY_UP) || input.IsKeyHeld(GLFW_KEY_DOWN);*/

        const bool wantRun = input.MoveUp() || input.MoveDown() || input.MoveLeft() || input.MoveRight();

        state.runParticleTimer = std::max(0.0f, state.runParticleTimer - dt);
        state.footstepTimer = std::max(0.0f, state.footstepTimer - dt);
        const bool isMoving = std::fabs(rb->velX) > 0.01f || std::fabs(rb->velY) > 0.01f;
        if (wantRun && isMoving && state.runParticleTimer <= 0.0f)
        {
            if (auto* particleSystem = Framework::ParticleSystem::Instance())
            {
                const float facingDir = (rc->w >= 0.0f) ? 1.0f : -1.0f;
                mygame::SpawnRunParticles(*particleSystem, { tr->x, tr->y }, facingDir);
            }
            state.runParticleTimer = 0.08f;
        }
        if (wantRun && isMoving && !isKnockback && !isThrowing && state.audio && state.footstepTimer <= 0.0f)
        {
            state.audio->PlayFootstep();
            state.footstepTimer = 0.32f;
        }

        /*************************************************************************************
          \brief Update player attack component (handles internal timers/logic).
        **************************************************************************************/
        attack->Update(dt, tr);

        /*************************************************************************************
          \brief Throw cooldown ticking.
        **************************************************************************************/
        if (state.throwCooldownTimer > 0.0f)
            state.throwCooldownTimer = std::max(0.0f, state.throwCooldownTimer - dt);

        /*************************************************************************************
          \brief Input: queue/hold throw request via RMB.
        **************************************************************************************/
        const bool knockbackActive = rb->knockbackTime > 0.0f || state.knockbackAnimTimer > 0.0f;
        if (input.RangedAttack() || (knockbackActive && input.RangedHeld()))
            state.throwRequestQueued = true;
        if (input.RangedReleased())
            state.throwRequestQueued = false;

        /*************************************************************************************
          \brief Input: LMB melee attack triggers hitbox + combo animation.
        **************************************************************************************/
        const bool canStartMelee = !IsAttackState(state.animState) && !knockbackActive;
        if (input.MeleeAttack() && canStartMelee &&
            (aimDirX != 0.0f || aimDirY != 0.0f))
        {
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
        /*************************************************************************************
          \brief Input: RMB throw request queues a projectile to be spawned after throw animation.
        **************************************************************************************/
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

        /*************************************************************************************
          \brief State machine priority: Death > Knockback > Attack/Throw > Run/Idle.
        **************************************************************************************/
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

        /*************************************************************************************
          \brief Manual frame stepping (local) based on fps; SpriteAnimationComponent holds config.
        **************************************************************************************/
        auto* animComp = SafeGetComponent<Framework::SpriteAnimationComponent>(obj, Framework::ComponentTypeId::CT_SpriteAnimationComponent);
        const PlayerAnimConfig cfg = ConfigFromSpriteSheet(animComp, state.animState);
        state.frameClock += dt * cfg.fps;
        while (state.frameClock >= 1.0f)
        {
            state.frameClock -= 1.0f;
            state.frame = (state.frame + 1) % std::max(1, cfg.frames);
        }

        /*************************************************************************************
          \brief Spawn queued projectile when throw animation completes.
        **************************************************************************************/
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
            if (state.audio)
                state.audio->PlayGrapple();
            state.pendingThrow.active = false;
        }
    }

    /*****************************************************************************************
      \brief PlayerController behaviour: End hook.
      \details Removes per-player state entry from gPlayerStates.
    *****************************************************************************************/
    void PlayerController_End(Framework::GameObjectComposition* obj)
    {
        if (!obj)
            return;
        gPlayerStates.erase(obj->GetId());
    }

    /*****************************************************************************************
      \brief CombatDirector behaviour: Init hook.
    *****************************************************************************************/
    void CombatDirector_Init(Framework::GameObjectComposition*)
    {
        std::cout << "[Behaviour] CombatDirector init\n";
    }

    /*****************************************************************************************
      \brief CombatDirector behaviour: Update hook (central melee hitbox vs enemy collision).
      \details
      - Finds a live player.
      - Reads PlayerAttackComponent hitbox AABB (if active).
      - Iterates level objects named "Enemy" and checks AABB overlap.
      - Deactivates hurtbox on first confirmed hit to avoid multi-hit in one swing.
    *****************************************************************************************/
    void CombatDirector_Update(Framework::GameObjectComposition*, float)
    {
        if (!gLogicSystem || !gLogicSystem->Factory())
            return;

        auto* player = gLogicSystem->FindAnyAlivePlayer();
        if (!player)
            return;

        auto* attack = SafeGetComponent<Framework::PlayerAttackComponent>(player, Framework::ComponentTypeId::CT_PlayerAttackComponent);
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

            auto* rb = SafeGetComponent<Framework::RigidBodyComponent>(obj, Framework::ComponentTypeId::CT_RigidBodyComponent);
            auto* tr = SafeGetComponent<Framework::TransformComponent>(obj, Framework::ComponentTypeId::CT_TransformComponent);
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

    /*****************************************************************************************
      \brief CombatDirector behaviour: End hook.
    *****************************************************************************************/
    void CombatDirector_End(Framework::GameObjectComposition*) {}

    /*****************************************************************************************
      \brief VfxCleanup behaviour: Init hook.
    *****************************************************************************************/
    void VfxCleanup_Init(Framework::GameObjectComposition*)
    {
        std::cout << "[Behaviour] VfxCleanup init\n";
    }

    /*****************************************************************************************
      \brief VfxCleanup behaviour: Update hook (destroy finished impact VFX objects).
      \details
      - Scans factory objects for impact VFX objects (Framework::IsImpactVfxObject).
      - Checks active animation config to see if the non-looping "impact" animation ended.
      - Collects finished objects, then destroys them via factory.
    *****************************************************************************************/
    void VfxCleanup_Update(Framework::GameObjectComposition*, float)
    {
        if (!gLogicSystem || !gLogicSystem->Factory())
            return;

        std::vector<Framework::GameObjectComposition*> finishedVfx;

        for (auto const& [id, ptr] : gLogicSystem->Factory()->Objects())
        {
            (void)id;
            auto* obj = ptr.get();
            if (!obj || !mygame::IsImpactVfxObject(obj))
                continue;

            auto* anim = SafeGetComponent<Framework::SpriteAnimationComponent>(obj, Framework::ComponentTypeId::CT_SpriteAnimationComponent);
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

    /*****************************************************************************************
      \brief VfxCleanup behaviour: End hook.
    *****************************************************************************************/
    void VfxCleanup_End(Framework::GameObjectComposition*) {}

    /*****************************************************************************************
      \brief GateLogic behaviour: Init hook.
      \details Resets gPendingGateTransition so the gate can trigger again in a new level.
    *****************************************************************************************/
    void GateLogic_Init(Framework::GameObjectComposition*)
    {
        gPendingGateTransition = false;
        std::cout << "[Behaviour] GateLogic init\n";
    }

    /*****************************************************************************************
      \brief GateLogic behaviour: Update hook (level transition gate).
      \param gateObject Gate object composition.
      \param dt         Delta time (unused).
      \details
      Gate activates only when:
      - No remaining enemies exist.
      - Player is alive.
      - Player AABB overlaps gate AABB.
      Then it resolves GateTargetComponent.levelPath (relative → data path) and calls
      LogicSystem::LoadLevel(). A guard flag prevents repeated triggers.
    *****************************************************************************************/
    void GateLogic_Update(Framework::GameObjectComposition* gateObject, float)
    {
        if (!gLogicSystem || !gateObject || gPendingGateTransition)
            return;

        if (HasRemainingEnemies())
            return;

        auto* player = gLogicSystem->FindAnyAlivePlayer();
        if (!player)
            return;

        auto* gateTr = SafeGetComponent<Framework::TransformComponent>(gateObject, Framework::ComponentTypeId::CT_TransformComponent);
        auto* gateRb = SafeGetComponent<Framework::RigidBodyComponent>(gateObject, Framework::ComponentTypeId::CT_RigidBodyComponent);
        auto* playerTr = SafeGetComponent<Framework::TransformComponent>(player, Framework::ComponentTypeId::CT_TransformComponent);
        auto* playerRb = SafeGetComponent<Framework::RigidBodyComponent>(player, Framework::ComponentTypeId::CT_RigidBodyComponent);
        auto* playerHealth = SafeGetComponent<Framework::PlayerHealthComponent>(player, Framework::ComponentTypeId::CT_PlayerHealthComponent);

        if (!gateTr || !gateRb || !playerTr || !playerRb || (playerHealth && playerHealth->isDead))
            return;

        Framework::AABB gateBox(gateTr->x, gateTr->y, gateRb->width, gateRb->height);
        Framework::AABB playerBox(playerTr->x, playerTr->y, playerRb->width, playerRb->height);

        if (!Framework::Collision::CheckCollisionRectToRect(playerBox, gateBox))
            return;

        auto* target = SafeGetComponent<Framework::GateTargetComponent>(gateObject, Framework::ComponentTypeId::CT_GateTargetComponent);
        if (!target || target->levelPath.empty())
            return;

        std::filesystem::path targetPath(target->levelPath);
        if (!targetPath.is_absolute())
            targetPath = Framework::ResolveDataPath(targetPath);

        gPendingGateTransition = true;
        gLogicSystem->LoadLevel(targetPath);
    }

    /*****************************************************************************************
      \brief GateLogic behaviour: End hook.
      \details Clears transition guard.
    *****************************************************************************************/
    void GateLogic_End(Framework::GameObjectComposition*)
    {
        gPendingGateTransition = false;
    }
}

namespace mygame {

    /*****************************************************************************************
      \brief Binds the engine LogicSystem pointer into this translation unit for behaviours.
      \param logic Engine LogicSystem reference.
    *****************************************************************************************/
    void BindBehaviourContext(Framework::LogicSystem& logic)
    {
        gLogicSystem = &logic;
    }

    /*****************************************************************************************
      \brief Registers all behaviour keys and their lifecycle callbacks into LogicSystem.
      \param logic Engine LogicSystem reference used for registration.

      \details
      Objects with BehaviourComponent.behaviourKey matching these keys will have their
      Init/Update/End functions dispatched by the engine.
    *****************************************************************************************/
    void RegisterGameBehaviourFunctions(Framework::LogicSystem& logic)
    {
        logic.RegisterBehaviour("GameDirector", { GameDirector_Init, GameDirector_Update, GameDirector_End });
        logic.RegisterBehaviour("PlayerController", { PlayerController_Init, PlayerController_Update, PlayerController_End });
        logic.RegisterBehaviour("CombatDirector", { CombatDirector_Init, CombatDirector_Update, CombatDirector_End });
        logic.RegisterBehaviour("VfxCleanup", { VfxCleanup_Init, VfxCleanup_Update, VfxCleanup_End });
        logic.RegisterBehaviour("GateLogic", { GateLogic_Init, GateLogic_Update, GateLogic_End });
    }

}
