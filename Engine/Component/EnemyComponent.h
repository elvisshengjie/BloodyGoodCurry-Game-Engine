#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
namespace Framework
{
    //A data container by itself (Does not do anything)
    class EnemyComponent : public GameComponent {
    public:
        int health{ 100 };
        float speed{ 1.0f };
        int damage{ 10 };

        void initialize() override {}

        void SendMessage(Message& m) override { (void)m; }

        void Serialize(ISerializer& s) override {
            if (s.HasKey("health")) StreamRead(s, "health", health);
            if (s.HasKey("speed"))  StreamRead(s, "speed", speed);
            if (s.HasKey("damage")) StreamRead(s, "damage", damage);
        }
        std::unique_ptr<GameComponent> Clone() const override {
            auto copy = std::make_unique<EnemyComponent>();
            copy->health = health;
            copy->speed = speed;
            copy->damage = damage;
            return copy;
        }
    };
}