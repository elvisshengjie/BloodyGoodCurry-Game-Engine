/*********************************************************************************************
 \file      EngineCall.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Defines the entry-point hook used by the sandbox/game layer to register all
            gameplay behaviours/scripts into the engine's LogicSystem.

 \details
            RegisterMyGameScripts() is called by the engine to allow the game project
            (namespace mygame) to:
            1) Bind any per-game behaviour context needed by scripts (e.g., input access,
               shared state pointers, factories, etc.).
            2) Register the behaviour function tables/callbacks so objects with
               BehaviourComponent.behaviourKey can be dispatched correctly at runtime.

            The actual implementation details are delegated to:
            - BindBehaviourContext(logic)
            - RegisterGameBehaviourFunctions(logic)

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "EngineCall.hpp"

#include "Audio/GameAudioSetup.h"
#include "Composition/Composition.h"
#include "Component/AudioComponent.h"
#include "Component/TransformComponent.h"
#include "Runtime/HealthSystem.h"
#include "Systems/CombatAudioEvents.h"
#include "Systems/HitBoxSystem.h"
#include "Systems/LogicSystem.h"

namespace mygame {
    /*************************************************************************************
      \brief Binds game-specific context required by behaviours to the engine LogicSystem.
      \param logic Reference to the engine LogicSystem to receive context bindings.

      \details
      This function is implemented in the game layer and typically installs pointers/
      accessors that scripts use during Init/Update/End (e.g., input, factories, globals).
    **************************************************************************************/
    void BindBehaviourContext(Framework::LogicSystem& logic);

    /*************************************************************************************
      \brief Registers all game behaviour callback tables/functions with the LogicSystem.
      \param logic Reference to the engine LogicSystem to register behaviour functions into.

      \details
      Behaviour keys (e.g., "PlayerController") are mapped to behaviour lifecycle callbacks
      so the engine can dispatch behaviours for objects that own BehaviourComponent.
    **************************************************************************************/
    void RegisterGameBehaviourFunctions(Framework::LogicSystem& logic);

    /*************************************************************************************
      \brief Engine-facing registration function for this game's scripts/behaviours.
      \param logic Reference to the engine LogicSystem used for behaviour orchestration.

      \details
      Called by the engine at startup (or on reload) to ensure all behaviour context and
      behaviour functions are available before the scene begins simulation.
    **************************************************************************************/
    void RegisterMyGameScripts(Framework::LogicSystem& logic)
    {
        BindBehaviourContext(logic);
        RegisterGameBehaviourFunctions(logic);
    }

    /*************************************************************************************
      \brief Binds this game's combat audio routing into engine combat event hooks.
      \param logic  Reference to the engine LogicSystem that exposes the active hitbox system.
      \param health Reference to the engine HealthSystem that emits health-related events.

      \details
      Installs a shared callback that translates generic combat events into MyGame's
      concrete player/enemy audio responses through the GameAudio facade.
    **************************************************************************************/
    void BindCombatAudio(Framework::LogicSystem& logic, Framework::HealthSystem& health)
    {
        const Framework::CombatAudioCallback callback =
            [](Framework::GameObjectComposition* obj, Framework::CombatAudioEvent event)
        {
            if (!obj)
                return;

            auto* audio = obj->GetComponentType<Framework::AudioComponent>(
                Framework::ComponentTypeId::CT_AudioComponent);
            if (!audio)
                return;

            float posX = 0.0f;
            float posY = 0.0f;
            if (auto* transform = obj->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent))
            {
                posX = transform->x;
                posY = transform->y;
            }

            switch (event)
            {
            case Framework::CombatAudioEvent::PlayerHurt:
            {
                mygame::GameAudio gameAudio(audio, mygame::GameAudio::Entity::Player);
                gameAudio.PlayHurt();
                break;
            }
            case Framework::CombatAudioEvent::PlayerDeath:
            {
                mygame::GameAudio gameAudio(audio, mygame::GameAudio::Entity::Player);
                gameAudio.PlayDeath();
                break;
            }
            case Framework::CombatAudioEvent::EnemyHurt:
            {
                mygame::GameAudio gameAudio(audio, mygame::GameAudio::Entity::Enemy);
                gameAudio.PlayHurt(posX, posY);
                break;
            }
            case Framework::CombatAudioEvent::EnemyDeath:
            {
                mygame::GameAudio gameAudio(audio, mygame::GameAudio::Entity::Enemy);
                gameAudio.PlayDeath(posX, posY);
                break;
            }
            case Framework::CombatAudioEvent::PlayerAttackHit:
            {
                mygame::GameAudio gameAudio(audio, mygame::GameAudio::Entity::Player);
                gameAudio.PlayAttack();
                break;
            }
            case Framework::CombatAudioEvent::PlayerAttackBlocked:
            {
                mygame::GameAudio gameAudio(audio, mygame::GameAudio::Entity::Player);
                gameAudio.PlayBoink();
                break;
            }
            case Framework::CombatAudioEvent::PlayerAttackMiss:
            {
                mygame::GameAudio gameAudio(audio, mygame::GameAudio::Entity::Player);
                gameAudio.PlayPunch();
                break;
            }
            }
        };

        if (logic.hitBoxSystem)
            logic.hitBoxSystem->SetCombatAudioCallback(callback);

        health.SetCombatAudioCallback(callback);
    }
}
