/*********************************************************************************************
 \file      EnemyAttackComponent.h
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Declaration and implementation of the EnemyAttackComponent class. This component
            defines enemy attack logic, handling timing, hitbox activation, and damage output
            during combat interactions.

 \details
            The EnemyAttackComponent provides essential combat behavior for enemy entities:
            - Manages attack intervals using an internal timer and attack speed variable.
            - Spawns and updates hitboxes to detect collisions with player entities.
            - Supports serialization of attack and hitbox parameters for configurable tuning.
            - Utilizes TransformComponent data to align attack position with the enemy’s
              current world coordinates.

            Designed for reuse across multiple enemy types, this component forms the core
            of basic melee-style attack functionality within the game framework.

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
namespace Framework
{
    /*********************************************************************************************
     \class EnemyAttackComponent
    \brief
    Represents an enemy's attack behavior, including attack timing, damage, and hitbox activation.

    \details
    The EnemyAttackComponent manages an enemy's attack logic in a game. It maintains key attack
    properties such as `damage`, `attack_speed`, and a `hitbox` representing the area of effect
    for the attack.

    - `Update(float dt, TransformComponent* tr)` handles attack timing. When the internal
        `attack_timer` exceeds `attack_speed`, the component activates the `hitbox` at the enemy's
        current position, enabling collision detection or damage application.
    - The `hitbox` remains active for a short duration, after which it is automatically
        deactivated.
    - `Serialize(ISerializer& s)` allows saving or loading attack parameters such as damage,
        speed, and hitbox dimensions.
    - `Clone()` provides a deep copy of the component, including its unique hitbox.

    This design allows modular and reusable enemy attack behavior in a component-based
    architecture.

    \note
    - The class uses a unique pointer for the hitbox to enforce unique ownership and
        prevent accidental copies.
    - Use `Update(float dt, TransformComponent* tr)` in each game update cycle to control
        attack logic and hitbox activation.
    - Hitbox duration and spawn coordinates are reset automatically during updates.
    *********************************************************************************************/

    class EnemyAttackComponent : public GameComponent 
    {
        public:
            int damage{10};
            float attack_speed{1.0f};
            float attack_timer{ 0.0f };
            std::unique_ptr<HitBoxComponent> hitbox;
            /*****************************************************************************************
            \brief
            Default constructor. Initializes the hitbox for the enemy attack.
            *****************************************************************************************/
            EnemyAttackComponent() { hitbox = std::make_unique<HitBoxComponent>(); }
            /*****************************************************************************************
            \brief
            Parameterized constructor. Sets attack damage and attack speed, and initializes the hitbox.

            \param dmg
            Damage the enemy deals per attack.

            \param spd
            Time interval between attacks (attack speed in seconds).
            *****************************************************************************************/
            EnemyAttackComponent(int dmg, float spd) : damage(dmg), attack_speed(spd) { hitbox = std::make_unique<HitBoxComponent>(); }
            /*****************************************************************************************
            \brief
            Initializes the component.

            \details
            Calls the hitbox's initialize method to set up any internal data.
            *****************************************************************************************/
            void initialize() override { hitbox->initialize(); }
            /*****************************************************************************************
            \brief
            Handles incoming messages.

            \param m
            Message to process.

            \details
            Currently unused but required by the GameComponent interface.
            *****************************************************************************************/
            void SendMessage(Message& m) override { (void)m; }
            /*****************************************************************************************
            \brief
            Serializes or deserializes the component data.

            \param s
            Serializer instance used to read/write component properties.

            \details
            Reads damage, attack speed, and hitbox parameters (width, height, duration) if available.
            *****************************************************************************************/
            void Serialize(ISerializer& s) override 
            {
                if (s.HasKey("damage")) StreamRead(s,"damage", damage);
                if (s.HasKey("attack_speed")) StreamRead(s,"attack_speed", attack_speed);
                if (s.HasKey("hitwidth")) StreamRead(s, "hitwidth", hitbox->width);
                if (s.HasKey("hitheight")) StreamRead(s, "hitheight", hitbox->height);
                if (s.HasKey("hitduration")) StreamRead(s, "hitduration", hitbox->duration);
            }
            /*****************************************************************************************
            \brief
            Creates a deep copy of this component.

            \return
            A std::unique_ptr to a new EnemyAttackComponent with identical damage, attack speed,
            and hitbox data.
            *****************************************************************************************/
            std::unique_ptr<GameComponent> Clone() const override 
            {
               auto copy = std::make_unique<EnemyAttackComponent>();
               copy->damage= damage;
               copy->attack_speed= attack_speed;
               copy->hitbox = std::make_unique<HitBoxComponent>(*hitbox);
               return copy;
            }
            /*****************************************************************************************
            \brief
            Updates attack logic and hitbox activation.

            \param dt
            Delta time since last frame (in seconds).

            \param tr
            Pointer to the enemy's TransformComponent to determine position for spawning the hitbox.

            \details
            Increments the attack timer, activates the hitbox when the timer exceeds attack speed,
            sets hitbox spawn position, and manages hitbox duration and automatic deactivation.
            *****************************************************************************************/
            void Update(float dt, TransformComponent* tr)
            {
                attack_timer += dt;
                if (attack_timer >= attack_speed)
                {
                    attack_timer = 0.0f;
                    hitbox->active = true;
                    hitbox->spawnX = tr->x;
                    hitbox->spawnY = tr->y;
                    hitbox->duration = 0.15f;
                    std::cout << "Enemy attacked! HitBox active at (" << hitbox->spawnX << ", " << hitbox->spawnY << ")\n";
                }
                if (hitbox->active)
                {
                    hitbox->duration -= dt;
                    if (hitbox->duration <= 0.0f) 
                    {
                        hitbox->DeactivateHurtBox();
                        hitbox->duration = 0.1f;
                    }
                }

            }
    };
}