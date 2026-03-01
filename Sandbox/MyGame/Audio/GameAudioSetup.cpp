/*********************************************************************************************
 \file      GameAudio.cpp
 \par       SofaSpuds
 \author    Choo Jian Wei - Primary Author

 \brief     Implementation of GameAudio.

 \copyright
            All content  2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "GameAudioSetup.h"

namespace Framework
{
    GameAudio::GameAudio(Framework::AudioComponent* audio, Entity entity)
    {
        if (entity == Entity::Player)
            m_Player = std::make_unique<PlayerAudioController>(audio);
        else
            m_Enemy = std::make_unique<EnemyAudioController>(audio);
    }

    // ---------------------------------------------------------------------------------
    // Unified interface
    // ---------------------------------------------------------------------------------

    void GameAudio::PlayAttack(float posX, float posY, bool is3D)
    {
        if (m_Player) m_Player->PlaySlash();
        else if (m_Enemy)  m_Enemy->PlayAttack(posX, posY, is3D);
    }

    void GameAudio::PlayHurt(float posX, float posY, bool is3D)
    {
        if (m_Player) m_Player->PlayPlayerHit();
        else if (m_Enemy)  m_Enemy->PlayHurt(posX, posY, is3D);
    }

    void GameAudio::PlayDeath(float posX, float posY, bool is3D)
    {
        if (m_Player) m_Player->PlayPlayerDead();
        else if (m_Enemy)  m_Enemy->PlayDeath(posX, posY, is3D);
    }

    void GameAudio::PlayFootstep()
    {
        if (m_Player) m_Player->PlayFootstep();
        // No-op for enemies.
    }

    void GameAudio::PlayPunch()
    {
        if (m_Player) m_Player->PlayPunch();
        // No-op for enemies.
    }

    void GameAudio::PlayGrapple()
    {
        if (m_Player) m_Player->PlayGrapple();
        // No-op for enemies.
    }

    void GameAudio::PlayBoink()
    {
        if (m_Player) m_Player->PlayBoink();
        // No-op for enemies.
    }

    void GameAudio::Update(float posX, float posY)
    {
        if (m_Enemy) m_Enemy->Update(posX, posY);
        // No 3D tracking needed for the player.
    }

} // namespace MyGame
