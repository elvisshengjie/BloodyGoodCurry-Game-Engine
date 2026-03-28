/*********************************************************************************************
 \file      EnemyAttackComponent.h
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 80%
            yimo.kong ( yimo.kong@digipen.edu) - Author, 20%
 \brief     Declaration and implementation of the EnemyAttackComponent class. This component
            defines enemy attack logic, handling timing, hitbox activation, and damage output
            during combat interactions.

 \details
            The EnemyAttackComponent provides essential combat behavior for enemy entities:
            - Manages attack intervals using an internal timer and attack speed variable.
            - Spawns and updates hitboxes to detect collisions with player entities.
            - Supports serialization of attack and hitbox parameters for configurable tuning.
            - Utilizes TransformComponent data to align attack position with the enemy
              current world coordinates.
            - Stores lightweight runtime state used by scripted multi-phase attacks.

            Designed for reuse across multiple enemy types, this component forms the core
            of basic melee-style attack functionality within the game framework.

 \note      HitBoxComponent is managed through ComponentHandleT / ComponentPool ownership,
            so prefab cloning can duplicate attack setup without manual lifetime handling.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include "Composition/Component.h"
#include "Memory/ComponentPool.h"
#include "Serialization/Serialization.h"
#include "Component/HitBoxComponent.h"
#include "Component/TransformComponent.h"
#include <memory>
#include <iostream>

namespace Framework
{
    /*****************************************************************************************
      \class EnemyAttackComponent
      \brief Represents an enemy attack behavior component.

      This component controls enemy attack timing and manages a hitbox used to deal
      damage to other entities. It works together with a TransformComponent to spawn
      the hitbox at the enemyï¿½s current location and uses internal timers to regulate
      attack intervals and durations.
    *****************************************************************************************/
    class EnemyAttackComponent : public GameComponent
    {
    public:
        int damage{ 1 };               ///< Damage dealt by this enemy attack.
        float attack_speed{ 3.0f };     ///< Cooldown time (seconds) between consecutive attacks.
        float attack_timer{ 0.0f };     ///< Tracks elapsed time since the last attack.
        float hitboxElapsed{ 0.0f };    ///< Time spent in the currently active hitbox phase.
        bool attack2BeamPhaseActive{ false }; ///< Runtime flag for HeiBang's attack2 beam follow-up.
        bool attack2BeamDamageSpawned{ false }; ///< True once HeiBang's beam damage zone has been created.
        float attack2BeamStartX{ 0.0f }; ///< Cached world-space X origin for HeiBang's beam.
        float attack2BeamStartY{ 0.0f }; ///< Cached world-space Y origin for HeiBang's beam.
        float attack2BeamTargetX{ 0.0f }; ///< Cached world-space X target for HeiBang's beam.
        float attack2BeamTargetY{ 0.0f }; ///< Cached world-space Y target for HeiBang's beam.
        float attack2BeamThickness{ 0.0f }; ///< Cached thickness for HeiBang's beam damage box.
        HitBoxComponent* attack2BeamRuntimeHitbox{ nullptr }; ///< Runtime pointer to HeiBang's active beam hitbox.
        bool attack3VolleySpawned{ false }; ///< True once HeiBang's attack3 projectile burst has been emitted.
        ComponentHandleT<HitBoxComponent> hitbox; ///< Managed hitbox instance used for attacks.

        /*************************************************************************************
          \brief Default constructor. Initializes a new HitBoxComponent.
        *************************************************************************************/
        EnemyAttackComponent()
        {
            hitbox = ComponentPool<HitBoxComponent>::CreateTyped();
        }

        /*************************************************************************************
          \brief Constructs an enemy attack component with custom damage and attack speed.
          \param dmg  Attack damage value.
          \param spd  Time (in seconds) between attacks.
        *************************************************************************************/
        EnemyAttackComponent(int dmg, float spd)
            : damage(dmg), attack_speed(spd)
        {
            hitbox = ComponentPool<HitBoxComponent>::CreateTyped();
        }

        /*************************************************************************************
          \brief Initializes the component and its owned HitBoxComponent.
          \note  Called automatically when attached to a GameObjectComposition.
        *************************************************************************************/
        void initialize() override
        {
            hitbox->initialize();
        }

        /*************************************************************************************
          \brief Handles incoming messages. (Currently unused for this component.)
          \param m  Reference to the Message object.
        *************************************************************************************/
        void SendMessage(Message& m) override
        {
            (void)m;
        }

        /*************************************************************************************
          \brief Serializes component data for loading from JSON or prefab files.
          \param s  Reference to serializer object.
          \details
            Reads the following fields if present:
            - "damage"
            - "attack_speed"
            - "hitwidth"
            - "hitheight"
            - "hitduration"
            Runtime timers and beam-phase state are intentionally not serialized.
        *************************************************************************************/
        void Serialize(ISerializer& s) override
        {
            if (s.HasKey("damage"))        StreamRead(s, "damage", damage);
            if (s.HasKey("attack_speed"))  StreamRead(s, "attack_speed", attack_speed);
            if (s.HasKey("hitwidth"))      StreamRead(s, "hitwidth", hitbox->width);
            if (s.HasKey("hitheight"))     StreamRead(s, "hitheight", hitbox->height);
            if (s.HasKey("hitduration"))   StreamRead(s, "hitduration", hitbox->duration);
        }

        /*************************************************************************************
          \brief Creates a deep copy of this component for prefab instancing.
          \return A component handle holding the cloned EnemyAttackComponent.
          \details Runtime-only state such as the attack2 beam phase is reset on clone.
        *************************************************************************************/
        ComponentHandle Clone() const override
        {
            auto copy = ComponentPool<EnemyAttackComponent>::CreateTyped();
            copy->damage = damage;
            copy->attack_speed = attack_speed;
            copy->attack2BeamPhaseActive = false;
            copy->attack2BeamDamageSpawned = false;
            copy->attack2BeamStartX = 0.0f;
            copy->attack2BeamStartY = 0.0f;
            copy->attack2BeamTargetX = 0.0f;
            copy->attack2BeamTargetY = 0.0f;
            copy->attack2BeamThickness = 0.0f;
            copy->attack2BeamRuntimeHitbox = nullptr;
            copy->attack3VolleySpawned = false;
            copy->hitbox = ComponentPool<HitBoxComponent>::CreateTyped(*hitbox);
            return copy;
        }

        /*************************************************************************************
          \brief Updates attack timing, triggers hitbox activation, and manages duration.
          \param dt  Delta time in seconds since the last frame.
          \param tr  Pointer to the TransformComponent of the owning GameObject.
          \details
            - Increments the internal attack timer by dt.
            - When the timer exceeds attack_speed, resets it and activates the hitbox.
            - Sets the hitbox position based on the owner transform.
            - Automatically deactivates the hitbox after its duration expires.
            - Intended for simple self-driven enemies; more advanced AI can still manage
              hitbox state directly while reusing the same serialized fields.
        *************************************************************************************/
        void Update(float dt, TransformComponent* tr)
        {
            attack_timer += dt;

            // Trigger attack when cooldown has elapsed
            if (attack_timer >= attack_speed && !hitbox->active)
            {
                attack_timer = 0.0f;
                hitbox->active = true;
                hitbox->spawnX = tr->x;
                hitbox->spawnY = tr->y;
                hitbox->duration = 0.92f;
                hitboxElapsed = 0.0f;
                std::cout << "Enemy attacked! HitBox active at ("
                    << hitbox->spawnX << ", " << hitbox->spawnY << ")\n";
            }

            // Manage hitbox lifetime and deactivate when finished
            if (hitbox->active)
            {
                hitbox->duration -= dt;
                if (hitbox->duration <= 0.0f)
                {
                    hitbox->DeactivateHurtBox();
                    hitbox->duration = 0.92f;
                }
            }
        }
    };
}
