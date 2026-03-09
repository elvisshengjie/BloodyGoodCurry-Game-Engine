/*********************************************************************************************
 \file      EngineCall.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Defines the generated game project's behaviour-registration entry point.
 \details   The default template keeps this empty so new projects start from a minimal
            engine integration and can add gameplay behaviour bindings incrementally.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "EngineCall.hpp"
#include "Systems/LogicSystem.h"

namespace mygame
{
    /*************************************************************************************
     \brief  Register generated-project behaviour bindings with the active logic system.
     \param  logic  Active engine LogicSystem.
     \details The template starts with no custom behaviours, so this is a no-op hook.
    *************************************************************************************/
    void RegisterMyGameScripts(Framework::LogicSystem& logic)
    {
        (void)logic;
    }
}
