/*********************************************************************************************
 \file      HealthPresentation.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%
 \brief     Implements sandbox-specific health UI and defeat presentation logic.
 \details   Owns the game-side player defeat latch, HUD rendering, and enemy health
            bar drawing layered on top of the engine's generic HealthSystem.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "HealthPresentation.hpp"

#include "EngineCall.hpp"
#include "Factory/Factory.h"
#include "Components/PlayerHUD.h"
#include "Core/PathUtils.h"
#include "Graphics/Graphics.hpp"
#include "Runtime/HealthSystem.h"
#include "Systems/RenderSystem.h"
#include "Components/EnemyHealthComponent.h"
#include "Components/PlayerHealthComponent.h"
#include "Component/TransformComponent.h"
#include "Component/RenderComponent.h"
#include "Resource_Asset_Manager/Resource_Manager.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include "Graphics/GLHeaders.h"
#include <string>
#include <string_view>
#include <utility>

namespace mygame {
    namespace {
        bool gPlayerDefeated = false;
        bool gHeiBangDefeated = false;
        bool gNancieDefeated = false;
        float gLastHealthUiDt = 0.0f;
        unsigned gKeyUiTexture = 0u;
        bool gTriedLoadKeyUiTexture = false;
        unsigned gAimArrowTexture = 0u;
        bool gTriedLoadAimArrowTexture = false;
        unsigned gEnemyHealthFrameTexture = 0u;
        bool gTriedLoadEnemyHealthFrameTexture = false;
        unsigned gEnemyHealthSliderTexture = 0u;
        bool gTriedLoadEnemyHealthSliderTexture = false;

        constexpr float kEnemyHealthFrameTextureWidth = 274.0f;
        constexpr float kEnemyHealthFrameTextureHeight = 34.0f;
        constexpr float kEnemyHealthSliderTextureWidth = 264.0f;
        constexpr float kEnemyHealthSliderTextureHeight = 24.0f;
        constexpr float kEnemyHealthInnerInsetX =
            (kEnemyHealthFrameTextureWidth - kEnemyHealthSliderTextureWidth) * 0.5f;
        constexpr float kEnemyHealthInnerInsetY =
            (kEnemyHealthFrameTextureHeight - kEnemyHealthSliderTextureHeight) * 0.5f;

        /*************************************************************************************
         \brief  Lazily loads a UI texture from the active project and caches its handle.
         \param  cachedTexture  Stored OpenGL texture id.
         \param  attemptedLoad  Guard that prevents repeated failed load attempts.
         \param  textureKey     Resource-manager cache key.
         \param  texturePath    Relative project asset path.
         \return Loaded texture id, or 0 when loading fails.
        *************************************************************************************/
        unsigned ResolveUiTexture(unsigned& cachedTexture,
            bool& attemptedLoad,
            const char* textureKey,
            const char* texturePath)
        {
            if (cachedTexture != 0u || attemptedLoad)
                return cachedTexture;

            attemptedLoad = true;

            if (const unsigned cached = Resource_Manager::getTexture(textureKey))
            {
                cachedTexture = cached;
                return cachedTexture;
            }

            const std::string resolvedPath = Framework::ResolveAssetPath(texturePath).string();
            if (Resource_Manager::load(textureKey, resolvedPath))
                cachedTexture = Resource_Manager::getTexture(textureKey);

            return cachedTexture;
        }

        /*************************************************************************************
         \brief  Lazily loads and returns the key UI icon texture.
         \return OpenGL texture id, or 0 when loading fails.
        *************************************************************************************/
        unsigned ResolveKeyUiTexture()
        {
            return ResolveUiTexture(gKeyUiTexture, gTriedLoadKeyUiTexture,
                "hud_key_icon", "Textures/UI/Key.png");
        }

        /*************************************************************************************
         \brief  Lazily loads and returns the world-space aim arrow texture.
         \return OpenGL texture id, or 0 when loading fails.
        *************************************************************************************/
        unsigned ResolveAimArrowTexture()
        {
            return ResolveUiTexture(gAimArrowTexture, gTriedLoadAimArrowTexture,
                "hud_aim_arrow", "Textures/UI/Arrow.png");
        }

        /*************************************************************************************
         \brief  Lazily loads the static enemy health-bar frame texture.
         \return OpenGL texture id, or 0 when loading fails.
        *************************************************************************************/
        unsigned ResolveEnemyHealthFrameTexture()
        {
            return ResolveUiTexture(gEnemyHealthFrameTexture, gTriedLoadEnemyHealthFrameTexture,
                "enemy_health_frame", "Textures/UI/Health Bar/Enemy Health.png");
        }

        /*************************************************************************************
         \brief  Lazily loads the enemy health-bar slider fill texture.
         \return OpenGL texture id, or 0 when loading fails.
        *************************************************************************************/
        unsigned ResolveEnemyHealthSliderTexture()
        {
            return ResolveUiTexture(gEnemyHealthSliderTexture, gTriedLoadEnemyHealthSliderTexture,
                "enemy_health_slider", "Textures/UI/Health Bar/Enemy Health_Slider.png");
        }

        /*************************************************************************************
         \brief Performs a case-insensitive comparison for two ASCII-like strings.
         \param a First string.
         \param b Second string.
         \return True when both strings match ignoring ASCII letter case.
        *************************************************************************************/
        bool EqualsIgnoreCase(std::string_view a, std::string_view b)
        {
            if (a.size() != b.size())
                return false;

            for (std::size_t i = 0; i < a.size(); ++i)
            {
                if (std::tolower(static_cast<unsigned char>(a[i])) !=
                    std::tolower(static_cast<unsigned char>(b[i])))
                {
                    return false;
                }
            }

            return true;
        }

        /*************************************************************************************
         \brief  Draws the key inventory icon and count at left-center of the viewport.
         \param  render     Active render system (for text drawing checks).
         \param  viewportW  Current viewport width in pixels.
         \param  viewportH  Current viewport height in pixels.
        *************************************************************************************/
        void DrawKeyInventoryUi(Framework::RenderSystem& render, int viewportW, int viewportH)
        {
            const int keyCount = std::max(0, GetPlayerKeyCount());
            if (keyCount <= 0)
                return;

            const float refHeight = 720.0f;
            const float scale = std::max(0.6f, static_cast<float>(viewportH) / refHeight);

            const float iconH = 56.0f * scale;
            const float baseIconW = iconH * (2.0f / 3.0f);
            const float iconW = baseIconW * 0.9f;
            const float iconX = (18.0f * scale) + ((baseIconW - iconW) * 0.5f);
            const float iconY = (viewportH * 0.5f) - (iconH * 0.5f);

            if (const unsigned keyTexture = ResolveKeyUiTexture())
            {
                gfx::Graphics::renderSpriteUI(keyTexture, iconX, iconY, iconW, iconH,
                    1.0f, 1.0f, 1.0f, 1.0f, viewportW, viewportH);
            }

            if (render.IsTextReadyHint())
            {
                const std::string label = "x" + std::to_string(keyCount);
                const float textX = iconX + iconW + (8.0f * scale);
                const float textY = iconY + (iconH * 0.28f);
                render.GetTextHint().RenderText(label.c_str(), textX, textY,
                    0.95f * scale, { 1.0f, 1.0f, 1.0f });
            }
        }

        /*************************************************************************************
         \brief  Converts a world-space position into UI-space screen coordinates.
         \param  worldX     World-space x position.
         \param  worldY     World-space y position.
         \param  screenW    Target viewport width in pixels.
         \param  screenH    Target viewport height in pixels.
         \param  vpMatrix   World-view-projection matrix used for projection.
         \return The projected screen-space position, or an off-screen fallback.
        *************************************************************************************/
        std::pair<float, float> WorldToScreenUI(
            float worldX, float worldY, int screenW, int screenH, const glm::mat4& vpMatrix)
        {
            const glm::vec4 clipPos = vpMatrix * glm::vec4(worldX, worldY, 0.0f, 1.0f);

            if (clipPos.w == 0.0f)
                return { -100.0f, -100.0f };

            const glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
            const float x = (ndc.x * 0.5f + 0.5f) * screenW;
            const float y = (ndc.y * 0.5f + 0.5f) * screenH;
            return { x, y };
        }

        /*************************************************************************************
         \brief  Draws an aim arrow orbiting around the player using the current aim direction.
         \param  render          Active render system.
         \param  player          Player object composition.
         \param  transform       Player transform.
         \param  renderComponent Player render component for size/orbit estimates.
        *************************************************************************************/
        void DrawPlayerAimArrow(Framework::RenderSystem& render,
            Framework::GameObjectComposition* player,
            Framework::TransformComponent* transform,
            Framework::RenderComponent* renderComponent)
        {
            if (!(player && transform))
                return;

            const mygame::PlayerAimIndicatorState aimState =
                mygame::GetPlayerAimIndicatorState(player);
            if (!aimState.valid)
                return;

            const unsigned arrowTexture = ResolveAimArrowTexture();
            if (arrowTexture == 0u)
                return;

            const float halfW = renderComponent
                ? std::max(0.05f, std::abs(renderComponent->w * transform->scaleX) * 0.5f)
                : 0.12f;
            const float halfH = renderComponent
                ? std::max(0.05f, std::abs(renderComponent->h * transform->scaleY) * 0.5f)
                : 0.12f;

            const float orbitRadius = std::max(halfW, halfH) + 0.04f;
            const float arrowX = transform->x + aimState.dirX * orbitRadius;
            const float arrowY = transform->y + aimState.dirY * orbitRadius;
            const float arrowRotation = std::atan2(aimState.dirY, aimState.dirX);
            const float arrowScale = std::max(halfW, halfH) * 0.92f;

            gfx::Graphics::setViewProjection(glm::mat4(1.0f), render.GetWorldViewProjectionMatrix());
            gfx::Graphics::renderSprite(
                arrowTexture,
                arrowX,
                arrowY,
                arrowRotation,
                arrowScale,
                arrowScale,
                1.0f, 1.0f, 1.0f, 1.0f);
            gfx::Graphics::resetViewProjection();
        }

        /*************************************************************************************
         \brief  Draws a textured enemy health bar using a frame and clipped slider fill.
         \param  centerX      Screen-space x center in the active gameplay viewport.
         \param  centerY      Screen-space y center in the active gameplay viewport.
         \param  frameWidth   Desired frame width in pixels.
         \param  healthRatio  Current health normalized to [0,1].
         \param  viewportW    Active gameplay viewport width in pixels.
         \param  viewportH    Active gameplay viewport height in pixels.
        *************************************************************************************/
        void DrawEnemyHealthBarUi(float centerX, float centerY, float frameWidth, float healthRatio,
            int viewportW, int viewportH)
        {
            healthRatio = std::clamp(healthRatio, 0.0f, 1.0f);

            const unsigned frameTexture = ResolveEnemyHealthFrameTexture();
            const unsigned sliderTexture = ResolveEnemyHealthSliderTexture();

            const float frameHeight =
                frameWidth * (kEnemyHealthFrameTextureHeight / kEnemyHealthFrameTextureWidth);
            const float frameLeft = centerX - (frameWidth * 0.5f);
            const float frameBottom = centerY - (frameHeight * 0.5f);

            if (frameTexture == 0u && sliderTexture == 0u)
            {
                gfx::Graphics::renderRectangleUI(
                    frameLeft, frameBottom,
                    frameWidth, frameHeight,
                    0.2f, 0.2f, 0.2f, 1.0f,
                    viewportW, viewportH);

                gfx::Graphics::renderRectangleUI(
                    frameLeft, frameBottom,
                    frameWidth * healthRatio, frameHeight,
                    0.0f, 1.0f, 0.0f, 1.0f,
                    viewportW, viewportH);
                return;
            }

            if (frameTexture != 0u)
            {
                gfx::Graphics::renderSpriteUI(frameTexture,
                    frameLeft, frameBottom, frameWidth, frameHeight,
                    1.0f, 1.0f, 1.0f, 1.0f,
                    viewportW, viewportH);
            }

            if (sliderTexture == 0u || healthRatio <= 0.0f)
                return;

            const float insetX = frameWidth * (kEnemyHealthInnerInsetX / kEnemyHealthFrameTextureWidth);
            const float insetY = frameHeight * (kEnemyHealthInnerInsetY / kEnemyHealthFrameTextureHeight);
            const float sliderAreaWidth = std::max(0.0f, frameWidth - (insetX * 2.0f));
            const float sliderAreaHeight = std::max(0.0f, frameHeight - (insetY * 2.0f));
            const float visibleSliderWidth = sliderAreaWidth * healthRatio;

            if (visibleSliderWidth <= 0.0f || sliderAreaHeight <= 0.0f)
                return;

            gfx::Graphics::renderSpriteUISubRect(sliderTexture,
                frameLeft + insetX, frameBottom + insetY,
                visibleSliderWidth, sliderAreaHeight,
                0.0f, 0.0f, healthRatio, 1.0f,
                1.0f, 1.0f, 1.0f, 1.0f,
                viewportW, viewportH);
        }
    } // namespace

    /*************************************************************************************
     \brief  Binds the game-side defeat latch to the engine HealthSystem.
     \param  health  The HealthSystem that emits player-death completion events.
    *************************************************************************************/
    void BindHealthPresentation(Framework::HealthSystem& health)
    {
        health.SetPlayerDeathCompleteCallback([]() { gPlayerDefeated = true; });
        health.SetEnemyDeathCompleteCallback([](Framework::GOC* enemy)
        {
            if (enemy && EqualsIgnoreCase(enemy->GetObjectName(), "heibang"))
                gHeiBangDefeated = true;
            if (enemy && EqualsIgnoreCase(enemy->GetObjectName(), "nancie"))  // ADD THIS
                gNancieDefeated = true;
        });
    }

    /*************************************************************************************
     \brief  Stores the latest UI delta time for HUD animation updates.
     \param  dt  Delta time in seconds.
    *************************************************************************************/
    void UpdateHealthPresentationDelta(float dt)
    {
        gLastHealthUiDt = dt;
    }

    /*************************************************************************************
     \brief  Reports whether the player defeat flow has been latched.
     \return True once the player's death sequence has completed.
    *************************************************************************************/
    bool IsPlayerDefeated()
    {
        return gPlayerDefeated;
    }

    /*************************************************************************************
     \brief  Clears the game-side player defeat latch.
    *************************************************************************************/
    void ResetPlayerDefeat()
    {
        gPlayerDefeated = false;
    }
    /*************************************************************************************
      \brief  Reports whether Nancie's death sequence has completed.
      \return True once Nancie has finished the death flow and 
      is ready to trigger Boss music fade out.
     *************************************************************************************/
    bool IsNancieDefeated()
    {
        return gNancieDefeated;
    }
    /*************************************************************************************
      \brief  Clears the game-side Nancy victory latch.
    *************************************************************************************/
    void ResetNancieDefeat()
    {
        gNancieDefeated = false;
    }
    /*************************************************************************************
     \brief  Reports whether HeiBang's death sequence has completed.
     \return True once HeiBang has finished the death flow and is ready to trigger victory.
    *************************************************************************************/
    bool IsHeiBangDefeated()
    {
        return gHeiBangDefeated;
    }

    /*************************************************************************************
     \brief  Clears the game-side HeiBang victory latch.
    *************************************************************************************/
    void ResetHeiBangDefeat()
    {
        gHeiBangDefeated = false;
    }

    /*************************************************************************************
     \brief  Draws the player HUD and enemy health bars for the current frame.
     \param  render  The active RenderSystem used for viewport and matrix queries.
    *************************************************************************************/
    void DrawHealthPresentation(Framework::RenderSystem& render)
    {
        if (!Framework::FACTORY)
            return;

        int viewportX = 0;
        int viewportY = 0;
        int viewportW = render.ScreenWidth();
        int viewportH = render.ScreenHeight();
        const bool hasViewport = render.GetGameViewportRect(viewportX, viewportY, viewportW, viewportH);

        if (viewportW <= 0 || viewportH <= 0)
            return;

        if (!hasViewport)
        {
            viewportX = 0;
            viewportY = 0;
            viewportW = render.ScreenWidth();
            viewportH = render.ScreenHeight();
        }

        glViewport(viewportX, viewportY, viewportW, viewportH);

        for (const auto& [id, gocPtr] : Framework::FACTORY->Objects())
        {
            (void)id;
            if (!gocPtr)
                continue;

            auto* playerHealth = gocPtr->GetComponentType<Framework::PlayerHealthComponent>(
                Framework::ComponentTypeId::CT_PlayerHealthComponent);
            if (!playerHealth)
                continue;

            auto* hud = gocPtr->GetComponentType<Framework::PlayerHUDComponent>(
                Framework::ComponentTypeId::CT_PlayerHUDComponent);
            if (!hud)
                continue;

            auto* transform = gocPtr->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            auto* renderComponent = gocPtr->GetComponentType<Framework::RenderComponent>(
                Framework::ComponentTypeId::CT_RenderComponent);

            DrawPlayerAimArrow(render, gocPtr.get(), transform, renderComponent);

            hud->Update(gLastHealthUiDt);
            hud->Draw(viewportW, viewportH);
        }

        for (const auto& [id, gocPtr] : Framework::FACTORY->Objects())
        {
            (void)id;
            if (!gocPtr)
                continue;

            auto* enemyHealth = gocPtr->GetComponentType<Framework::EnemyHealthComponent>(
                Framework::ComponentTypeId::CT_EnemyHealthComponent);
            auto* transform = gocPtr->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            auto* renderComponent = gocPtr->GetComponentType<Framework::RenderComponent>(
                Framework::ComponentTypeId::CT_RenderComponent);

            if (!enemyHealth || !transform)
                continue;

            const float scaleY = std::max(1.0f, std::fabs(transform->scaleY));
            float baseHeight = scaleY;
            if (renderComponent)
            {
                baseHeight = std::max(1.0f, std::fabs(renderComponent->h * transform->scaleY));
            }

            const float worldOffsetY = (baseHeight * 0.5f) * 0.35f;
            const auto screenPos = WorldToScreenUI(
                transform->x, transform->y + worldOffsetY, viewportW, viewportH,
                render.GetWorldViewProjectionMatrix());

            float healthRatio = 0.0f;
            if (enemyHealth->enemyMaxhealth > 0)
            {
                healthRatio = static_cast<float>(enemyHealth->enemyHealth) /
                    static_cast<float>(enemyHealth->enemyMaxhealth);
            }
            healthRatio = std::clamp(healthRatio, 0.0f, 1.0f);

            const bool isHeiBang = EqualsIgnoreCase(gocPtr->GetObjectName(), "heibang");
            const float barWidth = isHeiBang ? viewportW * 0.34f : viewportW * 0.05f;
            const float barCenterX = isHeiBang ? (viewportW * 0.5f) : screenPos.first;
            const float barCenterY = isHeiBang ? (viewportH * 0.08f) : screenPos.second;

            DrawEnemyHealthBarUi(
                barCenterX, barCenterY,
                barWidth, healthRatio,
                viewportW, viewportH);
        }

        DrawKeyInventoryUi(render, viewportW, viewportH);

        glViewport(0, 0, render.ScreenWidth(), render.ScreenHeight());
    }

} // namespace mygame

