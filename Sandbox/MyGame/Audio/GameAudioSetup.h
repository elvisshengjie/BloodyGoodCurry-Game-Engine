/*********************************************************************************************
 \file      GameAudioSetup.h
 \par       SofaSpuds
 \author    Choo Jian Wei (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Declares GameAudioSetup, a lightweight facade that behaviour scripts use to trigger
            audio without needing to know which controller is active.

 \details
            GameAudio owns either a PlayerAudioController or an EnemyAudioController
            (never both). It is constructed by passing in the relevant AudioComponent and
            specifying the entity category. Behaviour scripts call the unified interface
            (e.g. GameAudio::PlayAttack(), GameAudio::PlayHurt()) and GameAudio forwards
            to whichever controller is active.

            This avoids behaviour scripts needing to #include both controller headers or
            hold two optional pointers themselves.

            Typical usage in a behaviour script:
            -----------------------------------------------------------------------
            // On init:
            m_Audio = std::make_unique<MyGame::GameAudio>(
                          audioComponent, MyGame::GameAudio::Entity::Player);

            // On event:
            m_Audio->PlayAttack();   // routes to PlayerAudioController::PlaySlash()
            m_Audio->PlayHurt();     // routes to PlayerAudioController::PlayBoink() etc.
            -----------------------------------------------------------------------

 \note      Must only be compiled as part of MyGame.vcxproj.

 \copyright
            All content  2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once
#include "PlayerAudioController.h"
#include "EnemyAudioController.h"
#include <memory>
namespace mygame
{
    /*************************************************************************************
      \class GameAudio
      \brief Unified audio facade for behaviour scripts.

      Constructed once per game object. Routes generic audio calls to the correct
      game-side controller based on the entity category provided at construction.
    *************************************************************************************/
    class GameAudio
    {
    public:

        /*************************************************************************************
          \enum Entity
          \brief Identifies which controller to create.
        *************************************************************************************/
        enum class Entity
        {
            Player,  ///< Creates a PlayerAudioController internally.
            Enemy    ///< Creates an EnemyAudioController internally.
        };
        GameAudio(Framework::AudioComponent* audio, Entity entity);
        void PlayAttack(float posX = 0.0f, float posY = 0.0f);
        void PlayHurt(float posX = 0.0f, float posY = 0.0f);
        void PlayDeath(float posX = 0.0f, float posY = 0.0f);
        void PlayFootstep();
        void PlayPunch();
        void PlayGrapple();
        void PlayBoink();
        void Update(float posX, float posY);

        /*************************************************************************************
         \brief Returns the player audio controller.
         \return Pointer to the PlayerAudioController, or nullptr if this instance was
                 constructed with Entity::Enemy.
       *************************************************************************************/
        PlayerAudioController* GetPlayerController() const { return m_Player.get(); }

        /*************************************************************************************
         \brief Returns the enemy audio controller.
         \return Pointer to the EnemyAudioController, or nullptr if this instance was
                 constructed with Entity::Player.
         *************************************************************************************/
        EnemyAudioController* GetEnemyController()  const { return m_Enemy.get(); }

    private:

        std::unique_ptr<PlayerAudioController> m_Player;
        std::unique_ptr<EnemyAudioController>  m_Enemy;
    };

} 
