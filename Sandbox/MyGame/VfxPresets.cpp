/*********************************************************************************************
 \file      VfxPresets.cpp
 \par       SofaSpuds
 \author
 \brief     Implements sandbox-specific combat VFX presets and bindings.
 \details   Builds the current game's hit-impact sprite and particle effects, then
            binds them into engine combat callbacks from the game layer.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "VfxPresets.hpp"

#include "Common/ComponentTypeID.h"
#include "Component/RenderComponent.h"
#include "Component/SpriteAnimationComponent.h"
#include "Component/SpriteComponent.h"
#include "Component/TransformComponent.h"
#include "Core/PathUtils.h"
#include "Factory/Factory.h"
#include "Resource_Asset_Manager/Resource_Manager.h"
#include "Systems/LogicSystem.h"
#include "Systems/ParticleSystem.h"

#include <cmath>
#include <glm/vec2.hpp>
#include <random>
#include <string>
#include <string_view>

namespace mygame {
    namespace {
        constexpr std::string_view kImpactVfxName = "HitImpactVFX";
        constexpr std::string_view kImpactVfxTextureKey = "impact_vfx_sheet";
        HitImpactBurstPreset DefaultHitImpactBurstPreset()
        {
            return {};
        }

        HitImpactBurstPreset gHitImpactBurstPreset = DefaultHitImpactBurstPreset();

        void EnsureImpactTextureLoaded()
        {
            const auto path = Framework::ResolveAssetPath("Textures/Character/Ming_Sprite/ImpactVFX_Sprite.png");
            const std::string key{ kImpactVfxTextureKey };
            if (!Resource_Manager::getTexture(key))
            {
                Resource_Manager::load(key, path.string());
            }
        }

        Framework::GOC* SpawnHitImpactVfx(const glm::vec2& worldPos)
        {
            if (!Framework::FACTORY)
                return nullptr;

            EnsureImpactTextureLoaded();

            Framework::GOC* vfx = Framework::FACTORY->CreateEmptyComposition();
            if (!vfx)
                return nullptr;

            vfx->SetObjectName(std::string(kImpactVfxName));

            auto* tr = vfx->EmplaceComponent<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            tr->x = worldPos.x;
            tr->y = worldPos.y;

            auto* render = vfx->EmplaceComponent<Framework::RenderComponent>(
                Framework::ComponentTypeId::CT_RenderComponent);
            render->w = 0.25f;
            render->h = 0.25f;
            render->layer = 1;

            auto* sprite = vfx->EmplaceComponent<Framework::SpriteComponent>(
                Framework::ComponentTypeId::CT_SpriteComponent);
            sprite->texture_key = std::string(kImpactVfxTextureKey);
            sprite->texture_id = Resource_Manager::getTexture(sprite->texture_key);

            auto* anim = vfx->EmplaceComponent<Framework::SpriteAnimationComponent>(
                Framework::ComponentTypeId::CT_SpriteAnimationComponent);

            Framework::SpriteAnimationComponent::SpriteSheetAnimation impact{};
            impact.name = "impact";
            impact.textureKey = std::string(kImpactVfxTextureKey);
            impact.spriteSheetPath = Framework::ResolveAssetPath(
                "Textures/Character/Ming_Sprite/ImpactVFX_Sprite.png").string();
            impact.config.totalFrames = 8;
            impact.config.rows = 1;
            impact.config.columns = 8;
            impact.config.startFrame = 0;
            impact.config.endFrame = 7;
            impact.config.fps = 20.0f;
            impact.config.loop = false;
            impact.textureId = Resource_Manager::getTexture(impact.textureKey);

            anim->animations.push_back(impact);
            anim->activeAnimation = 0;
            anim->SetActiveAnimation(0);

            return vfx;
        }

        void SpawnHitImpactBurst(const glm::vec2& worldPos)
        {
            auto* particleSystem = Framework::ParticleSystem::Instance();
            if (!particleSystem)
                return;

            const HitImpactBurstPreset& preset = gHitImpactBurstPreset;
            if (preset.count <= 0)
                return;

            static std::mt19937 rng(std::random_device{}());
            std::uniform_real_distribution<float> angleDist(0.0f, 6.283185f);
            std::uniform_real_distribution<float> speedDist(
                std::min(preset.speedMin, preset.speedMax),
                std::max(preset.speedMin, preset.speedMax));
            std::uniform_real_distribution<float> lifeDist(
                std::min(preset.lifeMin, preset.lifeMax),
                std::max(preset.lifeMin, preset.lifeMax));
            std::uniform_real_distribution<float> radiusDist(
                std::min(preset.radiusMin, preset.radiusMax),
                std::max(preset.radiusMin, preset.radiusMax));
            std::uniform_real_distribution<float> offsetDist(
                std::min(preset.offsetMin, preset.offsetMax),
                std::max(preset.offsetMin, preset.offsetMax));

            for (int i = 0; i < preset.count; ++i)
            {
                const float angle = angleDist(rng);
                const float speed = speedDist(rng);
                const float radius = radiusDist(rng);

                Framework::ParticleSystem::CircleParticleSpec spec;
                spec.objectName = "HitImpactParticle";
                spec.position = {
                    worldPos.x + offsetDist(rng),
                    worldPos.y + offsetDist(rng)
                };
                spec.velocity = {
                    std::cos(angle) * speed,
                    std::sin(angle) * speed
                };
                spec.life = lifeDist(rng);
                spec.startRadius = radius;
                spec.endRadius = radius * preset.endRadiusScale;
                spec.r = preset.red;
                spec.g = preset.green;
                spec.b = preset.blue;
                spec.startAlpha = preset.startAlpha;
                spec.endAlpha = preset.endAlpha;

                particleSystem->SpawnCircleParticle(spec);
            }
        }
    } // namespace

    void BindCombatVfx(Framework::LogicSystem& logic)
    {
        if (!logic.hitBoxSystem)
            return;

        logic.hitBoxSystem->SetHitImpactVfxCallback(
            [](const glm::vec2& worldPos)
            {
                SpawnHitImpactVfx(worldPos);
                SpawnHitImpactBurst(worldPos);
            });
    }

    HitImpactBurstPreset& GetHitImpactBurstPreset()
    {
        return gHitImpactBurstPreset;
    }

    void ResetHitImpactBurstPreset()
    {
        gHitImpactBurstPreset = DefaultHitImpactBurstPreset();
    }

    void SpawnHitImpactPreview(const glm::vec2& worldPos)
    {
        SpawnHitImpactVfx(worldPos);
        SpawnHitImpactBurst(worldPos);
    }

    bool IsImpactVfxObject(const Framework::GOC* obj)
    {
        return obj && obj->GetObjectName() == kImpactVfxName;
    }
}
