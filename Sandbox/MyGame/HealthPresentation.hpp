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
