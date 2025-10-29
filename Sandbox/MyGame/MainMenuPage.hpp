#pragma once
#include "Systems/InputSystem.h"
#include "Systems/RenderSystem.h"

// Very small helper for menu layout
struct RectF { float x{}, y{}, w{}, h{}; };

class MainMenuPage {
public:
    void Init(int screenW, int screenH);
    void Update(Framework::InputSystem* input);
    void Draw(Framework::RenderSystem* render);

    bool ConsumeStart();
    bool ConsumeExit();

    // Optional: expose for tests
    int ScreenW() const { return sw; }
    int ScreenH() const { return sh; }

private:
    static bool Contains(const RectF& r, double mx, double my);

    int  sw{ 1280 }, sh{ 720 };
    RectF startBtn{ 70.f, 110.f, 220.f, 64.f };
    RectF exitBtn{ 70.f, 190.f, 220.f, 64.f };

    bool hoverStart{ false }, hoverExit{ false };
    bool startLatched{ false }, exitLatched{ false };

    unsigned menuBgTex{ 0 }; // GL texture for menu.jpg
};
