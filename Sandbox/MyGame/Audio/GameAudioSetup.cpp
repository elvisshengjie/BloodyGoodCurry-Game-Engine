/*********************************************************************************************
 \file      GameAudioSetup.cpp
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Implementation of GameAudioSetup.

 \copyright
            All content  2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "GameAudioSetup.h"

namespace mygame
{
    /*****************************************************************************************
      \brief Constructs a GameAudio facade for either a player or enemy entity.
      \param audio  AudioComponent sourced from the owning game object's prefab.
      \param entity Determines which underlying controller is created; exactly one
                    of m_Player or m_Enemy will be non-null after construction.
    *****************************************************************************************/
    GameAudio::GameAudio(Framework::AudioComponent* audio, Entity entity)
    {
        if (entity == Entity::Player)
            m_Player = std::make_unique<PlayerAudioController>(audio);
        else
            m_Enemy = std::make_unique<EnemyAudioController>(audio);
    }

    /*****************************************************************************************
      \brief Plays an attack sound appropriate for the entity type.
      \param posX  World X position (used for 3D spatialisation on enemies; ignored for player).
      \param posY  World Y position (used for 3D spatialisation on enemies; ignored for player).
      \details Routes to PlayerAudioController::PlaySlash or EnemyAudioController::PlayAttack.
    *****************************************************************************************/
    void GameAudio::PlayAttack(float posX, float posY)
    {
        if (m_Player) m_Player->PlaySlash();
        else if (m_Enemy)  m_Enemy->PlayAttack(posX, posY);
    }
    /*****************************************************************************************
      \brief Plays a hurt sound appropriate for the entity type.
      \param posX  World X position (used for 3D spatialisation on enemies; ignored for player).
      \param posY  World Y position (used for 3D spatialisation on enemies; ignored for player).
      \details Routes to PlayerAudioController::PlayPlayerHit or EnemyAudioController::PlayHurt.
    *****************************************************************************************/
    void GameAudio::PlayHurt(float posX, float posY)
    {
        if (m_Player) m_Player->PlayPlayerHit();
        else if (m_Enemy)  m_Enemy->PlayHurt(posX, posY);
    }
    /*****************************************************************************************
      \brief Plays a death sound appropriate for the entity type.
      \param posX  World X position (used for 3D spatialisation on enemies; ignored for player).
      \param posY  World Y position (used for 3D spatialisation on enemies; ignored for player).
      \details Routes to PlayerAudioController::PlayPlayerDead or EnemyAudioController::PlayDeath.
    *****************************************************************************************/
    void GameAudio::PlayDeath(float posX, float posY)
    {
        if (m_Player) m_Player->PlayPlayerDead();
        else if (m_Enemy)  m_Enemy->PlayDeath(posX, posY);
    }
    /*****************************************************************************************
      \brief Plays a player footstep sound. No-op for enemies.
    *****************************************************************************************/
    void GameAudio::PlayFootstep()
    {
        if (m_Player) m_Player->PlayFootstep();
        // No-op for enemies.
    }
    /*****************************************************************************************
      \brief Plays a player punch sound. No-op for enemies.
    *****************************************************************************************/

    void GameAudio::PlayPunch()
    {
        if (m_Player) m_Player->PlayPunch();
        // No-op for enemies.
    }
    /*****************************************************************************************
      \brief Plays a player grapple/throw sound. No-op for enemies.
    *****************************************************************************************/
    void GameAudio::PlayGrapple()
    {
        if (m_Player) m_Player->PlayGrapple();
        // No-op for enemies.
    }
    /*****************************************************************************************
      \brief Plays a player boink sound. No-op for enemies.
    *****************************************************************************************/
    void GameAudio::PlayBoink()
    {
        if (m_Player) m_Player->PlayBoink();
        // No-op for enemies.
    }
    /*****************************************************************************************
      \brief Per-frame update to reposition active enemy 3D audio channels.
      \param posX  Current world X position of the enemy.
      \param posY  Current world Y position of the enemy.
      \details No-op for player entities as player audio does not require 3D tracking.
    *****************************************************************************************/
    void GameAudio::Update(float posX, float posY)
    {
        if (m_Enemy) m_Enemy->Update(posX, posY);
        // No 3D tracking needed for the player.
    }

} // namespace mygame
