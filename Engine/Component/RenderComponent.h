// Component/RenderComponent.hpp
#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"

namespace Framework {
    class RenderComponent : public GameComponent {
    public:
        float w{ 64.f }, h{ 64.f };               // size (treated as scale factors in NDC)
        float r{ 1.f }, g{ 1.f }, b{ 1.f }, a{ 1.f }; // tint color

        void initialize() override {}
        void SendMessage(Message& m) override { (void)m; }

        void Serialize(ISerializer& s) override {
            if (s.HasKey("w")) StreamRead(s, "w", w);
            if (s.HasKey("h")) StreamRead(s, "h", h);
            if (s.HasKey("r")) StreamRead(s, "r", r);
            if (s.HasKey("g")) StreamRead(s, "g", g);
            if (s.HasKey("b")) StreamRead(s, "b", b);
            if (s.HasKey("a")) StreamRead(s, "a", a);
        }
    };
}
