/*********************************************************************************************
 \file      ShadowComponent.h
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu)
 \brief     Declares the ShadowComponent class, a helper component that configures
            sprite-flip shadows rendered under a sprite.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include "Composition/Component.h"
#include "Memory/ComponentPool.h"
#include "Serialization/Serialization.h"
#include "Component/RenderComponent.h"
#include <iostream>

namespace Framework {
    /*****************************************************************************************
      \class ShadowComponent
      \brief Stores settings for rendering a sprite-flip drop shadow.

      The shadow uses the same texture/animation frame as the sprite, but renders
      with a squashed, vertically flipped transform and a dark multiply tint.
    *****************************************************************************************/
    class ShadowComponent : public GameComponent {
    public:
        bool enabled{ true };

        float offsetX{ 0.0f };
        float offsetY{ -0.12f };

        float scaleX{ 1.05f };
        float scaleY{ 0.45f };
        bool flipY{ true };

        float r{ 0.2f };
        float g{ 0.2f };
        float b{ 0.2f };
        float a{ 0.7f };

        BlendMode blendMode{ BlendMode::Multiply };

        void initialize() override {}

        void Serialize(ISerializer& s) override {
            if (s.HasKey("enabled")) {
                int enabledInt = static_cast<int>(enabled);
                StreamRead(s, "enabled", enabledInt);
                enabled = (enabledInt != 0);
            }
            if (s.HasKey("offset_x")) StreamRead(s, "offset_x", offsetX);
            if (s.HasKey("offset_y")) StreamRead(s, "offset_y", offsetY);
            if (s.HasKey("scale_x")) StreamRead(s, "scale_x", scaleX);
            if (s.HasKey("scale_y")) StreamRead(s, "scale_y", scaleY);
            if (s.HasKey("flip_y")) {
                int flipInt = static_cast<int>(flipY);
                StreamRead(s, "flip_y", flipInt);
                flipY = (flipInt != 0);
            }
            if (s.HasKey("r")) StreamRead(s, "r", r);
            if (s.HasKey("g")) StreamRead(s, "g", g);
            if (s.HasKey("b")) StreamRead(s, "b", b);
            if (s.HasKey("a")) StreamRead(s, "a", a);

            if (s.HasKey("blend_mode")) {
                std::string modeValue;
                StreamRead(s, "blend_mode", modeValue);
                BlendMode parsedMode = BlendMode::Multiply;
                if (!TryParseBlendMode(modeValue, parsedMode)) {
                    std::cerr << "[ShadowComponent] Unknown blend_mode '" << modeValue
                        << "', defaulting to Multiply.\n";
                }
                blendMode = parsedMode;
            }
        }

        ComponentHandle Clone() const override {
            auto copy = ComponentPool<ShadowComponent>::CreateTyped();
            copy->enabled = enabled;
            copy->offsetX = offsetX;
            copy->offsetY = offsetY;
            copy->scaleX = scaleX;
            copy->scaleY = scaleY;
            copy->flipY = flipY;
            copy->r = r;
            copy->g = g;
            copy->b = b;
            copy->a = a;
            copy->blendMode = blendMode;
            return copy;
        }

        void SendMessage(Message&) override {}
    };
}
