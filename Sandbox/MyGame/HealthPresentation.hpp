/*********************************************************************************************
 \file      HealthPresentation.hpp
 \par       SofaSpuds
 \author
 \brief     Declares sandbox-specific health UI and defeat presentation helpers.
 \details   Exposes the game-layer bindings and draw helpers that sit on top of the
            engine HealthSystem for UI and defeat-screen flow.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

namespace Framework {
    class HealthSystem;
    class RenderSystem;
}

namespace mygame {

    void BindHealthPresentation(Framework::HealthSystem& health);
    void UpdateHealthPresentationDelta(float dt);
    bool IsPlayerDefeated();
    void ResetPlayerDefeat();
    void DrawHealthPresentation(Framework::RenderSystem& render);

} // namespace mygame
