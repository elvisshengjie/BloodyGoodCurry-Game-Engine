#pragma once
#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"

namespace Framework {
    class CircleRenderComponent : public GameComponent {
    public:
        float radius{ 0.10f };
        float r{ 1.f }, g{ 1.f }, b{ 1.f }, a{ 1.f };

        void initialize() override {}
        void SendMessage(Message& m) override { (void)m; }

        void Serialize(ISerializer& s) override {
            if (s.HasKey("radius")) StreamRead(s, "radius", radius);
            if (s.HasKey("r"))      StreamRead(s, "r", r);
            if (s.HasKey("g"))      StreamRead(s, "g", g);
            if (s.HasKey("b"))      StreamRead(s, "b", b);
            if (s.HasKey("a"))      StreamRead(s, "a", a);
        }
        std::unique_ptr<GameComponent>Clone() const override {
            // Create new CircleRenderComponent on heap
            // Wrap inside unique_ptr so it is automatically clean up if something goes wrong
            auto copy = std::make_unique<CircleRenderComponent>();
            //copy the values 
            copy->radius = radius;
            copy->r = r;
            copy->g = g;
            copy->b = b;
            copy->a = a;
            //Transfer ownership to whoever call clone()
            return copy;

        }
    };

    
}
