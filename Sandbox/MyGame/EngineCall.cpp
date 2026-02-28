#include "EngineCall.hpp"

#include "Systems/LogicSystem.h"

namespace mygame {
    void BindBehaviourContext(Framework::LogicSystem& logic);
    void RegisterGameBehaviourFunctions(Framework::LogicSystem& logic);

    void RegisterMyGameScripts(Framework::LogicSystem& logic)
    {
        BindBehaviourContext(logic);
        RegisterGameBehaviourFunctions(logic);
    }
}
