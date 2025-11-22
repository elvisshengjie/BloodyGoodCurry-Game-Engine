#include "HealthSystem.h"
#include "Factory/Factory.h"
#include "Component/SpriteAnimationComponent.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string_view>

namespace Framework
{
    namespace
    {
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
    } // anonymous namespace

    HealthSystem::HealthSystem(gfx::Window& win)
        : window(&win)
    {
    }

    void HealthSystem::Initialize()
    {
        // Track by ID instead of raw pointers to avoid dangling references.
        gameObjectIds.clear();
        deathTimers.clear();

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

            // Only track objects that actually have a health component.
            if (enemyHealth || playerHealth)
                gameObjectIds.push_back(id);
        }
    }

    void HealthSystem::Update(float dt)
    {
        gameObjectIds.erase(
            std::remove_if(
                gameObjectIds.begin(),
                gameObjectIds.end(),
                [this, dt](GOCId id) -> bool
                {
                    // Re-resolve each frame; if gone, drop it safely.
                    GOC* goc = FACTORY->GetObjectWithId(id);
                    if (!goc)
                    {
                        // Object was destroyed elsewhere; clean up any death timer.
                        deathTimers.erase(id);
                        return true;
                    }

                    // ---------------------------------------------------------
                    // Enemy health handling (plays death animation before destroy)
                    // ---------------------------------------------------------
                    if (auto* enemyHealth =
                        goc->GetComponentType<EnemyHealthComponent>(
                            ComponentTypeId::CT_EnemyHealthComponent))
                    {
                        if (enemyHealth->enemyHealth <= 0)
                        {
                            float& timer = deathTimers[id];

                            // First frame after "death" ¡ª trigger death animation and compute duration.
                            if (timer <= 0.0f)
                            {
                                PlayAnimationIfAvailable(goc, "death");

                                auto* anim =
                                    goc->GetComponentType<SpriteAnimationComponent>(
                                        ComponentTypeId::CT_SpriteAnimationComponent);

                                // Use animation length if available; otherwise fall back to a minimum.
                                timer = std::max(AnimationDuration(anim, "death"), 0.2f);
                            }
                            else
                            {
                                // Count down until we actually destroy the object.
                                timer = std::max(0.0f, timer - dt);
                            }

                            // When the timer has elapsed, destroy the enemy.
                            if (timer <= 0.0f)
                            {
                                FACTORY->Destroy(goc);
                                deathTimers.erase(id);

                                std::cout << "[HealthSystem] Enemy "
                                    << goc->GetId()
                                    << " destroyed.\n";
                                return true; // remove from tracked IDs
                            }

                            // Keep the object for now so the death animation can finish.
                            return false;
                        }

                        // Enemy is still alive; make sure we don't keep a stale timer.
                        deathTimers.erase(id);
                    }

                    // ---------------------------------------------------------
                    // Player health handling (instant destroy when health <= 0)
                    // ---------------------------------------------------------
                    if (auto* playerHealth =
                        goc->GetComponentType<PlayerHealthComponent>(
                            ComponentTypeId::CT_PlayerHealthComponent))
                    {
                        if (playerHealth->playerHealth <= 0)
                        {
                            FACTORY->Destroy(goc);

                            std::cout << "[HealthSystem] Player "
                                << goc->GetId()
                                << " destroyed.\n";
                            return true; // remove from tracked IDs
                        }
                    }

                    // Keep tracking this ID.
                    return false;
                }),
            gameObjectIds.end());
    }

    void HealthSystem::draw()
    {
        // Currently no UI/visuals for health. Rendering of health bars or
        // damage indicators could be added here in the future.
    }

    void HealthSystem::Shutdown()
    {
        gameObjectIds.clear();
        deathTimers.clear();
    }

} // namespace Framework
