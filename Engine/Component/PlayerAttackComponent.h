/*********************************************************************************************
\file      PlayerAttackComponent.h
\par       SofaSpuds
\author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

\brief     Declaration and implementation of the PlayerAttackComponent class. This component
           handles player attack logic, including damage, attack speed, and hitbox management.

\details
           The PlayerAttackComponent provides core combat functionality for player entities:
           - Manages attack properties such as damage and speed.
           - Spawns and updates hitboxes to detect collisions with enemies.
           - Supports serialization of attack parameters for flexible tuning.
           - Can be cloned for multiple player instances.

\note      HitBoxComponent is managed through a std::unique_ptr to ensure proper ownership
           semantics and automatic cleanup when the component is destroyed.

\copyright
           All content © 2025 DigiPen Institute of Technology Singapore.
           All rights reserved.
*********************************************************************************************/
#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include "Component/HitBoxComponent.h"
#include "Component/TransformComponent.h"
#include <memory>
#include <iostream>
namespace Framework
{
    /*****************************************************************************************
    \class PlayerAttackComponent
    \brief
    Component responsible for handling player attacks and associated hitbox logic.

    \details
    Provides methods to perform attacks, update active hitboxes, and serialize attack data.
    *****************************************************************************************/
    class PlayerAttackComponent : public GameComponent 
    {
        public:
            int damage{50};
            float attack_speed{1.0f};
            std::unique_ptr<HitBoxComponent> hitbox;
            /*****************************************************************************************
            \brief
            Default constructor.
            *****************************************************************************************/
            PlayerAttackComponent() = default;
            /*****************************************************************************************
            \brief
            Constructor with specified damage and attack speed.

            \param dmg
            Damage per attack.

            \param spd
            Attack speed in seconds.
            *****************************************************************************************/
            PlayerAttackComponent(int dmg, float spd) : damage(dmg), attack_speed(spd) {}
            /*****************************************************************************************
            \brief
            Initializes the component.

            \details
            Prints a debug message indicating that this object has a PlayerAttackComponent.
            *****************************************************************************************/
            void initialize() override { std::cout << "This object has a PlayerAttackComponent!\n"; }
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
            Serializes or deserializes the attack properties.

            \param s
            Serializer instance.

            \details
            Reads "damage" and "attack_speed" from the serializer.
            *****************************************************************************************/
            void Serialize(ISerializer& s) override 
            {
                if (s.HasKey("damage")) StreamRead(s,"damage", damage);
                if (s.HasKey("attack_speed")) StreamRead(s,"attack_speed", attack_speed);
            }
            /*****************************************************************************************
            \brief
            Creates a clone of this component.

            \return
            A std::unique_ptr to a new PlayerAttackComponent instance with the same damage and attack speed.
            *****************************************************************************************/
            std::unique_ptr<GameComponent> Clone() const override 
            {
               auto copy = std::make_unique<PlayerAttackComponent>();
               copy->damage= damage;
               copy->attack_speed= attack_speed;
               return copy;
            }
            /*****************************************************************************************
            \brief
            Performs an attack using the player's TransformComponent.

            \param playerTr
            Pointer to the player's TransformComponent.

            \details
            Creates or updates the hitbox, sets its position, size, damage, and duration,
            and activates it to detect collisions.
            *****************************************************************************************/
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
            /*****************************************************************************************
            \brief
            Updates the active hitbox over time.

            \param dt
            Delta time since the last frame.

            \param tr
            Pointer to the player's TransformComponent (currently unused).

            \details
            Decreases the hitbox duration and deactivates it when time runs out. Resets duration for next attack.
            *****************************************************************************************/
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