/*********************************************************************************************
\file      PlayerHealthComponent.h
\par       SofaSpuds
\author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

\brief     Declaration and implementation of the PlayerHealthComponent class. This component
           manages the health of a player entity, including damage and healing operations.

\details
           PlayerHealthComponent provides essential player health management:
           - Tracks current and maximum health.
           - Supports taking damage and healing with clamping to valid ranges.
           - Provides serialization for saving/loading health data.
           - Can be cloned to duplicate health state across entities.

\copyright
           All content © 2025 DigiPen Institute of Technology Singapore.
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
    \brief
    Manages the health state of a player entity.

    \details
    This component stores the current and maximum health of a player and provides functions
    to modify health safely. It also integrates with the serialization system to save/load
    health data.
    *****************************************************************************************/
    class PlayerHealthComponent : public GameComponent 
    {
        public:
            int playerHealth{100};
            int playerMaxhealth{ 100 };
            /*****************************************************************************************
            \brief
            Initializes the component.

            \details
            Prints a debug message indicating that this object has a PlayerHealthComponent.
            *****************************************************************************************/
            void initialize() override { std::cout << "This object has a PlayerHealthComponent!\n"; }
            /*****************************************************************************************
            \brief
            Handles incoming messages.

            \param m
            Message to process.

            \details
            Required override of GameComponent interface. Currently does nothing.
            *****************************************************************************************/
            void SendMessage(Message& m) override { (void)m; }
            /*****************************************************************************************
            \brief
            Serializes or deserializes the component.

            \param s
            Serializer instance.

            \details
            Supports reading playerHealth and playerMaxhealth from a serializer.
            *****************************************************************************************/
            void Serialize(ISerializer& s) override 
            {
                if (s.HasKey("playerHealth")) StreamRead(s, "playerHealth", playerHealth);
                if (s.HasKey("playerMaxhealth")) StreamRead(s, "playerMaxhealth", playerMaxhealth);
            }
            /*****************************************************************************************
            \brief
            Creates a clone of this component.

            \return
            A std::unique_ptr to a new PlayerHealthComponent instance with copied health values.
            *****************************************************************************************/
            std::unique_ptr<GameComponent> Clone() const override 
            {
             auto copy = std::make_unique<PlayerHealthComponent>();
             copy->playerHealth = playerHealth;
             copy->playerMaxhealth = playerMaxhealth;
             return copy;
            }
            /*****************************************************************************************
            \brief
            Reduces the player's health by a specified amount.

            \param dmg
            Amount of damage to apply.

            \details
            Health is clamped to a minimum of zero. Prints a debug message showing damage taken.
            *****************************************************************************************/
            void TakeDamage(int dmg)
            {playerHealth = std::max(playerHealth - dmg, static_cast<int>(0)); std::cout << "[PlayerHealthComponent] Took " << dmg << " damage"<< "\n";}
            /*****************************************************************************************
            \brief
            Increases the player's health by a specified amount.

            \param amount
            Amount to heal.

            \details
            Health is clamped to the maximum health value. Prints a debug message showing healing.
            *****************************************************************************************/
            void Heal(int amount)
            {playerHealth = std::min(playerHealth + amount, static_cast<int>(playerMaxhealth)); std::cout << "[PlayerHealthComponent] Healed " << amount << ", current health = " << playerHealth << "\n";}
    };
}