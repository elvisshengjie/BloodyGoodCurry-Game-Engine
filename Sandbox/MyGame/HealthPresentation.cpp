/*********************************************************************************************
 \file      HealthPresentation.cpp
 \par       SofaSpuds
 \author
 \brief     Implements sandbox-specific health UI and defeat presentation logic.
 \details   Owns the game-side player defeat latch, HUD rendering, and enemy health
            bar drawing layered on top of the engine's generic HealthSystem.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "HealthPresentation.hpp"

#include "Factory/Factory.h"
#include "Components/PlayerHUD.h"
#include "Core/PathUtils.h"
#include "Graphics/Graphics.hpp"
#include "Systems/HealthSystem.h"
#include "Systems/RenderSystem.h"
#include "Component/PlayerHealthComponent.h"
#include "Component/EnemyHealthComponent.h"
#include "Component/TransformComponent.h"
#include "Component/RenderComponent.h"
#include "Resource_Asset_Manager/Resource_Manager.h"

#include <algorithm>
#include <cmath>
#include <glad/glad.h>
#include <string>
#include <utility>

namespace mygame {
    int GetPlayerKeyCount();
}

namespace mygame {
    namespace {
        bool gPlayerDefeated = false;
        float gLastHealthUiDt = 0.0f;
        unsigned gKeyUiTexture = 0u;
        bool gTriedLoadKeyUiTexture = false;

        /*************************************************************************************
         \brief  Lazily loads and returns the key UI icon texture.
         \return OpenGL texture id, or 0 when loading fails.
        *************************************************************************************/
        unsigned ResolveKeyUiTexture()
        {
            if (gKeyUiTexture != 0u || gTriedLoadKeyUiTexture)
                return gKeyUiTexture;

            gTriedLoadKeyUiTexture = true;
            constexpr const char* kTextureKey = "hud_key_icon";

            if (const unsigned cached = Resource_Manager::getTexture(kTextureKey))
            {
                gKeyUiTexture = cached;
                return gKeyUiTexture;
            }

            const std::string texturePath = Framework::ResolveAssetPath("Textures/UI/Key.png").string();
            if (Resource_Manager::load(kTextureKey, texturePath))
                gKeyUiTexture = Resource_Manager::getTexture(kTextureKey);

            return gKeyUiTexture;
        }

        /*************************************************************************************
         \brief  Draws the key inventory icon and count at left-center of the viewport.
         \param  render     Active render system (for text drawing checks).
         \param  viewportW  Current viewport width in pixels.
         \param  viewportH  Current viewport height in pixels.
        *************************************************************************************/
        void DrawKeyInventoryUi(Framework::RenderSystem& render, int viewportW, int viewportH)
        {
            const float refHeight = 720.0f;
            const float scale = std::max(0.6f, static_cast<float>(viewportH) / refHeight);

            const float iconW = 56.0f * scale;
            const float iconH = 56.0f * scale;
            const float iconX = 18.0f * scale;
            const float iconY = (viewportH * 0.5f) - (iconH * 0.5f);

            if (const unsigned keyTexture = ResolveKeyUiTexture())
            {
                gfx::Graphics::renderSpriteUI(keyTexture, iconX, iconY, iconW, iconH,
                    1.0f, 1.0f, 1.0f, 1.0f, viewportW, viewportH);
            }

            if (render.IsTextReadyHint())
            {
                const std::string label = "x" + std::to_string(std::max(0, GetPlayerKeyCount()));
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
    } // namespace

    /*************************************************************************************
     \brief  Binds the game-side defeat latch to the engine HealthSystem.
     \param  health  The HealthSystem that emits player-death completion events.
    *************************************************************************************/
    void BindHealthPresentation(Framework::HealthSystem& health)
    {
        health.SetPlayerDeathCompleteCallback([]() { gPlayerDefeated = true; });
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

            const float barWidth = viewportW * 0.05f;
            const float barHeight = viewportH * 0.015f;

            gfx::Graphics::renderRectangleUI(
                screenPos.first - barWidth * 0.5f, screenPos.second - barHeight * 0.5f,
                barWidth, barHeight,
                0.2f, 0.2f, 0.2f, 1.0f,
                viewportW, viewportH);

            gfx::Graphics::renderRectangleUI(
                screenPos.first - barWidth * 0.5f, screenPos.second - barHeight * 0.5f,
                barWidth * healthRatio, barHeight,
                0.0f, 1.0f, 0.0f, 1.0f,
                viewportW, viewportH);
        }

        DrawKeyInventoryUi(render, viewportW, viewportH);

        glViewport(0, 0, render.ScreenWidth(), render.ScreenHeight());
    }

} // namespace mygame
