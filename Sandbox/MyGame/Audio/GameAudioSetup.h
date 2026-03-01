/*********************************************************************************************
 \file      GameAudio.h
 \par       SofaSpuds
 \author    Choo Jian Wei - Primary Author

 \brief     Declares GameAudio, a lightweight facade that behaviour scripts use to trigger
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
namespace Framework
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

        /*************************************************************************************
          \brief Construct GameAudio and create the appropriate internal controller.

          \param audio   Pointer to the game object's AudioComponent.
          \param entity  Whether this object is a player or enemy.
        *************************************************************************************/
        GameAudio(Framework::AudioComponent* audio, Entity entity);

        // ---------------------------------------------------------------------------------
        // Unified audio interface  (behaviour scripts call these)
        // ---------------------------------------------------------------------------------

        /*************************************************************************************
          \brief Play the primary attack sound for this entity.

          \details  Player  → PlaySlash()
                    Enemy   → PlayAttack()
        *************************************************************************************/
        void PlayAttack(float posX = 0.0f, float posY = 0.0f, bool is3D = false);

        /*************************************************************************************
          \brief Play a hurt / damage-received sound.

          \details  Player  → PlayPlayerHit()
                    Enemy   → PlayHurt()
        *************************************************************************************/
        void PlayHurt(float posX = 0.0f, float posY = 0.0f, bool is3D = false);

        /*************************************************************************************
          \brief Play a death sound.

          \details  Player  → PlayPlayerDead()
                    Enemy   → PlayDeath()
        *************************************************************************************/
        void PlayDeath(float posX = 0.0f, float posY = 0.0f, bool is3D = false);

        /*************************************************************************************
          \brief Play a footstep sound (player only; no-op for enemies).
        *************************************************************************************/
        void PlayFootstep();

        /*************************************************************************************
          \brief Play a secondary attack / punch sound (player only; no-op for enemies).
        *************************************************************************************/
        void PlayPunch();

        /*************************************************************************************
          \brief Play a grapple sound (player only; no-op for enemies).
        *************************************************************************************/
        void PlayGrapple();

        /*************************************************************************************
          \brief Play an ineffective-hit sound (player only; no-op for enemies).
        *************************************************************************************/
        void PlayBoink();

        /*************************************************************************************
          \brief Per-frame update. Forwards to EnemyAudioController::Update() for 3D
                 position tracking. Pass the owning object's world position.

          \param posX  World X position of the owner.
          \param posY  World Y position of the owner.
          \note        No-op for players (player sounds are not spatialised).
        *************************************************************************************/
        void Update(float posX, float posY);

        // ---------------------------------------------------------------------------------
        // Direct controller access (for behaviour scripts that need specific calls)
        // ---------------------------------------------------------------------------------

        /// Returns the player controller, or nullptr if this is an enemy.
        PlayerAudioController* GetPlayerController() const { return m_Player.get(); }

        /// Returns the enemy controller, or nullptr if this is a player.
        EnemyAudioController* GetEnemyController()  const { return m_Enemy.get(); }

    private:

        std::unique_ptr<PlayerAudioController> m_Player;
        std::unique_ptr<EnemyAudioController>  m_Enemy;
    };

} 
