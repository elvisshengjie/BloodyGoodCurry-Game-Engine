#pragma once
// MainMenuPage.hpp — simple Start/Exit menu (no GUISystem)

namespace Framework { class InputSystem; class RenderSystem; }

namespace mygame
{
    struct RectF { float x, y, w, h; };

    class MainMenuPage
    {
    public:
        // Call once after window+render are initialized
        void Init(int screenW, int screenH);

        // Per-frame
        void Update(Framework::InputSystem* input);
        void Draw(Framework::RenderSystem* render);

        // One-shot events: return true ONCE (latched then cleared)
        bool ConsumeStart();
        bool ConsumeExit();

    private:
        // Window size (used to flip mouse-Y from top-left to bottom-left)
        int  sw = 1280, sh = 720;

        // Button rects are in render space (origin bottom-left)
        RectF startBtn{ 100.f, 160.f, 260.f, 80.f };
        RectF exitBtn{ 100.f, 260.f, 260.f, 80.f };

        // Hover state
        bool hoverStart = false;
        bool hoverExit = false;

        // Latched click flags (consumed by Game.cpp)
        bool startLatched = false;
        bool exitLatched = false;

        // Helpers
        static bool Contains(const RectF& r, double mx, double my);
    };
}
