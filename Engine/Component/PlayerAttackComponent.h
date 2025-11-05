#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include "Component/HitBoxComponent.h"
#include "Component/TransformComponent.h"
#include <memory>
#include <iostream>
namespace Framework
{
    
    class PlayerAttackComponent : public GameComponent 
    {
        public:
            int damage{50};
            float attack_speed{1.0f};
            std::unique_ptr<HitBoxComponent> hitbox;
            PlayerAttackComponent() = default;
            PlayerAttackComponent(int dmg, float spd) : damage(dmg), attack_speed(spd) {}
            void initialize() override { std::cout << "This object has a PlayerAttackComponent!\n"; }
            void SendMessage(Message& m) override { (void)m; }
            void Serialize(ISerializer& s) override 
            {
                if (s.HasKey("damage")) StreamRead(s,"damage", damage);
                if (s.HasKey("attack_speed")) StreamRead(s,"attack_speed", attack_speed);
            }
            std::unique_ptr<GameComponent> Clone() const override 
            {
               auto copy = std::make_unique<PlayerAttackComponent>();
               copy->damage= damage;
               copy->attack_speed= attack_speed;
               return copy;
            }
            void PerformAttack(TransformComponent* playerTr)
            {

                if (!playerTr) return;
                if (!hitbox) hitbox = std::make_unique<HitBoxComponent>();
                hitbox->spawnX = playerTr->x + 50;
                hitbox->spawnY = playerTr->y;
                hitbox->width = 50;
                hitbox->height = 50;
                hitbox->damage = static_cast<float>(damage);
                hitbox->duration = 0.2f;
                hitbox->ActivateHurtBox();
            }
            void Update(float dt, TransformComponent* tr)
            {
                (void)tr;
                if (hitbox && hitbox->active)
                {
                    hitbox->duration -= dt;
                    if (hitbox->duration <= 0.0f)
                    {
                        hitbox->DeactivateHurtBox();
                        hitbox->duration = 0.2f; // reset for next attack
                    }
                }
            }
    };
}