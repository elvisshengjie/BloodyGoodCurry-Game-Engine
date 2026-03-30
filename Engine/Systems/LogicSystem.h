/*********************************************************************************************
 \file      LogicSystem.h
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Author, 10%
            yimo kong (yimo.kong@digipen.edu)      - Author, 10%
 \brief     Game logic coordinator: level load/refresh, input-driven updates, animation.
 \details   Owns the GameObjectFactory and level objects, advances player/enemy state each
            frame, updates sprite animation, exposes simple collision snapshots, and
            bridges to HitBoxSystem.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include "Factory/Factory.h"
#include "Composition/PrefabManager.h"
#include "Component/BehaviourComponent.h"
#include <Serialization/JsonSerialization.h>
#include "InputSystem.h"
#include "Config/WindowConfig.h"
#include "Debug/CrashLogger.hpp"
#include "Graphics/Window.hpp"
#include "Physics/Collision/Collision.h"
#include "Component/HitBoxComponent.h"
#include "Logic/BehaviourFCT.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gfx { class Window; }
class CrashLogger;

namespace Framework {

    class InputsSyetm;   //!< (kept to match existing fwd-decl)
    class GameObjectFactory;
    class HitBoxSystem;

    /*!
     * \class  LogicSystem
     * \brief  Central gameplay loop driver for level objects and player/enemy logic.
     * \note   Integrates input, animation ticking, simple collision exports, and level I/O.
     */
    class LogicSystem : public Framework::ISystem {
    public:
        /*! \brief Lightweight snapshot of current sprite animation state. */
        struct AnimationInfo {
            enum class Mode { Idle, Run, Attack1, Attack2, Attack3, Throw, Knockback, Death };

            int  frame{ 0 };
            int  columns{ 1 };
            int  rows{ 1 };
            Mode mode{ Mode::Idle };
            bool running{ false };
        };

        /*! \brief Minimal AABB snapshot for debugging/render integration. */
        struct CollisionInfo {
            bool playerValid{ false };
            bool targetValid{ false };
            AABB player{ 0.f,0.f,0.f,0.f };
            AABB target{ 0.f,0.f,0.f,0.f };
        };

        /*! \brief Construct with a target window and input system. */
        LogicSystem(gfx::Window& window, InputSystem& input);
        /*! \brief Destroy owned subsystems and logging resources. */
        ~LogicSystem() override;

        /*! \brief Set up factory, load level, cache references. */
        void Initialize() override;

        /*! \brief Per-frame gameplay update (input, animation, bookkeeping). */
        void Update(float dt) override;

        /*! \brief Release level/factory resources. */
        void Shutdown() override;

        /*! \brief Reload current level and refresh object references. */
        void ReloadLevel();
        void LoadLevel(const std::filesystem::path& levelPath);

        /*! \brief Destroy the current world so the next level can be built incrementally over multiple frames. */
        void PrepareForIncrementalLevelLoad();

        /*! \brief Apply the same post-load restoration and cache refresh used by the normal synchronous load path. */
        void FinalizeIncrementalLevelLoad(const std::filesystem::path& levelPath,
            const std::string& levelName,
            const std::vector<GOC*>& loadedObjects);
        void RegisterBehaviour(const std::string& key, BehaviourFCT fct);
        void SetStartupLevelPath(std::filesystem::path levelPath) { startupLevelPath = std::move(levelPath); }
        void SetPostLevelLoadCallback(std::function<void(LogicSystem&)> callback) { postLevelLoadCallback = std::move(callback); }
        /*****************************************************************************************
         \brief Registers a game-layer callback that runs at the end of LogicSystem::Update().
         \param callback Callback invoked once per logic tick with delta time in seconds.
         \details
            - Runs after factory updates, behaviour dispatch, and animation advancement.
            - Lets the game attach project-specific runtime work without hardcoding that work
              into the engine's LogicSystem.
            - Used by BloodyGoodCurry to update its game-owned HitBoxSystem at the same
              point in the frame where the engine used to update it directly.
         \note
            If no callback is set, LogicSystem simply skips this hook.
        *****************************************************************************************/
        void SetPostUpdateCallback(std::function<void(float)> callback)
        {
            postUpdateCallback = std::move(callback);
        }
        /*****************************************************************************************
         \brief Registers a game-layer hook that runs after the factory is created.
         \param callback Callback invoked with the active GameObjectFactory.
         \details
            - Runs during Initialize() after the engine registers engine-owned components.
            - Lets the current game register game-only components without moving that
              registration code into the engine target.
            - Intended for project-specific ECS types such as BloodyGoodCurry's HUD component.
         \note
            If no callback is set, factory initialization proceeds normally with engine-only
            component registration.
        *****************************************************************************************/
        void SetFactorySetupCallback(std::function<void(GameObjectFactory&)> callback)
        {
            factorySetupCallback = std::move(callback);
        }
        /*****************************************************************************************
         \brief Registers the game-provided player-discovery policy.
         \param callback Callback that returns the current player object for this game.
         \details
            - Used by LogicSystem::FindAnyAlivePlayer() after engine-side assumptions about
              player components were removed.
            - Lets each game decide which component or tag identifies its player entity.
        *****************************************************************************************/
        void SetFindPlayerCallback(std::function<GOC*(LogicSystem&)> callback)
        {
            findPlayerCallback = std::move(callback);
        }
        /*****************************************************************************************
         \brief Registers a game-layer hook for restoring per-game audio state after level load.
         \param callback Callback invoked with the freshly loaded level objects.
         \details
            - Runs immediately after CreateLevel() returns inside the engine load path.
            - Lets the active game restore prefab-driven audio or other project-specific
              post-processing without hardcoding that logic into the engine.
        *****************************************************************************************/
        void SetPostAudioRestoreCallback(
            std::function<void(LogicSystem&, const std::vector<GOC*>&)> callback)
        {
            postAudioRestoreCallback = std::move(callback);
        }
        std::filesystem::path ResolveDataPath(std::string_view name) const { return resolveData(name); }
        bool HasLevelObjectNamed(std::string_view name) const;
        void AddLevelObject(GOC* obj);

        /*! \name Accessors */
        ///@{
        GameObjectFactory* Factory()       const { return factory.get(); }
        const std::vector<GOC*>& LevelObjects()  const { return levelObjects; }
        const AnimationInfo& Animation()     const { return animInfo; }
        const CollisionInfo& Collision()     const { return collisionInfo; }
        InputSystem&                Input()         { return input; }
        const InputSystem&          Input()   const { return input; }
        bool                       GetPlayerWorldPosition(float& outX, float& outY) const;
        int                        ScreenWidth()   const { return screenW; }
        int                        ScreenHeight()  const { return screenH; }
        GOC* FindAnyAlivePlayer();
        HitBoxSystem* hitBoxSystem = nullptr;
        ///@}

        /*! \brief System name for diagnostics/profiling. */
        std::string GetName() override { return "LogicSystem"; }

    private:

        std::filesystem::path resolveData(std::string_view name) const;

        bool                 IsAlive(GOC* obj) const;
        void                 RefreshLevelReferences();
        void                 LoadLevelAndResetState(const std::filesystem::path& levelPath);
        void                 DispatchBehaviours(float dt);
        void                 EndAllBehaviours();

        gfx::Window* window;
        InputSystem& input;

        std::unique_ptr<GameObjectFactory>   factory;
        std::vector<GOC*>                    levelObjects;

        GOC* player{ nullptr };
        GOC* collisionTarget{ nullptr };

        AnimationInfo                        animInfo{};
        CollisionInfo                        collisionInfo{};

        int                                  screenW{ 800 };
        int                                  screenH{ 600 };

        bool                                 crashTestLatched{ false };
        bool                                 hangTestLatched{ false };
        std::unique_ptr<CrashLogger>         crashLogger;
        std::filesystem::path                startupLevelPath;
        std::function<void(LogicSystem&)>    postLevelLoadCallback;
        std::function<void(float)>           postUpdateCallback;
        std::function<void(GameObjectFactory&)> factorySetupCallback;
        std::function<GOC*(LogicSystem&)>    findPlayerCallback;
        std::function<void(LogicSystem&, const std::vector<GOC*>&)> postAudioRestoreCallback;

        std::unordered_map<std::string, BehaviourFCT> behaviours;
    };

} // namespace Framework
