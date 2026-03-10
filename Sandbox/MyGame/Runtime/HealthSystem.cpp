/*********************************************************************************************
 \file      HealthSystem.cpp
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 40%
            yimo.kong (yimo.kong@digipen.edu) - Secondary Author, 40%
            elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Secondary Author, 20% (Draw)
 \brief     Implements the HealthSystem responsible for managing player and enemy health,
            handling death timers, triggering death animations, and destroying objects at
            the correct time.
 \details   Responsibilities:
            - Tracks all GameObjectComposition instances that contain health components.
            - Handles enemy death: triggers death animation (if available), waits for both
              animation completion and a minimum timer before destruction.
            - Handles player death: plays death animation, enforces invulnerability timers,
              and destroys the player only after animation + timer finish.
            - Uses stable IDs instead of raw pointers to avoid dangling references.
            - Fully integrates with SpriteAnimationComponent for frame-based animation logic.
 \copyright
            All content � 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "HealthSystem.h"
#include "Factory/Factory.h"
#include "Component/RenderComponent.h"
#include "Component/SpriteAnimationComponent.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <iostream>
#include <string_view>
#include "Common/CRTDebug.h"   // <- bring in DBG_NEW

#ifdef _DEBUG
#define new DBG_NEW       // <- redefine new AFTER all includes
#endif
namespace Framework
{
    namespace
    {
        void EmitCombatAudio(const CombatAudioCallback& callback, GOC* source, CombatAudioEvent event)
        {
            if (callback && source)
                callback(source, event);
        }

        /*****************************************************************************************
         \brief  Find the index of a named animation on a SpriteAnimationComponent (case-insensitive).

         \param anim
                Pointer to the SpriteAnimationComponent.
         \param desired
                Name of the animation we want to find.

         \return
                Index of the animation if found, otherwise -1.
        *****************************************************************************************/
        int FindAnimationIndex(SpriteAnimationComponent* anim, std::string_view desired)
        {
            if (!anim)
                return -1;

            auto equalsIgnoreCase = [](std::string_view a, std::string_view b)
                {
                    if (a.size() != b.size())
                        return false;

                    for (std::size_t i = 0; i < a.size(); ++i)
                    {
                        if (std::tolower(static_cast<unsigned char>(a[i])) !=
                            std::tolower(static_cast<unsigned char>(b[i])))
                            return false;
                    }
                    return true;
                };

            for (std::size_t i = 0; i < anim->animations.size(); ++i)
            {
                if (equalsIgnoreCase(anim->animations[i].name, desired))
                    return static_cast<int>(i);
            }

            return -1;
        }

        /*****************************************************************************************
         \brief  Helper to safely switch an animation by name if it exists on the given object.

         \param goc
                Game object that owns the SpriteAnimationComponent.
         \param name
                Animation name we want to set as active.

         \details
                Does nothing if the animation component or requested animation is missing.
        *****************************************************************************************/
        void PlayAnimationIfAvailable(GOC* goc, std::string_view name)
        {
            if (!goc)
                return;

            auto* anim =
                goc->GetComponentType<SpriteAnimationComponent>(ComponentTypeId::CT_SpriteAnimationComponent);
            if (!anim)
                return;

            const int idx = FindAnimationIndex(anim, name);
            if (idx >= 0 && idx != anim->ActiveAnimationIndex())
            {
                anim->SetActiveAnimation(idx);
            }
        }

        /*****************************************************************************************
         \brief  Compute the duration (in seconds) of a given animation based on its frame range
                 and FPS settings.

         \param anim
                Pointer to the SpriteAnimationComponent.
         \param name
                Name of the animation whose duration we want to calculate.

         \return
                Duration of the animation in seconds, or 0.0f if it cannot be determined.
        *****************************************************************************************/
        float AnimationDuration(SpriteAnimationComponent* anim, std::string_view name)
        {
            if (!anim)
                return 0.0f;

            const int idx = FindAnimationIndex(anim, name);
            if (idx < 0 || idx >= static_cast<int>(anim->animations.size()))
                return 0.0f;

            const auto& sheet = anim->animations[static_cast<std::size_t>(idx)];

            const int total = std::max(1, sheet.config.totalFrames);
            const int start = std::clamp(sheet.config.startFrame, 0, total - 1);
            const int end = (sheet.config.endFrame >= 0)
                ? std::clamp(sheet.config.endFrame, start, total - 1)
                : (total - 1);

            const int frameCount = end - start + 1;
            if (sheet.config.fps <= 0.0f)
                return 0.0f;

            // duration = number of frames in the animation / frames per second
            return static_cast<float>(frameCount) / sheet.config.fps;
        }

        /*****************************************************************************************
         \brief  Check if a named animation has finished playing (for non-looping animations).

         \param anim
                Pointer to the SpriteAnimationComponent.
         \param name
                Name of the animation we want to inspect (case-insensitive).

         \return
                True if the animation exists, is non-looping, and its current frame has reached
                or surpassed the configured end frame. Returns false if the animation is missing
                or still in progress.
        *****************************************************************************************/
        bool IsAnimationFinished(SpriteAnimationComponent* anim, std::string_view name)
        {
            if (!anim)
                return false;

            const int idx = FindAnimationIndex(anim, name);
            if (idx < 0 || idx >= static_cast<int>(anim->animations.size()))
                return false;

            const auto& sheet = anim->animations[static_cast<std::size_t>(idx)];
            if (sheet.config.loop)
                return false; // looping animations never "finish"

            const int total = std::max(1, sheet.config.totalFrames);
            const int start = std::clamp(sheet.config.startFrame, 0, total - 1);
            const int end = (sheet.config.endFrame >= 0)
                ? std::clamp(sheet.config.endFrame, start, total - 1)
                : (total - 1);

            const int current = std::clamp(sheet.currentFrame, 0, total - 1);
            return current >= end;
        }
    } // anonymous namespace

    HealthSystem::HealthSystem(gfx::Window& win)
        : window(&win)
    {
    }

    void HealthSystem::RefreshTrackedObjects()
    {
        for (auto& [id, goc] : FACTORY->Objects())
        {
            if (!goc)
                continue;

            auto* enemyHealth =
                goc->GetComponentType<EnemyHealthComponent>(
                    ComponentTypeId::CT_EnemyHealthComponent);

            auto* playerHealth =
                goc->GetComponentType<PlayerHealthComponent>(
                    ComponentTypeId::CT_PlayerHealthComponent);

            if (!(enemyHealth || playerHealth))
                continue;

            // Skip if we're already tracking this object
            if (std::find(gameObjectIds.begin(), gameObjectIds.end(), id) != gameObjectIds.end())
                continue;

            gameObjectIds.push_back(id);
        }
    }

    void HealthSystem::Initialize()
    {
        // Track by ID instead of raw pointers to avoid dangling references.
        gameObjectIds.clear();
        deathTimers.clear();

        RefreshTrackedObjects();
    }
    
   

    void HealthSystem::Update(float dt)
    {
        RefreshTrackedObjects();
        gameObjectIds.erase(
            std::remove_if(
                gameObjectIds.begin(),
                gameObjectIds.end(),
                [this, dt](GOCId id) -> bool
                {
                    GOC* goc = FACTORY->GetObjectWithId(id);
                    if (!goc)
                    {
                        deathTimers.erase(id);
                        return true;
                    }

                    // ------------------------
                    // Enemy health
                    // ------------------------
                    if (auto* enemyHealth = goc->GetComponentType<EnemyHealthComponent>(
                        ComponentTypeId::CT_EnemyHealthComponent))
                    {
                        if (enemyHealth->enemyHealth <= 0)
                        {
                            float& timer = deathTimers[id];
                            auto* anim = goc->GetComponentType<SpriteAnimationComponent>(
                                ComponentTypeId::CT_SpriteAnimationComponent);
                            const bool hasDeathAnimation = FindAnimationIndex(anim, "death") >= 0;
                            if (timer <= 0.0f)
                            {
                                if (!hasDeathAnimation)
                                {
                                    if (auto* render = goc->GetComponentType<RenderComponent>(
                                        ComponentTypeId::CT_RenderComponent))
                                    {
                                        render->visible = false;
                                    }
                                }

                                if (hasDeathAnimation)
                                    PlayAnimationIfAvailable(goc, "death");
                                EmitCombatAudio(combatAudioCallback, goc, CombatAudioEvent::EnemyDeath);
                                timer = std::max(AnimationDuration(anim, "death"), 0.2f);
                            }
                            else
                            {
                                timer = std::max(0.0f, timer - dt);
                            }

                            // Enemies without a death clip should still be removed once the timer expires.
                            const bool finished = !hasDeathAnimation || IsAnimationFinished(anim, "death");
                            if (timer <= 0.0f && finished)
                            {
                                FACTORY->Destroy(goc);
                                deathTimers.erase(id);
                                return true;
                            }

                            return false;
                        }
                        deathTimers.erase(id);
                    }

                    // ------------------------
                    // Player health
                    // ------------------------
                    if (auto* playerHealth = goc->GetComponentType<PlayerHealthComponent>(
                        ComponentTypeId::CT_PlayerHealthComponent))
                    {
                        if (playerHealth->isInvulnerable)
                        {
                            playerHealth->invulnTime = std::max(0.0f, playerHealth->invulnTime - dt);
                            if (playerHealth->invulnTime <= 0.0f)
                                playerHealth->isInvulnerable = false;
                        }

                        if (playerHealth->playerHealth <= 0 && deathTimers.find(id) == deathTimers.end())
                        {
                            playerHealth->isDead = true;
                            PlayAnimationIfAvailable(goc, "death");
                            if (!playerHealth->deathSoundPlayed)
                            {
                                EmitCombatAudio(combatAudioCallback, goc, CombatAudioEvent::PlayerDeath);
                                playerHealth->deathSoundPlayed = true;
                            }
                            deathTimers[id] = std::max(AnimationDuration(
                                goc->GetComponentType<SpriteAnimationComponent>(
                                    ComponentTypeId::CT_SpriteAnimationComponent), "death"), 0.2f);
                            return false;
                        }

                        if (playerHealth->isDead)
                        {
                            float& timer = deathTimers[id];
                            timer = std::max(0.0f, timer - dt);
                            auto* anim = goc->GetComponentType<SpriteAnimationComponent>(
                                ComponentTypeId::CT_SpriteAnimationComponent);
                            const bool finished = anim ? IsAnimationFinished(anim, "death") : true;
                            if (timer <= 0.0f && finished)
                            {
                                if (playerDeathCompleteCallback)
                                    playerDeathCompleteCallback();
                                FACTORY->Destroy(goc);
                                deathTimers.erase(id);
                                return true;
                            }
                            return false;
                        }
                    }

                    return false;
                }),
            gameObjectIds.end());
    }
    void HealthSystem::draw()
    {
        // Health presentation (HUD / enemy bars) is game-specific and now lives in Sandbox/MyGame.
    }

    void HealthSystem::Shutdown()
    {
        gameObjectIds.clear();
        deathTimers.clear();
    }

} // namespace Framework
