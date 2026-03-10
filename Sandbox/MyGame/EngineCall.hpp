/*********************************************************************************************
 \file      EngineCall.hpp
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Declares the game-layer hook function used by the engine to register all
            gameplay scripts/behaviours into the LogicSystem.

 \details
            The engine calls RegisterMyGameScripts() to allow the game project (namespace
            mygame) to bind any required behaviour context and register behaviour callback
            tables/functions. This keeps gameplay logic in the sandbox/game layer while
            the engine remains generic.

 \copyright
            All content Â© 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

#include <filesystem>

namespace Framework {
    class AiSystem;
    class GameObjectComposition;
    class LogicSystem;
    class HealthSystem;
    class RenderSystem;
}

namespace mygame {
    struct AbilityCooldownUiState
    {
        bool ready{ true };
        float remaining{ 0.0f };
        float duration{ 0.0f };
    };

    struct PlayerAbilityHudState
    {
        AbilityCooldownUiState melee{};
        AbilityCooldownUiState ranged{};
        AbilityCooldownUiState talisman{};
    };

    struct PlayerAimIndicatorState
    {
        bool valid{ false };
        float dirX{ 1.0f };
        float dirY{ 0.0f };
    };

    /*************************************************************************************
      \brief Registers all game behaviours/scripts with the engine's LogicSystem.
      \param logic Reference to the engine LogicSystem used for behaviour orchestration.

      \details
      This is the main entry point from engine â†’ game layer for behaviour setup.
      Implementations typically:
      - Bind any required script context into the LogicSystem.
      - Register behaviour keys to their Init/Update/End callbacks.
    **************************************************************************************/
    void RegisterMyGameScripts(Framework::LogicSystem& logic);

    /*************************************************************************************
      \brief Configures game-specific startup content for the active LogicSystem.
      \param logic Active engine LogicSystem prior to Initialize().
    **************************************************************************************/
    void ConfigureGameBootstrap(Framework::LogicSystem& logic);

    /*************************************************************************************
      \brief Configures game-specific render defaults for the active RenderSystem.
      \param render Active engine RenderSystem prior to Initialize().
    **************************************************************************************/
    void ConfigureRenderBootstrap(Framework::RenderSystem& render);

    /*************************************************************************************
      \brief Binds game-side combat audio routing into engine combat systems.
      \param logic   Active engine LogicSystem (provides the current HitBoxSystem hook).
      \param health  Active engine HealthSystem.
    **************************************************************************************/
    void BindCombatAudio(Framework::LogicSystem& logic, Framework::HealthSystem& health);

    /*************************************************************************************
      \brief Binds game-side AI combat spawning into the engine AI system.
      \param ai     Active engine AiSystem.
      \param logic  Active engine LogicSystem (provides the current HitBoxSystem hook).
    **************************************************************************************/
    void BindAiCombat(Framework::AiSystem& ai, Framework::LogicSystem& logic);

    /*************************************************************************************
      \brief Returns current player key inventory count used by key-door gameplay.
    **************************************************************************************/
    int GetPlayerKeyCount();

    /*************************************************************************************
      \brief Returns the current HUD-facing readiness/cooldown state for the player's abilities.
      \param player Player object to query.
    **************************************************************************************/
    PlayerAbilityHudState GetPlayerAbilityHudState(const Framework::GameObjectComposition* player);

    /*************************************************************************************
      \brief Returns the current aim direction used for player-facing presentation.
      \param player Player object to query.
    **************************************************************************************/
    PlayerAimIndicatorState GetPlayerAimIndicatorState(const Framework::GameObjectComposition* player);

    /*************************************************************************************
      \brief Clears player key inventory and key-door runtime unlock state.
    **************************************************************************************/
    void ResetPlayerKeyCount();

    /*************************************************************************************
      \brief Requests an incremental reload of the currently active level.
    **************************************************************************************/
    bool RequestReloadLevel();

    /*************************************************************************************
      \brief Requests an incremental load of the specified level through the game state machine.
    **************************************************************************************/
    bool RequestLoadLevel(const std::filesystem::path& levelPath);

    /*************************************************************************************
      \brief Returns whether gameplay input should be ignored by game-side scripts this frame.
    **************************************************************************************/
    bool IsGameplayInputBlocked();

}
