/*********************************************************************************************
 \file      ParticleSystem.cpp
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Primary Author, 100%
 \brief     Implements a lightweight particle system for one-off particles.
 \details   Spawns and updates short-lived circle/sprite particles with
            Factory-managed lifetime. Game-specific presets are owned by the
            sandbox/game layer.
 \copyright
            All content (c) 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "Systems/ParticleSystem.h"

#include "Component/CircleRenderComponent.h"
#include "Component/TransformComponent.h"
#include "Component/RenderComponent.h"
#include "Component/SpriteComponent.h"
#include "Common/ComponentTypeID.h"
#include "Factory/Factory.h"
#include "Resource_Asset_Manager/Resource_Manager.h"
#include "Core/PathUtils.h"

#include <algorithm>
#include "Common/CRTDebug.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif

namespace Framework {

    ParticleSystem* ParticleSystem::instance = nullptr;

    /*****************************************************************************************
     \brief Constructs the particle system and registers the global singleton pointer.
     \details
        Assigns ParticleSystem::instance to this object so callers can access the
        system via ParticleSystem::Instance().
     \note
        This assumes only one ParticleSystem instance exists at a time.
    *****************************************************************************************/
    ParticleSystem::ParticleSystem()
    {
        instance = this;
    }

    /*****************************************************************************************
     \brief Returns the active ParticleSystem singleton instance.
     \return Pointer to the current ParticleSystem, or nullptr if none exists.
    *****************************************************************************************/
    ParticleSystem* ParticleSystem::Instance()
    {
        return instance;
    }

    /*****************************************************************************************
     \brief Initializes particle runtime state.
     \details
        Clears any previously tracked particles so the system starts empty for the
        current scene/run.
    *****************************************************************************************/
    void ParticleSystem::Initialize()
    {
        particles.clear();
    }

    /*****************************************************************************************
     \brief Shuts down the particle system and clears runtime state.
     \details
        - Clears all tracked particles.
        - Releases the singleton pointer if it points to this instance.
     \note
        Particle GameObjects are Factory-owned; the system only tracks IDs.
    *****************************************************************************************/
    void ParticleSystem::Shutdown()
    {
        particles.clear();
        if (instance == this)
        {
            instance = nullptr;
        }
    }

    /*****************************************************************************************
     \brief Updates all active particles (movement, fading, and lifetime).
     \param dt Delta time in seconds.
     \details
        For each tracked particle:
        - Fetch the particle GameObject from the Factory.
        - Decrease lifetime and destroy the object when expired.
        - Validate required components exist (Transform + CircleRender or Render+Sprite).
        - Integrate velocity into position and interpolate size/alpha over lifetime.
        - Apply simple damping to velocity.
     \note
        Particles are implemented as normal GameObjects created/destroyed via Factory,
        with this system maintaining a lightweight list of IDs + per-particle state.
    *****************************************************************************************/
    void ParticleSystem::Update(float dt)
    {
        if (!FACTORY)
            return;

        auto eraseParticle = [&](std::size_t index)
            {
                particles[index] = particles.back();
                particles.pop_back();
            };

        std::size_t index = 0;
        while (index < particles.size())
        {
            Particle& particle = particles[index];
            GOC* obj = FACTORY->GetObjectWithId(particle.id);
            if (!obj)
            {
                eraseParticle(index);
                continue;
            }

            particle.life = std::max(0.0f, particle.life - dt);
            if (particle.life <= 0.0f)
            {
                FACTORY->Destroy(obj);
                eraseParticle(index);
                continue;
            }

            auto* tr = obj->GetComponentType<TransformComponent>(
                ComponentTypeId::CT_TransformComponent);
            auto* circle = obj->GetComponentType<CircleRenderComponent>(
                ComponentTypeId::CT_CircleRenderComponent);
            auto* rc = obj->GetComponentType<RenderComponent>(
                ComponentTypeId::CT_RenderComponent);
            auto* sprite = obj->GetComponentType<SpriteComponent>(
                ComponentTypeId::CT_SpriteComponent);

            const bool wantsSprite = (particle.visual == ParticleVisual::Sprite);
            const bool hasSprite = (rc && sprite);
            const bool hasCircle = (circle != nullptr);

            if (!tr || (wantsSprite ? !hasSprite : !hasCircle))
            {
                FACTORY->Destroy(obj);
                eraseParticle(index);
                continue;
            }

            tr->x += particle.velocity.x * dt;
            tr->y += particle.velocity.y * dt;

            const float t = 1.0f - (particle.life / std::max(particle.totalLife, 0.001f));
            if (wantsSprite)
            {
                const float size = particle.startSize + (particle.endSize - particle.startSize) * t;
                rc->w = size;
                rc->h = size;
                rc->a = particle.startAlpha + (particle.endAlpha - particle.startAlpha) * t;
            }
            else
            {
                circle->radius = particle.startRadius + (particle.endRadius - particle.startRadius) * t;
                circle->a = particle.startAlpha + (particle.endAlpha - particle.startAlpha) * t;
            }

            particle.velocity *= (1.0f - std::min(dt * 1.5f, 0.9f));

            ++index;
        }
    }

    /*****************************************************************************************
     \brief Spawns a circle-rendered particle GameObject.
     \param spec Circle particle specification (position, color, velocity, lifetime, etc.).
     \details
        - Creates an empty GameObject via Factory.
        - Attaches TransformComponent + CircleRenderComponent.
        - Initializes rendering values (radius + RGBA).
        - Records a Particle entry so Update() can drive lifetime and interpolation.
     \note
        If the Factory is unavailable or spec.life <= 0, this function does nothing.
    *****************************************************************************************/
    void ParticleSystem::SpawnCircleParticle(const CircleParticleSpec& spec)
    {
        if (!FACTORY || spec.life <= 0.0f)
            return;

        GOC* particleObj = FACTORY->CreateEmptyComposition();
        if (!particleObj)
            return;

        particleObj->SetObjectName(spec.objectName.empty() ? "Particle" : spec.objectName);

        auto* tr = particleObj->EmplaceComponent<TransformComponent>(
            ComponentTypeId::CT_TransformComponent);
        tr->x = spec.position.x;
        tr->y = spec.position.y;

        auto* circle = particleObj->EmplaceComponent<CircleRenderComponent>(
            ComponentTypeId::CT_CircleRenderComponent);
        circle->radius = spec.startRadius;
        circle->r = spec.r;
        circle->g = spec.g;
        circle->b = spec.b;
        circle->a = spec.startAlpha;

        Particle particle{};
        particle.id = particleObj->GetId();
        particle.visual = ParticleVisual::Circle;
        particle.velocity = spec.velocity;
        particle.totalLife = spec.life;
        particle.life = spec.life;
        particle.startRadius = spec.startRadius;
        particle.endRadius = spec.endRadius;
        particle.startAlpha = spec.startAlpha;
        particle.endAlpha = spec.endAlpha;

        particles.push_back(particle);
    }

    /*****************************************************************************************
     \brief Spawns a sprite-rendered particle GameObject.
     \param spec Sprite particle specification (texture, size/alpha ranges, lifetime, etc.).
     \details
        - Ensures the referenced texture is loaded (loads from texturePath if needed).
        - Creates an empty GameObject via Factory.
        - Attaches TransformComponent + RenderComponent + SpriteComponent.
        - Initializes render size/color/alpha and sprite texture bindings.
        - Records a Particle entry so Update() can drive lifetime and interpolation.
     \note
        If the Factory is unavailable, spec.life <= 0, or spec.textureKey is empty,
        this function does nothing.
    *****************************************************************************************/
    void ParticleSystem::SpawnSpriteParticle(const SpriteParticleSpec& spec)
    {
        if (!FACTORY || spec.life <= 0.0f || spec.textureKey.empty())
            return;

        if (!Resource_Manager::getTexture(spec.textureKey) && !spec.texturePath.empty())
        {
            const auto resolved = ResolveAssetPath(spec.texturePath);
            const std::string pathStr = resolved.empty() ? spec.texturePath : resolved.string();
            Resource_Manager::load(spec.textureKey, pathStr);
        }

        GOC* particleObj = FACTORY->CreateEmptyComposition();
        if (!particleObj)
            return;

        particleObj->SetObjectName(spec.objectName.empty() ? "Particle" : spec.objectName);

        auto* tr = particleObj->EmplaceComponent<TransformComponent>(
            ComponentTypeId::CT_TransformComponent);
        tr->x = spec.position.x;
        tr->y = spec.position.y;

        auto* rc = particleObj->EmplaceComponent<RenderComponent>(
            ComponentTypeId::CT_RenderComponent);
        rc->w = spec.startSize;
        rc->h = spec.startSize;
        rc->r = spec.r;
        rc->g = spec.g;
        rc->b = spec.b;
        rc->a = spec.startAlpha;

        auto* sp = particleObj->EmplaceComponent<SpriteComponent>(
            ComponentTypeId::CT_SpriteComponent);
        sp->texture_key = spec.textureKey;
        sp->texture_id = Resource_Manager::getTexture(spec.textureKey);

        Particle particle{};
        particle.id = particleObj->GetId();
        particle.visual = ParticleVisual::Sprite;
        particle.velocity = spec.velocity;
        particle.totalLife = spec.life;
        particle.life = spec.life;
        particle.startSize = spec.startSize;
        particle.endSize = spec.endSize;
        particle.startAlpha = spec.startAlpha;
        particle.endAlpha = spec.endAlpha;

        particles.push_back(particle);
    }

} // namespace Framework