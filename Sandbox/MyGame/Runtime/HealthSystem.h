/*********************************************************************************************
 \file      HealthSystem.h
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Secondary Author, 40%
            yimo.kong (yimo.kong@digipen.edu) - Secondary Author, 40%
            elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - co Author, 20%
 \brief     Implements the HealthSystem responsible for managing player and enemy health,
            handling death timers, triggering death animations, and destroying objects at
            the correct time.
 \details   Responsibilities:
            - Tracks all GameObjectComposition instances that contain health components.
            - Handles enemy death: triggers death animation (if available), waits for both
              animation completion and a minimum timer before destruction.
            - Handles player death: plays death animation, enforces invulnerability timers,
              and destroys the player only after animation + timer finish.
            - Uses stable IDs instead of raw pointers to avoid dangling references.
            - Fully integrates with SpriteAnimationComponent for frame-based animation logic.
 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include "Common/System.h"
#include "Factory/Factory.h"
#include "Graphics/Window.hpp"
#include "Serialization/Serialization.h"
#include "Component/RenderComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/PlayerHealthComponent.h"
#include "Components/EnemyHealthComponent.h"
#include "Component/SpriteAnimationComponent.h"
#include "Component/AudioComponent.h"
#include "Systems/CombatAudioEvents.h"
#include <functional>
#include <unordered_map>


namespace Framework {

    struct PlayerInvulnerabilityRenderState
    {
        float r{ 1.0f };
        float g{ 1.0f };
        float b{ 1.0f };
        float a{ 1.0f };
        BlendMode blendMode{ BlendMode::Alpha };
    };

    class HealthSystem : public Framework::ISystem {
    public:
        // Bind to the active window for resolution/DPI�aware logic.
        explicit HealthSystem(gfx::Window& window);

        // Allocate resources, discover/register enemy entities, and prime state.
        void Initialize() override;

        // Step enemy AI/state for the current frame.
        void Update(float dt) override;

      
        void draw() override;

        // Release resources and clear internal caches.
        void Shutdown() override;

        // System name for diagnostics and registries.
        std::string GetName() override { return "HealthSystem"; }
        void RefreshTrackedObjects();

        // Bind a game-side callback for combat audio routing.
        void SetCombatAudioCallback(CombatAudioCallback callback) { combatAudioCallback = callback; }
        void SetPlayerDeathCompleteCallback(std::function<void()> callback)
        {
            playerDeathCompleteCallback = std::move(callback);
        }
        void SetEnemyDeathCompleteCallback(std::function<void(GOC*)> callback)
        {
            enemyDeathCompleteCallback = std::move(callback);
        }


    private:
        gfx::Window* window;          // Non-owning window handle used by the system.
        std::vector<GOCId> gameObjectIds;
        std::unordered_map<GOCId, float> deathTimers;
        std::unordered_map<GOCId, PlayerInvulnerabilityRenderState> playerInvulnerabilityRenderStates;

        CombatAudioCallback combatAudioCallback;
        std::function<void()> playerDeathCompleteCallback;
        std::function<void(GOC*)> enemyDeathCompleteCallback;
    };

} // namespace Framework

