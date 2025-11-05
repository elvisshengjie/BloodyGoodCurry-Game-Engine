#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include <iostream>
#include <algorithm>
namespace Framework
{
    //A data container by itself (Does not do anything)
    class EnemyHealthComponent : public GameComponent 
    {
        public:
            int enemyHealth{100};
            int enemyMaxhealth{ 100 };
            void initialize() override {}
            void SendMessage(Message& m) override { (void)m; }
            void Serialize(ISerializer& s) override 
            {
                if (s.HasKey("enemyHealth")) StreamRead(s, "enemyHealth", enemyHealth);
                if (s.HasKey("enemyMaxhealth")) StreamRead(s, "enemyMaxhealth", enemyMaxhealth);
            }
            std::unique_ptr<GameComponent> Clone() const override 
            {
             auto copy = std::make_unique<EnemyHealthComponent>();
             copy->enemyHealth = enemyHealth;
             copy->enemyMaxhealth = enemyMaxhealth;
             return copy;
            }

            void TakeDamage(int dmg)
            { 
              enemyHealth = std::max(enemyHealth - dmg, 0);
              std::cout << "[EnemyHealthComponent] Took " << dmg << " damage, current health = " << enemyHealth << "\n";
            }

            void Heal(int amount)
            {
                enemyHealth = std::min(enemyHealth + amount, enemyMaxhealth);
                std::cout << "[EnemyHealthComponent] Healed " << amount << ", current health = " << enemyHealth << "\n";
            }
    };
}