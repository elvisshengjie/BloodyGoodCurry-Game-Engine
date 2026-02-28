/*********************************************************************************************
 \file      LogicSystem.cpp
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Author, 10%
            elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 20%
            yimo kong (yimo.kong@digipen.edu)      - Author, 10%
            Ho Jun (h.jun@digipen.edu) - Author, 20%

 \brief     Core gameplay loop and input-driven logic for the sandbox.
 \details   Owns high-level game orchestration:
            - Factory lifetime: component registration, prefab loading, level create/reload.
            - Player state: discovery and animation state (idle/run/melee combo/throw/knockback/death).
            - Input mapping: WASD move, LMB melee combo, RMB throw projectile, F1 overlay.
            - HitBoxSystem: spawns melee hitboxes and deferred projectile throws (after throw animation).
            - Editor hooks (when enabled): selection/spawn/debug tooling integration.
            - Crash logging: writes crash logs and supports a debug-only crash test.

 \copyright
            All content (c) 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "Common/CRTDebug.h"
#include "Systems/LogicSystem.h"
#include "Core/PathUtils.h"
#include "Systems/RenderSystem.h"      // for ScreenToWorld / camera-based world mapping
#include "Debug/Selection.h"
#include "Debug/Spawn.h"
#include "Systems/VfxHelpers.h"
#include "Memory/GameObjectPool.h"
#include "Resource_Asset_Manager/Resource_Manager.h"
#include "Systems/ParticleSystem.h"
#include <cctype>
#include <string>
#include <string_view>
#include <csignal>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <Debug/UndoStack.h>

#include "Common/CRTDebug.h"   // <- bring in DBG_NEW

#ifdef _DEBUG
#define new DBG_NEW       // <- redefine new AFTER all includes
#endif

namespace Framework {

    /*****************************************************************************************
      \brief Construct a LogicSystem bound to a window and input system.
      \param window  Reference to the active gfx::Window (dimension queries, etc.).
      \param input   Reference to the engine-wide InputSystem.
    *****************************************************************************************/
    LogicSystem::LogicSystem(gfx::Window& window, InputSystem& input)
        : window(&window), input(input) {
    }

    /// Defaulted virtual destructor; shuts down via Shutdown().
    LogicSystem::~LogicSystem() = default;

    /*****************************************************************************************
      \brief Check if a given object pointer still exists in the factory.
      \param obj Candidate object pointer.
      \return true if obj is found among factory->Objects(); false otherwise.
      \note   Useful to invalidate cached pointers when levels reload or objects are destroyed.
    *****************************************************************************************/
    bool LogicSystem::IsAlive(GOC* obj) const
    {
        if (!obj || !factory)
            return false;
        for (auto& [id, ptr] : factory->Objects())
        {
            (void)id;
            if (ptr.get() == obj)
                return true;
        }
        return false;
    }

    /*****************************************************************************************
      \brief Find the first alive object with a PlayerComponent.
    *****************************************************************************************/
    GOC* LogicSystem::FindAnyAlivePlayer()
    {
        if (!factory)
            return nullptr;

        for (auto& [id, ptr] : factory->Objects())
        {
            if (auto* obj = ptr.get())
            {
                if (obj->GetComponentType<PlayerComponent>(ComponentTypeId::CT_PlayerComponent))
                    return obj;
            }
        }
        return nullptr;
    }

    /*****************************************************************************************
      \brief Refresh references after level load or object churn.
             - Updates levelObjects with LastLevelObjects()
             - Finds/validates player
             - Finds/validates a default collision target ("rect", case-insensitive)
             - Caches player size if needed
    *****************************************************************************************/
    void LogicSystem::RefreshLevelReferences()
    {
        if (!factory)
            return;

        // Rebuild the level object cache every frame so we only keep alive objects.
        // The previous implementation grabbed the snapshot returned by
        // GameObjectFactory::LastLevelObjects(), which is only updated when a level is
        // loaded/saved. Once gameplay started, pointers to objects that were destroyed
        // (e.g. the player being killed by enemies) remained inside levelObjects even
        // though the underlying memory had been freed. Systems like HitBoxSystem
        // iterate this list every frame and dereference each pointer to query
        // components. Walking into enemies would quickly destroy either the player or
        // an enemy, leaving a dangling pointer behind and eventually causing an access
        // violation when the stale pointer was dereferenced. Rebuilding the cache from
        // the factory’s current ownership map guarantees we only keep valid objects.
        levelObjects.clear();
        for (auto const& [id, obj] : factory->Objects())
        {
            (void)id;
            if (obj)
                levelObjects.push_back(obj.get());
        }

        if (!IsAlive(player))
            player = nullptr;
        if (!player)
        {
            player = FindAnyAlivePlayer();
            if (player)
            {
                std::cout << "[LogicSystem] Player re-assigned to another alive instance: "
                    << player->GetObjectName() << "\n";
            }
        }

        if (!IsAlive(collisionTarget))
            collisionTarget = nullptr;

        gateController.SetPlayer(player);

        auto nameEqualsIgnoreCase = [](const std::string& lhs, std::string_view rhs)
            {
                if (lhs.size() != rhs.size())
                    return false;
                for (std::size_t i = 0; i < lhs.size(); ++i)
                {
                    unsigned char c1 = static_cast<unsigned char>(lhs[i]);
                    unsigned char c2 = static_cast<unsigned char>(rhs[i]);
                    if (std::tolower(c1) != std::tolower(c2))
                        return false;
                }
                return true;
            };

        if (!collisionTarget)
        {
            for (auto* obj : levelObjects)
            {
                if (obj && nameEqualsIgnoreCase(obj->GetObjectName(), "rect"))
                {
                    collisionTarget = obj;
                    break;
                }
            }
        }

        gateController.RefreshGateReference(levelObjects);
    }

    /*****************************************************************************************
      \brief Get the player's world position (if available).
      \param outX [out] Player world X.
      \param outY [out] Player world Y.
      \return true if player and TransformComponent are present; false otherwise.
    *****************************************************************************************/
    bool LogicSystem::GetPlayerWorldPosition(float& outX, float& outY) const
    {
        if (!IsAlive(player))
            return false;

        auto* tr = player->GetComponentType<Framework::TransformComponent>(
            Framework::ComponentTypeId::CT_TransformComponent);
        if (!tr)
            return false;

        outX = tr->x;
        outY = tr->y;
        return true;
    }

    std::filesystem::path LogicSystem::resolveData(std::string_view name) const
    {
        return Framework::ResolveDataPath(std::filesystem::path(name));
    }

    /*****************************************************************************************
      \brief Initialize the game logic systems and world.
             - Sets up crash logging (file + logcat mirror).
             - Installs terminate/signal handlers.
             - Instantiates factory; registers components; loads prefabs; creates initial level.
             - Discovers player and caches initial size; loads window config.
             - Builds HitBoxSystem and prints control help.
    *****************************************************************************************/
    void LogicSystem::Initialize()
    {
        auto crashLogDir = Framework::GetUserDocumentsDir() / "BloodyGoodCurry" / "logs";
        crashLogger = std::make_unique<CrashLogger>(crashLogDir.string(),
            std::string("crash.log"),
            std::string("ENGINE/CRASH"));
        g_crashLogger = crashLogger.get();
        std::cout << "[CrashLog] " << g_crashLogger->LogPath() << "\n";
#ifndef NDEBUG
        std::cout << "[CrashLog] Press F9 to force a crash-test (logs to file + logcat).\n";
#endif
        std::cout << "[CrashLog] Android builds mirror to ENGINE/CRASH in logcat.\n";

        InstallTerminateHandler();
        InstallSignalHandlers();

        factory = std::make_unique<GameObjectFactory>();
        RegisterComponent(TransformComponent);
        RegisterComponent(RenderComponent);
        RegisterComponent(CircleRenderComponent);
        RegisterComponent(GlowComponent);
        RegisterComponent(SpriteComponent);
        RegisterComponent(ShadowComponent);
        RegisterComponent(RigidBodyComponent);
        RegisterComponent(PlayerComponent);
        RegisterComponent(PlayerAttackComponent);
        RegisterComponent(PlayerHealthComponent);
        RegisterComponent(HitBoxComponent);
        RegisterComponent(SpriteAnimationComponent);
        RegisterComponent(EnemyComponent);
        RegisterComponent(EnemyAttackComponent);
        RegisterComponent(EnemyDecisionTreeComponent);
        RegisterComponent(BehaviorTreeComponent);
        RegisterComponent(EnemyHealthComponent);
        RegisterComponent(EnemyTypeComponent);
        RegisterComponent(AudioComponent);
        RegisterComponent(ZoomTriggerComponent);
        RegisterComponent(GateTargetComponent);
        RegisterComponent(BehaviourComponent);
        RegisterComponent(PlayerHUDComponent);
        FACTORY = factory.get();
        gateController.SetFactory(factory.get());
        LoadPrefabs();

        auto playerPrefab = resolveData("player.json");
        std::cout << "[Prefab] Player path = " << std::filesystem::absolute(playerPrefab)
            << "  exists=" << std::filesystem::exists(playerPrefab) << "\n";
        std::filesystem::path startLevelPath = resolveData("level_RealTutorial.json");

        levelObjects = factory->CreateLevel(startLevelPath.string());

        const bool hasAnimatedStore = std::any_of(levelObjects.begin(), levelObjects.end(), [](Framework::GOC* obj) {
            if (!obj)
                return false;

            const std::string& name = obj->GetObjectName();
            return name == "HawkerStoreAnimated" || name == "Hawker_Store_Animated";
            });

        if (!hasAnimatedStore)
        {
            auto storePath = resolveData("hawker_store_animated.json");
            if (!std::filesystem::exists(storePath))
            {
                std::cerr << "[Prefab] Missing hawker_store_animated.json at " << storePath << "\n";
            }
            else if (auto* store = factory->Create(storePath.string()))
            {
                if (auto* tr = store->GetComponentType<Framework::TransformComponent>(
                    Framework::ComponentTypeId::CT_TransformComponent))
                {
                    tr->x = -0.5f;
                    tr->y = -0.9f;
                    tr->rot = 0.0f;
                }

                if (auto* render = store->GetComponentType<Framework::RenderComponent>(
                    Framework::ComponentTypeId::CT_RenderComponent))
                {
                    render->visible = true;
                }

                levelObjects.push_back(store);
            }
            else
            {
                std::cerr << "[Prefab] Failed to spawn Hawker_Store_Animated from " << storePath << "\n";
            }
        }
        RefreshLevelReferences();

        WindowConfig cfg = LoadWindowConfig(resolveData("window.json").string());
        screenW = cfg.width;
        screenH = cfg.height;

        // Build HitBoxSystem after references are valid.
        if (hitBoxSystem)
        {
            hitBoxSystem->Shutdown();
            delete hitBoxSystem;
            hitBoxSystem = nullptr;
        }

        // Build HitBoxSystem after references are valid.
        hitBoxSystem = new HitBoxSystem(*this);
        hitBoxSystem->Initialize();

        // ---------------------------------------------------------------------
        // Preload Fire Enemy textures to avoid hitches when they first spawn.
        // ---------------------------------------------------------------------
        Resource_Manager::load(
            "fire_idle",
            Framework::ResolveAssetPath("Textures/Character/Fire Enemy_Sprite/Idle_Sprite.png").string()
        );
        Resource_Manager::load(
            "fire_attack",
            Framework::ResolveAssetPath("Textures/Character/Fire Enemy_Sprite/Fire Attack_Sprite.png").string()
        );
        Resource_Manager::load(
            "fire_projectile",
            Framework::ResolveAssetPath("Textures/Character/Fire Enemy_Sprite/FireProjectileSprite.png").string()
        );
        Resource_Manager::load(
            "fire_knockback",
            Framework::ResolveAssetPath("Textures/Character/Fire Enemy_Sprite/Knockback_Sprite.png").string()
        );
        Resource_Manager::load(
            "fire_death",
            Framework::ResolveAssetPath("Textures/Character/Fire Enemy_Sprite/Death_Sprite.png").string()
        );

        std::cout << "\n=== Controls ===\n"
            << "WASD: Move | Q/E: Rotate | Z/X: Scale | R: Reset\n"
            << "F1: Toggle Performance Overlay (FPS & timings)\n"
#ifndef NDEBUG
            << "F9: Trigger crash logging test (SIGABRT)\n"
#endif
            << "=======================================\n";
    }

    /*****************************************************************************************
      \brief Per-frame update: input handling, physics intent, animation stepping, hitbox spawn,
             collision AABB bookkeeping, and crash-test handling.
      \param dt Delta time (seconds).
    *****************************************************************************************/
    void LogicSystem::Update(float dt)
    {
        TryGuard::Run([&] {
#ifndef NDEBUG
            bool triggerCrash = input.IsKeyPressed(GLFW_KEY_F9);
            if (triggerCrash && !crashTestLatched) {
                crashTestLatched = true;
                if (g_crashLogger) {
                    auto line = g_crashLogger->WriteWithStack("manual_trigger", "key=F9|stage=pre_abort");
                    g_crashLogger->Mirror(line);
                }
                std::cout << "[CrashLog] Deliberate crash requested via F9.\n";
                std::raise(SIGABRT);
            }
            else if (!triggerCrash) {
                crashTestLatched = false;
            }
#endif
            if (factory)
                factory->Update(dt);

            RefreshLevelReferences();
            DispatchBehaviours(dt);

            if (input.IsKeyReleased(GLFW_KEY_P))
            {
                std::cout << "FPS IS TOGGLED\n";
                if (auto* rs = Framework::RenderSystem::Get())
                    rs->ToggleFPS();
            }

            if (factory)
            {
                for (auto& [id, objPtr] : factory->Objects())
                {
                    (void)id;
                    auto* obj = objPtr.get();
                    if (!obj)
                        continue;

                    auto* anim = obj->GetComponentType<Framework::SpriteAnimationComponent>(
                        Framework::ComponentTypeId::CT_SpriteAnimationComponent);
                    if (!anim || (!anim->HasFrames() && !anim->HasSpriteSheets()))
                        continue;

                    anim->Advance(dt);

                    auto* sprite = obj->GetComponentType<Framework::SpriteComponent>(
                        Framework::ComponentTypeId::CT_SpriteComponent);
                    if (!sprite)
                        continue;

                    if (anim->HasSpriteSheets())
                    {
                        auto sample = anim->CurrentSheetSample();
                        if (!sample.textureKey.empty())
                            sprite->texture_key = sample.textureKey;
                        if (sample.texture)
                            sprite->texture_id = sample.texture;
                    }
                    else
                    {
                        size_t frameIndex = anim->CurrentFrameIndex();
                        if (frameIndex >= anim->frames.size())
                            continue;

                        const auto& frame = anim->frames[frameIndex];
                        sprite->texture_key = frame.texture_key;
                        unsigned tex = anim->ResolveFrameTexture(frameIndex);
                        if (tex)
                            sprite->texture_id = tex;
                    }
                }
            }

            if (hitBoxSystem)
                hitBoxSystem->Update(dt);
        }, "LogicSystem::Update");
    }

    void LogicSystem::LoadLevelAndResetState(const std::filesystem::path& levelPath)
    {
        if (!factory)
            return;

        EndAllBehaviours();

        for (auto const& [id, obj] : factory->Objects())
        {
            (void)id;
            if (obj)
                factory->Destroy(obj.get());
        }
        factory->Update(0.0f);

        {
            const unsigned pagesFreed = Framework::GameObjectPool::Storage().Allocator().FreeEmptyPages();
            if (pagesFreed > 0) {
                std::cout << "[Allocator] FreeEmptyPages trimmed " << pagesFreed
                    << " empty pages after level unload.\n";
            }
        }

        levelObjects = factory->CreateLevel(levelPath.string());

        player = nullptr;
        collisionTarget = nullptr;
        pendingLevelTransition = false;
        animInfo = AnimationInfo{};
        collisionInfo = CollisionInfo{};
        gateController.Reset();
        gateController.SetPlayer(nullptr);

        RefreshLevelReferences();
    }

    /*****************************************************************************************
       \brief Reload the current level (or a default one) and reset cached state.
             - Destroys all live objects, recreates the level, clears cached pointers/state,
               then refreshes references.
    *****************************************************************************************/
    void LogicSystem::ReloadLevel()
    {
        if (!factory)
            return;

        std::filesystem::path levelPath = factory->LastLevelPath();
        if (levelPath.empty())
            levelPath = resolveData("level.json");

        LoadLevelAndResetState(levelPath);
    }

    void LogicSystem::LoadLevel(const std::filesystem::path& levelPath)
    {
        LoadLevelAndResetState(levelPath);
    }

    /*****************************************************************************************
      \brief Shutdown and release owned systems/resources.
             - Clears references, shuts down factory and unloads prefabs.
             - Tears down crash logger and HitBoxSystem.
    *****************************************************************************************/
    void LogicSystem::Shutdown()
    {
        EndAllBehaviours();
        levelObjects.clear();
        collisionTarget = nullptr;
        player = nullptr;
        gateController.Reset();
        gateController.SetFactory(nullptr);

        if (factory) {
            factory->Shutdown();
            factory.reset();
        }
        UnloadPrefabs();

        if (crashLogger)
        {
            g_crashLogger = nullptr;
            crashLogger.reset();
        }

        if (hitBoxSystem)
        {
            hitBoxSystem->Shutdown();
            delete hitBoxSystem;
            hitBoxSystem = nullptr;
        }
    }


    void LogicSystem::RegisterBehaviour(const std::string& key, BehaviourFCT fct)
    {
        if (key.empty())
            return;

        behaviours[key] = fct;
    }

    void LogicSystem::DispatchBehaviours(float dt)
    {
        // Iterate levelObjects but always ensure the pointer is still alive in the factory.
        // Do NOT access internal GameObjectComposition members (like ObjectId) directly here
        // because they may be non-public; use IsAlive() / pointer identity checks instead.
        for (auto* rawObjPtr : levelObjects)
        {
            // quick null check
            if (!rawObjPtr)
                continue;

            // If we have a factory, ensure this pointer still refers to a live object the factory owns.
            // Use IsAlive(rawObjPtr) which compares pointer identity against factory->Objects().
            if (factory && !IsAlive(rawObjPtr))
                continue; // object no longer exists (was destroyed), skip safely

            // rawObjPtr is a live pointer now; use it directly.
            Framework::GameObjectComposition* obj = rawObjPtr;

            // Now use `obj` (a live pointer) for component access.
            auto* behaviour = obj->GetComponentType<BehaviourComponent>(ComponentTypeId::CT_BehaviourComponent);
            if (!behaviour || behaviour->behaviourKey.empty())
                continue;

            auto it = behaviours.find(behaviour->behaviourKey);
            if (it == behaviours.end())
                continue;

            BehaviourFCT& fct = it->second;
            if (!behaviour->started)
            {
                if (fct.Init)
                    fct.Init(obj);
                behaviour->started = true;
            }

            if (fct.Update)
                fct.Update(obj, dt);
        }
    }

    void LogicSystem::EndAllBehaviours()
    {
        for (auto* obj : levelObjects)
        {
            if (!obj)
                continue;

            auto* behaviour = obj->GetComponentType<BehaviourComponent>(ComponentTypeId::CT_BehaviourComponent);
            if (!behaviour || !behaviour->started || behaviour->behaviourKey.empty())
                continue;

            auto it = behaviours.find(behaviour->behaviourKey);
            if (it != behaviours.end() && it->second.End)
                it->second.End(obj);

            behaviour->started = false;
        }
    }
} // namespace Framework
