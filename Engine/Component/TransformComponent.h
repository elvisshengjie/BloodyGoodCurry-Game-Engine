#pragma once
#include "Composition/Component.h"
#include <iostream>

namespace Framework {

    class TransformComponent : public GameComponent {
    public:
        float x{ 0.0f }, y{ 0.0f }, rot{ 0.0f };

        void initialize() override {
            std::cout << "[TransformComponent] init: x=" << x << " y=" << y << " rot=" << rot << "\n";
        }

        void SendMessage(Message& m) override {
            (void)m; // nothing for now
        }

        void Serialize(ISerializer& s) override {
            // We are already inside { "TransformComponent": { ... } } thanks to the factory
            if (s.HasKey("x"))   StreamRead(s, "x", x);
            if (s.HasKey("y"))   StreamRead(s, "y", y);
            if (s.HasKey("rot")) StreamRead(s, "rot", rot);
        }
    };

} // namespace Framework
