/*********************************************************************************************
 \file      GameBootstrap.cpp
 \par       SofaSpuds
 \author
 \brief     Installs BloodyGoodCurry-specific startup and render bootstrap defaults.
 \details   Provides the game-side bootstrap hooks that configure startup levels, editor
            defaults, save-time behaviour objects, fallback content spawns, and legacy
            render resource setup for the current project.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "EngineCall.hpp"

#include "Debug/Spawn.h"
#include "Game.hpp"
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
    /*************************************************************************************
     \brief  Ensures a named behaviour-only helper object exists in saved level data.
     \param  gameObjects  The serialized level object array being finalized.
     \param  name         The behaviour object name and behaviourKey to guarantee.
    *************************************************************************************/
    void EnsureBehaviourObject(Framework::json& gameObjects, const char* name)
    {
        if (!name || !gameObjects.is_array())
            return;

        for (const auto& go : gameObjects)
        {
            if (!go.is_object())
                continue;

            const auto nameIt = go.find("name");
            if (nameIt == go.end() || !nameIt->is_string() || nameIt->get<std::string>() != name)
                continue;

            const auto compsIt = go.find("Components");
            if (compsIt == go.end() || !compsIt->is_object())
                continue;

            const auto behaviourIt = compsIt->find("BehaviourComponent");
            if (behaviourIt == compsIt->end() || !behaviourIt->is_object())
                continue;

            const auto keyIt = behaviourIt->find("behaviourKey");
            if (keyIt != behaviourIt->end() && keyIt->is_string() && keyIt->get<std::string>() == name)
                return;
        }

        gameObjects.push_back(Framework::json{
            {"Components", Framework::json{
                {"BehaviourComponent", Framework::json{
                    {"behaviourKey", name}
                }}
            }},
            {"layer", "Gameplay:0"},
            {"name", name}
        });
    }

    /*************************************************************************************
     \brief  Installs MyGame's save-finalization policy into the factory.
     \param  logic  The active LogicSystem used to access the factory.
     \details Ensures helper behaviour objects required by this game are present before
              a level is written to disk.
    *************************************************************************************/
    void InstallFactorySavePolicy(Framework::LogicSystem& logic)
    {
        auto* factory = logic.Factory();
        if (!factory)
            return;

        factory->SetLevelSaveFinalizeCallback([](Framework::json& gameObjects)
        {
            EnsureBehaviourObject(gameObjects, "CombatDirector");
            EnsureBehaviourObject(gameObjects, "VfxCleanup");
        });
    }

    /*************************************************************************************
     \brief  Spawns the animated hawker store prefab if the current level lacks it.
     \param  logic  The active LogicSystem used for level queries and object insertion.
    *************************************************************************************/
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

    /*************************************************************************************
     \brief  Preloads fire-enemy textures used by the current game.
    *************************************************************************************/
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

    /*************************************************************************************
     \brief  Loads a texture by key and returns its renderer handle.
     \param  key           Resource-manager key used to cache the texture.
     \param  relativePath  Project-relative asset path for the texture file.
     \return The loaded texture handle.
    *************************************************************************************/
    unsigned LoadTextureHandle(const std::string& key, const std::string& relativePath)
    {
        const auto fullPath = Framework::ResolveProjectAssetPath(relativePath).string();
        Resource_Manager::load(key, fullPath);
        return Resource_Manager::getTexture(key);
    }

    /*************************************************************************************
     \brief  Applies the current game's legacy fallback render textures.
     \param  render  The RenderSystem receiving fallback texture defaults.
    *************************************************************************************/
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
    /*************************************************************************************
     \brief  Configures game-specific startup behavior for the LogicSystem.
     \param  logic  The LogicSystem to receive startup and post-load hooks.
    *************************************************************************************/
    void ConfigureGameBootstrap(Framework::LogicSystem& logic)
    {
        logic.SetStartupLevelPath(logic.ResolveDataPath("level_RealTutorial.json"));
#if SOFASPUDS_ENABLE_EDITOR
        SetSpawnPanelLevelDefaults("level_RealTutorial.json", "RealLevel1.json");
        SetSpawnPanelEditorCallbacks(IsEditorSimulationRunning, LoadLevelFromEditor);
#endif
        logic.SetPostLevelLoadCallback([](Framework::LogicSystem& runtime)
        {
            InstallFactorySavePolicy(runtime);
            EnsureAnimatedStore(runtime);
            PreloadFireEnemyTextures();
        });
    }

    /*************************************************************************************
     \brief  Configures game-specific render bootstrap behavior.
     \param  render  The RenderSystem to receive initialization callbacks.
    *************************************************************************************/
    void ConfigureRenderBootstrap(Framework::RenderSystem& render)
    {
        render.SetInitializeCallback([](Framework::RenderSystem& runtime)
        {
            ConfigureMyGameRenderDefaults(runtime);
        });
    }
}
