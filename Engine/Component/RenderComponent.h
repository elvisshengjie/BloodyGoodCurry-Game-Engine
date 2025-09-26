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

        std::unique_ptr<GameComponent>Clone() const override {
            // Create new CircleRenderComponent on heap
            // Wrap inside unique_ptr so it is automatically clean up if something goes wrong
            auto copy = std::make_unique<RenderComponent>();
            //copy the values
            copy->w = w;
            copy->h = h;
            copy->r = r;
            copy->g = g;
            copy->b = b;
            copy->a = a;
        
            //Transfer ownership to whoever call clone()
            return copy;

        }
    };
}
