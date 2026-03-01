/*********************************************************************************************
 \file      GameBootstrap.cpp
 \par       SofaSpuds
 \author    OpenAI Codex - Refactor support

 \brief     Moves BloodyGoodCurry-specific startup content out of the engine LogicSystem.
*********************************************************************************************/
#include "EngineCall.hpp"

#include "Debug/Spawn.h"
#include "Component/RenderComponent.h"
#include "Component/TransformComponent.h"
#include "Core/PathUtils.h"
#include "Resource_Asset_Manager/Resource_Manager.h"
#include "Systems/LogicSystem.h"
#include "Systems/RenderSystem.h"

#include <array>
#include <filesystem>
#include <iostream>

namespace
{
    void EnsureAnimatedStore(Framework::LogicSystem& logic)
    {
        if (logic.HasLevelObjectNamed("HawkerStoreAnimated") ||
            logic.HasLevelObjectNamed("Hawker_Store_Animated"))
        {
            return;
        }

        auto* factory = logic.Factory();
        if (!factory)
            return;

        const auto storePath = logic.ResolveDataPath("hawker_store_animated.json");
        if (!std::filesystem::exists(storePath))
        {
            std::cerr << "[Prefab] Missing hawker_store_animated.json at " << storePath << "\n";
            return;
        }

        auto* store = factory->Create(storePath.string());
        if (!store)
        {
            std::cerr << "[Prefab] Failed to spawn Hawker_Store_Animated from " << storePath << "\n";
            return;
        }

        if (auto* tr = store->GetComponentType<Framework::TransformComponent>(
            Framework::ComponentTypeId::CT_TransformComponent))
        {
            tr->x = -0.5f;
            tr->y = -0.9f;
            tr->rot = 0.0f;
        }

        if (auto* render = store->GetComponentType<Framework::RenderComponent>(
            Framework::ComponentTypeId::CT_RenderComponent))
        {
            render->visible = true;
        }

        logic.AddLevelObject(store);
    }

    void PreloadFireEnemyTextures()
    {
        Resource_Manager::load(
            "fire_idle",
            Framework::ResolveProjectAssetPath("Textures/Character/Fire Enemy_Sprite/Idle_Sprite.png").string());
        Resource_Manager::load(
            "fire_attack",
            Framework::ResolveProjectAssetPath("Textures/Character/Fire Enemy_Sprite/Fire Attack_Sprite.png").string());
        Resource_Manager::load(
            "fire_projectile",
            Framework::ResolveProjectAssetPath("Textures/Character/Fire Enemy_Sprite/FireProjectileSprite.png").string());
        Resource_Manager::load(
            "fire_knockback",
            Framework::ResolveProjectAssetPath("Textures/Character/Fire Enemy_Sprite/Knockback_Sprite.png").string());
        Resource_Manager::load(
            "fire_death",
            Framework::ResolveProjectAssetPath("Textures/Character/Fire Enemy_Sprite/Death_Sprite.png").string());
    }

    unsigned LoadTextureHandle(const std::string& key, const std::string& relativePath)
    {
        const auto fullPath = Framework::ResolveProjectAssetPath(relativePath).string();
        Resource_Manager::load(key, fullPath);
        return Resource_Manager::getTexture(key);
    }

    void ConfigureMyGameRenderDefaults(Framework::RenderSystem& render)
    {
        render.SetLegacyPlayerTexture(
            LoadTextureHandle("player_png", "Textures/player.png"));

        const std::array<unsigned, 3> attackTextures{
            LoadTextureHandle("ming_attack1", "Textures/Character/Ming_Sprite/1st_Attack Sprite.png"),
            LoadTextureHandle("ming_attack2", "Textures/Character/Ming_Sprite/2nd_Attack Sprite.png"),
            LoadTextureHandle("ming_attack3", "Textures/Character/Ming_Sprite/3rd_Attack Sprite.png")
        };

        render.SetLegacyAnimationTextures(
            LoadTextureHandle("ming_idle", "Textures/Idle Sprite .png"),
            LoadTextureHandle("ming_run", "Textures/Running Sprite .png"),
            attackTextures,
            LoadTextureHandle("ming_knockback", "Textures/Character/Ming_Sprite/Knockback_Sprite.png"),
            LoadTextureHandle("ming_knife", "Textures/Character/Ming_Sprite/Knife_Sprite.png"),
            LoadTextureHandle("fire_projectile", "Textures/Character/Fire Enemy_Sprite/FireProjectileSprite.png"));

        Resource_Manager::load(
            "impact_vfx_sheet",
            Framework::ResolveProjectAssetPath("Textures/Character/Ming_Sprite/ImpactVFX_Sprite.png").string());
    }
}

namespace mygame
{
    void ConfigureGameBootstrap(Framework::LogicSystem& logic)
    {
        logic.SetStartupLevelPath(logic.ResolveDataPath("level_RealTutorial.json"));
#if SOFASPUDS_ENABLE_EDITOR
        SetSpawnPanelLevelDefaults("level_RealTutorial.json", "RealLevel1.json");
#endif
        logic.SetPostLevelLoadCallback([](Framework::LogicSystem& runtime)
        {
            EnsureAnimatedStore(runtime);
            PreloadFireEnemyTextures();
        });
    }

    void ConfigureRenderBootstrap(Framework::RenderSystem& render)
    {
        render.SetInitializeCallback([](Framework::RenderSystem& runtime)
        {
            ConfigureMyGameRenderDefaults(runtime);
        });
    }
}
