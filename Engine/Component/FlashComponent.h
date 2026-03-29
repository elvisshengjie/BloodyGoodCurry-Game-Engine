#pragma once

#include "Composition/Component.h"
#include "Component/RenderComponent.h"
#include "Memory/ComponentPool.h"
#include "Serialization/Serialization.h"

namespace Framework
{
    class FlashComponent : public GameComponent
    {
    public:
        float frequency{ 8.0f };
        float duration{ 0.0f };
        bool start_visible{ true };
        bool activate_on_enemy_clear{ true };
        bool hide_until_activated{ false };

        // Runtime state.
        float timer{ 0.0f };
        bool visible{ true };
        bool flashing{ false };
        bool completed{ false };
        bool hasCachedRenderState{ false };
        float cachedR{ 1.0f };
        float cachedG{ 1.0f };
        float cachedB{ 1.0f };
        float cachedA{ 1.0f };
        bool cachedVisible{ true };
        BlendMode cachedBlendMode{ BlendMode::Alpha };

        void initialize() override
        {
            timer = 0.0f;
            visible = start_visible;
            flashing = false;
            completed = false;
            hasCachedRenderState = false;
        }

        void SendMessage(Message& m) override { (void)m; }

        void Serialize(ISerializer& s) override
        {
            if (s.HasKey("frequency"))
                StreamRead(s, "frequency", frequency);
            if (s.HasKey("duration"))
                StreamRead(s, "duration", duration);
            if (s.HasKey("start_visible"))
                StreamRead(s, "start_visible", start_visible);
            if (s.HasKey("activate_on_enemy_clear"))
                StreamRead(s, "activate_on_enemy_clear", activate_on_enemy_clear);
            if (s.HasKey("hide_until_activated"))
                StreamRead(s, "hide_until_activated", hide_until_activated);
        }

        ComponentHandle Clone() const override
        {
            auto copy = ComponentPool<FlashComponent>::CreateTyped();
            copy->frequency = frequency;
            copy->duration = duration;
            copy->start_visible = start_visible;
            copy->activate_on_enemy_clear = activate_on_enemy_clear;
            copy->hide_until_activated = hide_until_activated;
            copy->timer = 0.0f;
            copy->visible = start_visible;
            copy->flashing = false;
            copy->completed = false;
            copy->hasCachedRenderState = false;
            copy->cachedR = 1.0f;
            copy->cachedG = 1.0f;
            copy->cachedB = 1.0f;
            copy->cachedA = 1.0f;
            copy->cachedVisible = true;
            copy->cachedBlendMode = BlendMode::Alpha;
            return copy;
        }
    };
}
