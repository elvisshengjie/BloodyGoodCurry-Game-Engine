/*********************************************************************************************
 \file      VfxPresets.hpp
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Primary Author, 100%
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
#include <glm/vec2.hpp>

namespace Framework {
    class LogicSystem;
}

namespace mygame {
    struct HitImpactBurstPreset
    {
        int count{ 7 };
        float speedMin{ 0.08f };
        float speedMax{ 0.22f };
        float lifeMin{ 0.18f };
        float lifeMax{ 0.32f };
        float radiusMin{ 0.02f };
        float radiusMax{ 0.045f };
        float offsetMin{ -0.03f };
        float offsetMax{ 0.03f };
        float endRadiusScale{ 0.25f };
        float red{ 1.0f };
        float green{ 0.68f };
        float blue{ 0.28f };
        float startAlpha{ 0.9f };
        float endAlpha{ 0.0f };
    };

    HitImpactBurstPreset& GetHitImpactBurstPreset();
    void ResetHitImpactBurstPreset();

    void BindCombatVfx(Framework::LogicSystem& logic);
    void SpawnHitImpactPreview(const glm::vec2& worldPos);
    Framework::GOC* SpawnHeiBangAttack2BeamVfx(const Framework::GOC& owner, const glm::vec2& targetPos);
    Framework::GOC* SpawnFireImpactVfx(const glm::vec2& worldPos);

    bool IsImpactVfxObject(const Framework::GOC* obj);
    bool IsHeiBangAttack2BeamVfxObject(const Framework::GOC* obj);
    bool IsFireImpactVfxObject(const Framework::GOC* obj);
}
