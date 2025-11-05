/*********************************************************************************************
 \file      PlayerHealthComponent.h
 \par       SofaSpuds
 \author    - Primary Author, 100%

 \brief     Declares the PlayerHealthComponent class, which stores and manages the player’s
            health data. This component defines current and maximum health values used by
            gameplay systems for combat, UI, and respawn logic.

 \details
            The PlayerHealthComponent is a lightweight data container that tracks the
            player’s current and maximum health values. It provides serialization for
            prefab-driven initialization and supports cloning for respawning or prefab
            instancing. Systems such as combat, damage, and UI can reference this data
            to update the player’s health bar or trigger death behavior.

            Responsibilities:
            - Store and manage player health values.
            - Provide serialization for prefab or save data.
            - Support deep-copy functionality.
            - Log initialization for debugging purposes.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include <iostream>

namespace Framework
{
    /*****************************************************************************************
      \class PlayerHealthComponent
      \brief Component that holds the player’s current and maximum health values.

      This component acts as a simple data container. It can be queried or modified by
      combat, healing, or UI systems to update gameplay state accordingly.
    *****************************************************************************************/
    class PlayerHealthComponent : public GameComponent
    {
    public:
        int playerHealth{ 100 };      ///< Current health of the player.
        int playerMaxhealth{ 100 };   ///< Maximum health value for the player.

        /*************************************************************************************
          \brief Called when the component is initialized.
          \details Prints a debug message confirming this GameObject has a PlayerHealthComponent.
        *************************************************************************************/
        void initialize() override { std::cout << "This object has a PlayerHealthComponent!\n"; }

        /*************************************************************************************
          \brief Handles messages sent to this component.
          \param m  Reference to the message.
          \note  Currently unused; placeholder for future logic (e.g., damage events).
        *************************************************************************************/
        void SendMessage(Message& m) override { (void)m; }

        /*************************************************************************************
          \brief Serializes player health data from the provided serializer.
          \param s  Reference to the serializer.
          \details Reads "playerHealth" and "playerMaxhealth" values if available in the data stream.
        *************************************************************************************/
        void Serialize(ISerializer& s) override
        {
            if (s.HasKey("playerHealth")) StreamRead(s, "playerHealth", playerHealth);
            if (s.HasKey("playerMaxhealth")) StreamRead(s, "playerMaxhealth", playerMaxhealth);
        }

        /*************************************************************************************
          \brief Creates a deep copy of this component.
          \return A unique_ptr holding a cloned PlayerHealthComponent instance.
          \details Copies current and maximum health values to the new instance.
        *************************************************************************************/
        std::unique_ptr<GameComponent> Clone() const override
        {
            auto copy = std::make_unique<PlayerHealthComponent>();
            copy->playerHealth = playerHealth;
            copy->playerMaxhealth = playerMaxhealth;
            return copy;
        }
    };
}
