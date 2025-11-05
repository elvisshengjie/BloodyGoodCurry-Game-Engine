/*********************************************************************************************
 \file      PlayerHealthComponent.h
 \par       SofaSpuds
 \author    - Primary Author, 100%

 \brief     Declares the PlayerHealthComponent class, which stores and manages the player�s
            health data. This component defines current and maximum health values used by
            gameplay systems for combat, UI, and respawn logic.

 \details
            The PlayerHealthComponent is a lightweight data container that tracks the
            player�s current and maximum health values. It provides serialization for
            prefab-driven initialization and supports cloning for respawning or prefab
            instancing. Systems such as combat, damage, and UI can reference this data
            to update the player�s health bar or trigger death behavior.

            Responsibilities:
            - Store and manage player health values.
            - Provide serialization for prefab or save data.
            - Support deep-copy functionality.
            - Log initialization for debugging purposes.

 \copyright
            All content � 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include <iostream>
#include <algorithm>
namespace Framework
{
    /*****************************************************************************************
      \class PlayerHealthComponent
      \brief Component that holds the player's current and maximum health values.

      This component acts as a simple data container. It can be queried or modified by
      combat, healing, or UI systems to update gameplay state accordingly.
    *****************************************************************************************/
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
