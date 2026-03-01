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
}