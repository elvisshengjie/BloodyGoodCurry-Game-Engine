#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include <iostream>
#include <algorithm>
namespace Framework
{
    //A data container by itself (Does not do anything)
    class PlayerHealthComponent : public GameComponent 
    {
        public:
            int playerHealth{100};
            int playerMaxhealth{ 100 };
            void initialize() override { std::cout << "This object has a PlayerHealthComponent!\n"; }
            void SendMessage(Message& m) override { (void)m; }
            void Serialize(ISerializer& s) override 
            {
                if (s.HasKey("playerHealth")) StreamRead(s, "playerHealth", playerHealth);
                if (s.HasKey("playerMaxhealth")) StreamRead(s, "playerMaxhealth", playerMaxhealth);
            }
            std::unique_ptr<GameComponent> Clone() const override 
            {
             auto copy = std::make_unique<PlayerHealthComponent>();
             copy->playerHealth = playerHealth;
             copy->playerMaxhealth = playerMaxhealth;
             return copy;
            }
            // Reduce health
            void TakeDamage(int dmg)
            {playerHealth = std::max(playerHealth - dmg, static_cast<int>(0)); std::cout << "[PlayerHealthComponent] Took " << dmg << " damage"<< "\n";}
            void Heal(int amount)
            {playerHealth = std::min(playerHealth + amount, static_cast<int>(playerMaxhealth)); std::cout << "[PlayerHealthComponent] Healed " << amount << ", current health = " << playerHealth << "\n";}
    };
}