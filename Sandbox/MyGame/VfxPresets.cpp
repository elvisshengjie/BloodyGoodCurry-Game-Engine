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
        constexpr std::string_view kHeiBangBeamVfxName = "HeiBangAttack2BeamVFX";
        constexpr std::string_view kHeiBangBeamTextureKey = "heibang_attack2_laser";
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

        void EnsureHeiBangBeamTextureLoaded()
        {
            const auto path = Framework::ResolveAssetPath(
                "Textures/Character/Hei Bang_Sprite/2nd Laser Beam_Sprite .png");
            const std::string key{ kHeiBangBeamTextureKey };
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

        Framework::GOC* SpawnHeiBangBeamVfxInternal(const Framework::GOC& owner, const glm::vec2& targetPos)
        {
            if (!Framework::FACTORY)
                return nullptr;

            auto* ownerTransform = owner.GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            if (!ownerTransform)
                return nullptr;

            auto* ownerRender = owner.GetComponentType<Framework::RenderComponent>(
                Framework::ComponentTypeId::CT_RenderComponent);
            const float ownerWidth = ownerRender
                ? std::fabs(ownerRender->w * ownerTransform->scaleX)
                : 0.3f;
            const float ownerHeight = ownerRender
                ? std::fabs(ownerRender->h * ownerTransform->scaleY)
                : 0.3f;
            const float facingSign = (ownerRender && ownerRender->w < 0.0f) ? -1.0f : 1.0f;
            // Tune the start point to HeiBang's open mouth in the last attack2 frame.
            const glm::vec2 beamStart{
                ownerTransform->x + (facingSign * ownerWidth * 0.38f),
                ownerTransform->y + (ownerHeight * 0.14f)
            };
            glm::vec2 beamDelta = targetPos - beamStart;
            float beamDistance = std::sqrt(beamDelta.x * beamDelta.x + beamDelta.y * beamDelta.y);
            if (beamDistance < 0.0001f)
            {
                beamDelta = { 1.0f, 0.0f };
                beamDistance = 1.0f;
            }
            const glm::vec2 beamDir = beamDelta / beamDistance;
            const float beamLength = std::max(ownerWidth * 2.6f, beamDistance + 0.18f);
            const glm::vec2 beamCenter = beamStart + beamDir * (beamLength * 0.5f);

            EnsureHeiBangBeamTextureLoaded();

            Framework::GOC* vfx = Framework::FACTORY->CreateEmptyComposition();
            if (!vfx)
                return nullptr;

            vfx->SetObjectName(std::string(kHeiBangBeamVfxName));
            vfx->SetLayerName(owner.GetLayerName());

            auto* tr = vfx->EmplaceComponent<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            tr->x = beamCenter.x;
            tr->y = beamCenter.y;
            tr->rot = std::atan2(beamDir.y, beamDir.x);
            tr->scaleX = 1.0f;
            tr->scaleY = 1.0f;

            auto* render = vfx->EmplaceComponent<Framework::RenderComponent>(
                Framework::ComponentTypeId::CT_RenderComponent);
            render->w = 0.3f;
            render->h = 0.3f;
            render->layer = 2;

            if (ownerRender)
            {
                render->w = beamLength;
                render->h = std::max(0.18f, ownerHeight * 0.95f);
                render->r = ownerRender->r;
                render->g = ownerRender->g;
                render->b = ownerRender->b;
                render->a = ownerRender->a;
                render->layer = ownerRender->layer + 1;
                render->blendMode = ownerRender->blendMode;
            }
            else
            {
                render->w = beamLength;
                render->h = 0.24f;
            }

            auto* sprite = vfx->EmplaceComponent<Framework::SpriteComponent>(
                Framework::ComponentTypeId::CT_SpriteComponent);
            sprite->texture_key = std::string(kHeiBangBeamTextureKey);
            sprite->texture_id = Resource_Manager::getTexture(sprite->texture_key);

            auto* anim = vfx->EmplaceComponent<Framework::SpriteAnimationComponent>(
                Framework::ComponentTypeId::CT_SpriteAnimationComponent);

            Framework::SpriteAnimationComponent::SpriteSheetAnimation beam{};
            beam.name = "attack2_laser";
            beam.textureKey = std::string(kHeiBangBeamTextureKey);
            beam.spriteSheetPath = Framework::ResolveAssetPath(
                "Textures/Character/Hei Bang_Sprite/2nd Laser Beam_Sprite .png").string();
            beam.config.totalFrames = 14;
            beam.config.rows = 1;
            beam.config.columns = 14;
            beam.config.startFrame = 0;
            beam.config.endFrame = 13;
            beam.config.fps = 12.0f;
            beam.config.loop = false;
            beam.textureId = Resource_Manager::getTexture(beam.textureKey);

            anim->animations.push_back(beam);
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

    Framework::GOC* SpawnHeiBangAttack2BeamVfx(const Framework::GOC& owner, const glm::vec2& targetPos)
    {
        return SpawnHeiBangBeamVfxInternal(owner, targetPos);
    }

    bool IsImpactVfxObject(const Framework::GOC* obj)
    {
        return obj && obj->GetObjectName() == kImpactVfxName;
    }

    bool IsHeiBangAttack2BeamVfxObject(const Framework::GOC* obj)
    {
        return obj && obj->GetObjectName() == kHeiBangBeamVfxName;
    }
}
