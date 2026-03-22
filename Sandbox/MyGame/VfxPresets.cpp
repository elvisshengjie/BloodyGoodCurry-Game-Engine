/*********************************************************************************************
 \file      VfxPresets.cpp
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Primary Author, 100%
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
        constexpr std::string_view kFireImpactVfxName = "FireImpactVFX";
        constexpr std::string_view kFireImpactVfxTextureKey = "fire_impact_vfx_sheet";
        constexpr std::string_view kHeiBangBeamVfxName = "HeiBangAttack2BeamVFX";
        constexpr std::string_view kHeiBangBeamTextureKey = "heibang_attack2_laser";

        /*************************************************************************************
          \brief Builds the default hit-impact burst preset values.
          \return Default-initialized hit-impact particle settings.
        *************************************************************************************/
        HitImpactBurstPreset DefaultHitImpactBurstPreset()
        {
            return {};
        }

        HitImpactBurstPreset gHitImpactBurstPreset = DefaultHitImpactBurstPreset();

        /*************************************************************************************
          \brief Ensures the hit-impact sprite sheet is loaded into the resource manager.
        *************************************************************************************/
        void EnsureImpactTextureLoaded()
        {
            const auto path = Framework::ResolveAssetPath("Textures/Character/Ming_Sprite/ImpactVFX_Sprite.png");
            const std::string key{ kImpactVfxTextureKey };
            if (!Resource_Manager::getTexture(key))
            {
                Resource_Manager::load(key, path.string());
            }
        }

        /*************************************************************************************
          \brief Ensures HeiBang's attack2 beam sprite sheet is loaded into the resource manager.
        *************************************************************************************/
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

        /*************************************************************************************
          \brief Ensures the fire-impact sprite sheet is loaded into the resource manager.
        *************************************************************************************/
        void EnsureFireImpactTextureLoaded()
        {
            const auto path = Framework::ResolveAssetPath(
                "Textures/Character/Fire Enemy_Sprite/Fire Impact_Sprite.png");
            const std::string key{ kFireImpactVfxTextureKey };
            if (!Resource_Manager::getTexture(key))
            {
                Resource_Manager::load(key, path.string());
            }
        }

        /*************************************************************************************
          \brief Spawns the animated hit-impact sprite VFX at a world position.
          \param worldPos World-space position where the impact should appear.
          \return Newly created VFX object, or nullptr when creation fails.
        *************************************************************************************/
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

        /*************************************************************************************
          \brief Spawns HeiBang's attack2 beam VFX aimed toward a target position.
          \param owner     Owning game object used for origin, facing, and layer data.
          \param targetPos World-space target used to orient and size the beam.
          \return Newly created beam VFX object, or nullptr when creation fails.
        *************************************************************************************/
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
            beam.config.fps = 9.0f;
            beam.config.loop = false;
            beam.textureId = Resource_Manager::getTexture(beam.textureKey);

            anim->animations.push_back(beam);
            anim->activeAnimation = 0;
            anim->SetActiveAnimation(0);

            return vfx;
        }

        /*************************************************************************************
          \brief Spawns the animated fire-impact sprite VFX at a world position.
          \param worldPos World-space position where the impact should appear.
          \return Newly created VFX object, or nullptr when creation fails.
        *************************************************************************************/
        Framework::GOC* SpawnFireImpactVfxInternal(const glm::vec2& worldPos)
        {
            if (!Framework::FACTORY)
                return nullptr;

            EnsureFireImpactTextureLoaded();

            Framework::GOC* vfx = Framework::FACTORY->CreateEmptyComposition();
            if (!vfx)
                return nullptr;

            vfx->SetObjectName(std::string(kFireImpactVfxName));

            auto* tr = vfx->EmplaceComponent<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            tr->x = worldPos.x;
            tr->y = worldPos.y;

            auto* render = vfx->EmplaceComponent<Framework::RenderComponent>(
                Framework::ComponentTypeId::CT_RenderComponent);
            render->w = 0.28f;
            render->h = 0.28f;
            render->layer = 2;

            auto* sprite = vfx->EmplaceComponent<Framework::SpriteComponent>(
                Framework::ComponentTypeId::CT_SpriteComponent);
            sprite->texture_key = std::string(kFireImpactVfxTextureKey);
            sprite->texture_id = Resource_Manager::getTexture(sprite->texture_key);

            auto* anim = vfx->EmplaceComponent<Framework::SpriteAnimationComponent>(
                Framework::ComponentTypeId::CT_SpriteAnimationComponent);

            Framework::SpriteAnimationComponent::SpriteSheetAnimation impact{};
            impact.name = "fire_impact";
            impact.textureKey = std::string(kFireImpactVfxTextureKey);
            impact.spriteSheetPath = Framework::ResolveAssetPath(
                "Textures/Character/Fire Enemy_Sprite/Fire Impact_Sprite.png").string();
            impact.config.totalFrames = 11;
            impact.config.rows = 1;
            impact.config.columns = 11;
            impact.config.startFrame = 0;
            impact.config.endFrame = 10;
            impact.config.fps = 18.0f;
            impact.config.loop = false;
            impact.textureId = Resource_Manager::getTexture(impact.textureKey);

            anim->animations.push_back(impact);
            anim->activeAnimation = 0;
            anim->SetActiveAnimation(0);

            return vfx;
        }

        /*************************************************************************************
          \brief Spawns the preset-driven hit-impact particle burst at a world position.
          \param worldPos World-space position used as the particle burst origin.
        *************************************************************************************/
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

    /*************************************************************************************
      \brief Binds the game-specific combat VFX callbacks into the engine hitbox system.
      \param logic Active LogicSystem containing the gameplay hitbox system.
    *************************************************************************************/
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

    /*************************************************************************************
      \brief Returns the mutable hit-impact burst preset used by the current game.
      \return Reference to the shared hit-impact particle preset.
    *************************************************************************************/
    HitImpactBurstPreset& GetHitImpactBurstPreset()
    {
        return gHitImpactBurstPreset;
    }

    /*************************************************************************************
      \brief Restores the hit-impact burst preset back to its default values.
    *************************************************************************************/
    void ResetHitImpactBurstPreset()
    {
        gHitImpactBurstPreset = DefaultHitImpactBurstPreset();
    }

    /*************************************************************************************
      \brief Spawns both hit-impact VFX layers for editor or debug previewing.
      \param worldPos World-space preview position for the spawned VFX.
    *************************************************************************************/
    void SpawnHitImpactPreview(const glm::vec2& worldPos)
    {
        SpawnHitImpactVfx(worldPos);
        SpawnHitImpactBurst(worldPos);
    }

    /*************************************************************************************
      \brief Public wrapper that spawns HeiBang's attack2 beam effect.
      \param owner     Owning game object used as the beam source.
      \param targetPos World-space target used to orient the beam.
      \return Newly created beam VFX object, or nullptr when creation fails.
    *************************************************************************************/
    Framework::GOC* SpawnHeiBangAttack2BeamVfx(const Framework::GOC& owner, const glm::vec2& targetPos)
    {
        return SpawnHeiBangBeamVfxInternal(owner, targetPos);
    }

    /*************************************************************************************
      \brief Public wrapper that spawns a fire impact effect.
      \param worldPos World-space position for the effect.
      \return Newly created fire impact VFX object, or nullptr when creation fails.
    *************************************************************************************/
    Framework::GOC* SpawnFireImpactVfx(const glm::vec2& worldPos)
    {
        return SpawnFireImpactVfxInternal(worldPos);
    }

    /*************************************************************************************
      \brief Checks whether a game object is the spawned hit-impact sprite VFX.
      \param obj Object pointer to inspect.
      \return True when the object name matches the hit-impact VFX tag.
    *************************************************************************************/
    bool IsImpactVfxObject(const Framework::GOC* obj)
    {
        return obj && obj->GetObjectName() == kImpactVfxName;
    }

    /*************************************************************************************
      \brief Checks whether a game object is HeiBang's attack2 beam VFX.
      \param obj Object pointer to inspect.
      \return True when the object name matches the beam VFX tag.
    *************************************************************************************/
    bool IsHeiBangAttack2BeamVfxObject(const Framework::GOC* obj)
    {
        return obj && obj->GetObjectName() == kHeiBangBeamVfxName;
    }

    /*************************************************************************************
      \brief Checks whether a game object is the spawned fire-impact VFX.
      \param obj Object pointer to inspect.
      \return True when the object name matches the fire-impact VFX tag.
    *************************************************************************************/
    bool IsFireImpactVfxObject(const Framework::GOC* obj)
    {
        return obj && obj->GetObjectName() == kFireImpactVfxName;
    }
}
