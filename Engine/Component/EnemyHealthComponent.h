/*********************************************************************************************
\file      EnemyHealthComponent.h
\par       SofaSpuds
\author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

\brief     Declaration and implementation of the EnemyHealthComponent class. This component
           manages the health and damage mechanics of enemy entities.

\details
           The EnemyHealthComponent provides a simple health system for enemies, including:
           - Storing current and maximum health values.
           - Handling damage application with clamping to zero.
           - Healing with clamping to maximum health.
           - Supporting serialization of health values.
           - Cloning for reuse across multiple enemy instances.

\note      This component does not handle death logic, animations, or interactions;
           it strictly manages health values.

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
    \class EnemyHealthComponent
    \brief
    Component responsible for tracking and modifying enemy health.

    \details
    Provides methods to apply damage, heal, serialize, and clone health data for enemies.
    *****************************************************************************************/
    class EnemyHealthComponent : public GameComponent 
    {
        public:
            int enemyHealth{100};
            int enemyMaxhealth{ 100 };
            /*****************************************************************************************
            \brief
            Initializes the component.

            \details
            Required override of GameComponent interface. Currently does nothing.
            *****************************************************************************************/
            void initialize() override {}
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
            Serializes or deserializes the health values.

            \param s
            Serializer instance.

            \details
            Supports reading `enemyHealth` and `enemyMaxhealth` from the serializer if the keys exist.
            *****************************************************************************************/
            void Serialize(ISerializer& s) override 
            {
                if (s.HasKey("enemyHealth")) StreamRead(s, "enemyHealth", enemyHealth);
                if (s.HasKey("enemyMaxhealth")) StreamRead(s, "enemyMaxhealth", enemyMaxhealth);
            }
            /*****************************************************************************************
            \brief
            Creates a clone of this component.

            \return
            A std::unique_ptr to a new EnemyHealthComponent instance.

            \details
            Clones the current health and maximum health values for use on multiple enemy entities.
            *****************************************************************************************/
            std::unique_ptr<GameComponent> Clone() const override 
            {
             auto copy = std::make_unique<EnemyHealthComponent>();
             copy->enemyHealth = enemyHealth;
             copy->enemyMaxhealth = enemyMaxhealth;
             return copy;
            }
            /*****************************************************************************************
            \brief
            Applies damage to the enemy.

            \param dmg
            Amount of damage to apply.

            \details
            Reduces `enemyHealth` by `dmg`, clamped to a minimum of zero. Prints the updated health
            to the console.
            *****************************************************************************************/
            void TakeDamage(int dmg)
            { 
              enemyHealth = std::max(enemyHealth - dmg, 0);
              std::cout << "[EnemyHealthComponent] Took " << dmg << " damage, current health = " << enemyHealth << "\n";
            }
            /*****************************************************************************************
            \brief
            Heals the enemy.

            \param amount
            Amount of health to restore.

            \details
            Increases `enemyHealth` by `amount`, clamped to `enemyMaxhealth`. Prints the updated health
            to the console.
            *****************************************************************************************/
            void Heal(int amount)
            {
                enemyHealth = std::min(enemyHealth + amount, enemyMaxhealth);
                std::cout << "[EnemyHealthComponent] Healed " << amount << ", current health = " << enemyHealth << "\n";
            }
    };
}