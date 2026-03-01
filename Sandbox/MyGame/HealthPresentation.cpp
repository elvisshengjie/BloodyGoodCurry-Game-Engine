#include "HealthPresentation.hpp"

#include "Factory/Factory.h"
#include "Graphics/PlayerHUD.h"
#include "Graphics/Graphics.hpp"
#include "Systems/HealthSystem.h"
#include "Systems/RenderSystem.h"
#include "Component/PlayerHealthComponent.h"
#include "Component/EnemyHealthComponent.h"
#include "Component/TransformComponent.h"
#include "Component/RenderComponent.h"

#include <algorithm>
#include <cmath>
#include <glad/glad.h>
#include <utility>

namespace mygame {
    namespace {
        bool gPlayerDefeated = false;
        float gLastHealthUiDt = 0.0f;

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

    void BindHealthPresentation(Framework::HealthSystem& health)
    {
        health.SetPlayerDeathCompleteCallback([]() { gPlayerDefeated = true; });
    }

    void UpdateHealthPresentationDelta(float dt)
    {
        gLastHealthUiDt = dt;
    }

    bool IsPlayerDefeated()
    {
        return gPlayerDefeated;
    }

    void ResetPlayerDefeat()
    {
        gPlayerDefeated = false;
    }

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

        glViewport(0, 0, render.ScreenWidth(), render.ScreenHeight());
    }

} // namespace mygame
