/*********************************************************************************************
 \file      VfxPresets.hpp
 \par       SofaSpuds
 \author
 \brief     Declares sandbox-specific combat VFX preset bindings.
 \details   Exposes helpers that bind and identify the current game's hit-impact VFX
            while keeping the engine combat systems generic.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

#include "Composition/Composition.h"
#include "Systems/HitBoxSystem.h"

namespace Framework {
    class LogicSystem;
}

namespace mygame {
    void BindCombatVfx(Framework::LogicSystem& logic);

    bool IsImpactVfxObject(const Framework::GOC* obj);
}
