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

#include "Common/GameComponentIDs.h"
#include "Components/EnemyAttackComponent.h"
#include "Components/EnemyComponent.h"
#include "Components/EnemyDecisionTreeComponent.h"
#include "Components/EnemyHealthComponent.h"
#include "Components/EnemyTypeComponent.h"
#include "Components/GateTargetComponent.h"
#include "Components/GlowComponent.h"
#include "Components/PlayerAttackComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/PlayerHUD.h"
#include "Components/PlayerHealthComponent.h"
#include "Components/WayPointComponent.h"
#include "Components/ZoomTriggerComponent.h"
#include "Composition/ComponentCreator.h"
#include "Debug/SpawnPanel.h"
#include "Editor/InspectorPanel.h"
#include "Editor/SpawnExtensions.h"
#include "Game.hpp"
#include "Component/RenderComponent.h"
#include "Component/TransformComponent.h"
#include "Core/PathUtils.h"
#include "Resource_Asset_Manager/Resource_Manager.h"
#include "Systems/LogicSystem.h"
#include "Systems/RenderSystem.h"

#include <array>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <vector>
#include <glm/vec3.hpp>

namespace
{
    std::string ChooseProjectLevelFile(std::initializer_list<const char*> preferredFiles)
    {
        std::error_code ec;
        for (const char* file : preferredFiles)
        {
            if (!file || *file == '\0')
                continue;

            const auto candidate = Framework::ResolveDataPath(file);
            if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec))
                return file;

            ec.clear();
        }

        return "level.json";
    }

    void ReadJsonFloat(const Framework::json& data, const char* key, float& out)
    {
        auto it = data.find(key);
        if (it != data.end() && it->is_number())
            out = static_cast<float>(it->get<double>());
    }

    void ReadJsonInt(const Framework::json& data, const char* key, int& out)
    {
        auto it = data.find(key);
        if (it != data.end() && it->is_number_integer())
            out = it->get<int>();
    }

    void ReadJsonBool(const Framework::json& data, const char* key, bool& out)
    {
        auto it = data.find(key);
        if (it != data.end() && it->is_boolean())
            out = it->get<bool>();
    }

    void ReadJsonString(const Framework::json& data, const char* key, std::string& out)
    {
        auto it = data.find(key);
        if (it != data.end() && it->is_string())
            out = it->get<std::string>();
    }

    /*************************************************************************************
     \brief  Finds the first alive player object using the current game's PlayerComponent ID.
     \param  logic  Active LogicSystem owning the factory to search.
     \return The matching player object, or nullptr if no live player exists.
    *************************************************************************************/
    Framework::GOC* FindAlivePlayer(Framework::LogicSystem& logic)
    {
        auto* factory = logic.Factory();
        if (!factory)
            return nullptr;

        for (auto& [id, ptr] : factory->Objects())
        {
            (void)id;
            if (!ptr)
                continue;

            auto* object = ptr.get();
            if (object->GetComponentType<Framework::PlayerComponent>(mygame::CT_PlayerComponent()))
                return object;
        }

        return nullptr;
    }

    /*************************************************************************************
     \brief  Copies prefab audio settings onto a live object when its audio data is missing.
     \param  obj        Live object that should receive audio state.
     \param  prefabKey  Prefab key used to look up the source audio data.
    *************************************************************************************/
    void RestoreAudioFromPrefab(Framework::GOC* obj, const char* prefabKey)
    {
        if (!obj || !prefabKey)
            return;

        auto prefabIt = Framework::master_copies.find(prefabKey);
        if (prefabIt == Framework::master_copies.end() || !prefabIt->second)
            return;

        auto* prefabAudio = prefabIt->second->GetComponentType<Framework::AudioComponent>(
            Framework::ComponentTypeId::CT_AudioComponent);
        if (!prefabAudio)
            return;

        auto* audio = obj->GetComponentType<Framework::AudioComponent>(
            Framework::ComponentTypeId::CT_AudioComponent);
        if (!audio)
        {
            auto clone = prefabAudio->Clone();
            if (!clone)
                return;

            obj->AddComponent(Framework::ComponentTypeId::CT_AudioComponent, std::move(clone));
            audio = obj->GetComponentType<Framework::AudioComponent>(
                Framework::ComponentTypeId::CT_AudioComponent);
        }

        if (!audio || !audio->GetSounds().empty())
            return;

        audio->volume = prefabAudio->volume;
        for (const auto& [action, info] : prefabAudio->GetSounds())
            audio->AddSound(action, info.id, info.loop);
    }

    /*************************************************************************************
     \brief  Restores game-specific audio defaults after a level is loaded by the engine.
     \param  logic    Active LogicSystem requesting the restore pass.
     \param  objects  Newly loaded live level objects.
     \details Uses current-game component IDs to identify player and enemy variants.
    *************************************************************************************/
    void RestoreMissingLevelAudio(Framework::LogicSystem& logic, const std::vector<Framework::GOC*>& objects)
    {
        (void)logic;
        for (auto* obj : objects)
        {
            if (!obj)
                continue;

            auto* audio = obj->GetComponentType<Framework::AudioComponent>(
                Framework::ComponentTypeId::CT_AudioComponent);
            if (audio && !audio->GetSounds().empty())
                continue;

            if (obj->GetComponentType<Framework::PlayerComponent>(mygame::CT_PlayerComponent()))
            {
                RestoreAudioFromPrefab(obj, "player");
                continue;
            }

            auto* enemyType = obj->GetComponentType<Framework::EnemyTypeComponent>(
                mygame::CT_EnemyTypeComponent());
            if (enemyType && enemyType->Etype == Framework::EnemyTypeComponent::EnemyType::ranged)
            {
                RestoreAudioFromPrefab(obj, "enemyranged");
                continue;
            }

            if (obj->GetComponentType<Framework::EnemyComponent>(mygame::CT_EnemyComponent()))
                RestoreAudioFromPrefab(obj, "enemy");
        }
    }

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
     \brief  Registers BloodyGoodCurry-specific JSON save/load hooks with the factory.
     \param  factory  The active GameObjectFactory receiving the component handlers.
     \details Keeps current-game component snapshot and level serialization rules on the
              game side so Engine/Factory only owns engine-native component schemas.
    *************************************************************************************/
    void InstallFactoryJsonHandlers(Framework::GameObjectFactory& factory)
    {
        factory.SetComponentJsonHandlers(
            mygame::CT_GlowComponent(),
            [](const Framework::GameComponent& component) -> std::optional<Framework::json>
            {
                auto const& glow = static_cast<const Framework::GlowComponent&>(component);
                Framework::json points = Framework::json::array();
                for (const auto& point : glow.points)
                    points.push_back({ {"x", point.x}, {"y", point.y} });

                return Framework::json{
                    {"r", glow.r},
                    {"g", glow.g},
                    {"b", glow.b},
                    {"opacity", glow.opacity},
                    {"brightness", glow.brightness},
                    {"inner_radius", glow.innerRadius},
                    {"outer_radius", glow.outerRadius},
                    {"falloff_exponent", glow.falloffExponent},
                    {"visible", glow.visible},
                    {"points", points}
                };
            },
            [](Framework::GameComponent& component, const Framework::json& data) -> bool
            {
                auto& glow = static_cast<Framework::GlowComponent&>(component);
                ReadJsonFloat(data, "r", glow.r);
                ReadJsonFloat(data, "g", glow.g);
                ReadJsonFloat(data, "b", glow.b);
                ReadJsonFloat(data, "opacity", glow.opacity);
                ReadJsonFloat(data, "brightness", glow.brightness);
                ReadJsonFloat(data, "inner_radius", glow.innerRadius);
                ReadJsonFloat(data, "outer_radius", glow.outerRadius);
                ReadJsonFloat(data, "falloff_exponent", glow.falloffExponent);
                ReadJsonBool(data, "visible", glow.visible);

                glow.points.clear();
                if (auto it = data.find("points"); it != data.end() && it->is_array())
                {
                    glow.points.reserve(it->size());
                    for (const auto& point : *it)
                    {
                        float x = 0.0f;
                        float y = 0.0f;
                        if (point.contains("x") && point["x"].is_number())
                            x = point["x"].get<float>();
                        if (point.contains("y") && point["y"].is_number())
                            y = point["y"].get<float>();
                        glow.points.emplace_back(x, y);
                    }
                }

                return true;
            });

        factory.SetComponentJsonHandlers(
            mygame::CT_PlayerHealthComponent(),
            [](const Framework::GameComponent& component) -> std::optional<Framework::json>
            {
                auto const& health = static_cast<const Framework::PlayerHealthComponent&>(component);
                return Framework::json{
                    {"playerHealth", health.playerHealth},
                    {"playerMaxhealth", health.playerMaxhealth}
                };
            },
            [](Framework::GameComponent& component, const Framework::json& data) -> bool
            {
                auto& health = static_cast<Framework::PlayerHealthComponent&>(component);
                ReadJsonInt(data, "playerHealth", health.playerHealth);
                ReadJsonInt(data, "playerMaxhealth", health.playerMaxhealth);
                return true;
            });

        factory.SetComponentJsonHandlers(
            mygame::CT_PlayerAttackComponent(),
            [](const Framework::GameComponent& component) -> std::optional<Framework::json>
            {
                auto const& attack = static_cast<const Framework::PlayerAttackComponent&>(component);
                return Framework::json{
                    {"damage", attack.damage},
                    {"attack_speed", attack.attack_speed}
                };
            },
            [](Framework::GameComponent& component, const Framework::json& data) -> bool
            {
                auto& attack = static_cast<Framework::PlayerAttackComponent&>(component);
                ReadJsonInt(data, "damage", attack.damage);
                ReadJsonFloat(data, "attack_speed", attack.attack_speed);
                return true;
            });

        factory.SetComponentJsonHandlers(
            mygame::CT_EnemyAttackComponent(),
            [](const Framework::GameComponent& component) -> std::optional<Framework::json>
            {
                auto const& attack = static_cast<const Framework::EnemyAttackComponent&>(component);
                return Framework::json{
                    {"damage", attack.damage},
                    {"attack_speed", attack.attack_speed}
                };
            },
            [](Framework::GameComponent& component, const Framework::json& data) -> bool
            {
                auto& attack = static_cast<Framework::EnemyAttackComponent&>(component);
                ReadJsonInt(data, "damage", attack.damage);
                ReadJsonFloat(data, "attack_speed", attack.attack_speed);
                if (attack.hitbox)
                {
                    ReadJsonFloat(data, "hitwidth", attack.hitbox->width);
                    ReadJsonFloat(data, "hitheight", attack.hitbox->height);
                    ReadJsonFloat(data, "hitduration", attack.hitbox->duration);
                }
                return true;
            });

        factory.SetComponentJsonHandlers(
            mygame::CT_EnemyHealthComponent(),
            [](const Framework::GameComponent& component) -> std::optional<Framework::json>
            {
                auto const& health = static_cast<const Framework::EnemyHealthComponent&>(component);
                return Framework::json{
                    {"enemyHealth", health.enemyHealth},
                    {"enemyMaxhealth", health.enemyMaxhealth}
                };
            },
            [](Framework::GameComponent& component, const Framework::json& data) -> bool
            {
                auto& health = static_cast<Framework::EnemyHealthComponent&>(component);
                ReadJsonInt(data, "enemyHealth", health.enemyHealth);
                ReadJsonInt(data, "enemyMaxhealth", health.enemyMaxhealth);
                return true;
            });

        factory.SetComponentJsonHandlers(
            mygame::CT_EnemyTypeComponent(),
            [](const Framework::GameComponent& component) -> std::optional<Framework::json>
            {
                auto const& type = static_cast<const Framework::EnemyTypeComponent&>(component);
                return Framework::json{
                    {"type", type.Etype == Framework::EnemyTypeComponent::EnemyType::ranged ? "ranged" : "physical"}
                };
            },
            [](Framework::GameComponent& component, const Framework::json& data) -> bool
            {
                auto& type = static_cast<Framework::EnemyTypeComponent&>(component);
                std::string typeName;
                ReadJsonString(data, "type", typeName);
                type.Etype = (typeName == "ranged")
                    ? Framework::EnemyTypeComponent::EnemyType::ranged
                    : Framework::EnemyTypeComponent::EnemyType::physical;
                return true;
            });

        factory.SetComponentJsonHandlers(
            mygame::CT_WayPointComponent(),
            [](const Framework::GameComponent& component) -> std::optional<Framework::json>
            {
                auto const& wayPoint = static_cast<const Framework::WayPointComponent&>(component);
                return Framework::json{
                    {"NeighborIDs", wayPoint.NeighborIDs}
                };
            },
            [](Framework::GameComponent& component, const Framework::json& data) -> bool
            {
                auto& wayPoint = static_cast<Framework::WayPointComponent&>(component);
                wayPoint.NeighborIDs.clear();
                auto it = data.find("NeighborIDs");
                if (it != data.end() && it->is_array())
                    wayPoint.NeighborIDs = it->get<std::vector<int>>();
                return true;
            });

        factory.SetComponentJsonHandlers(
            mygame::CT_ZoomTriggerComponent(),
            [](const Framework::GameComponent& component) -> std::optional<Framework::json>
            {
                auto const& trigger = static_cast<const Framework::ZoomTriggerComponent&>(component);
                return Framework::json{
                    {"targetZoom", trigger.targetZoom},
                    {"oneShot", trigger.oneShot}
                };
            },
            [](Framework::GameComponent& component, const Framework::json& data) -> bool
            {
                auto& trigger = static_cast<Framework::ZoomTriggerComponent&>(component);
                ReadJsonFloat(data, "targetZoom", trigger.targetZoom);
                ReadJsonBool(data, "oneShot", trigger.oneShot);
                return true;
            });

        factory.SetComponentJsonHandlers(
            mygame::CT_GateTargetComponent(),
            [](const Framework::GameComponent& component) -> std::optional<Framework::json>
            {
                auto const& gateTarget = static_cast<const Framework::GateTargetComponent&>(component);
                Framework::json out = Framework::json::object();
                if (!gateTarget.levelPath.empty())
                    out["level_path"] = gateTarget.levelPath;
                return out;
            },
            [](Framework::GameComponent& component, const Framework::json& data) -> bool
            {
                auto& gateTarget = static_cast<Framework::GateTargetComponent&>(component);
                ReadJsonString(data, "level_path", gateTarget.levelPath);
                return true;
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
        // Legacy standalone player texture is no longer used by the game.
        render.SetLegacyPlayerTexture(0);

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
            LoadTextureHandle("fire_projectile", "Textures/Character/Fire Enemy_Sprite/FireProjectileSprite.png")
        );

        Resource_Manager::load(
            "impact_vfx_sheet",
            Framework::ResolveProjectAssetPath("Textures/Character/Ming_Sprite/ImpactVFX_Sprite.png").string());

        Resource_Manager::load(
            "talisman_throw",
            Framework::ResolveProjectAssetPath("Textures/Character/Ming_Sprite/Shooting Talismans_Sprite.png").string());
    }

    /*************************************************************************************
     \brief  Draws BloodyGoodCurry's objective overlay inside the engine render frame.
     \param  render  Active render system providing text renderers and screen size.
    *************************************************************************************/
    void DrawMyGameOverlay(Framework::RenderSystem& render)
    {
        int enemiesLeft = 0;
        if (Framework::FACTORY)
        {
            for (const auto& [id, objPtr] : Framework::FACTORY->Objects())
            {
                (void)id;
                auto* obj = objPtr.get();
                if (!obj)
                    continue;

                if (!obj->GetComponentType<Framework::EnemyComponent>(mygame::CT_EnemyComponent()))
                    continue;

                auto* health = obj->GetComponentType<Framework::EnemyHealthComponent>(
                    mygame::CT_EnemyHealthComponent());
                if (health && health->enemyHealth > 0)
                    ++enemiesLeft;
            }
        }

        std::string text = "Objective: Go to the gate";
        if (enemiesLeft > 0)
        {
            const char* enemyLabel = (enemiesLeft == 1) ? "enemy" : "enemies";
            text = "Objective: Kill all enemies (" + std::to_string(enemiesLeft) + " " + enemyLabel + " remaining)";
        }

        render.GetTextHint().RenderText(
            text,
            static_cast<float>(render.ScreenWidth()) / 3.0f,
            static_cast<float>(render.ScreenHeight()) - 64.0f,
            0.75f,
            glm::vec3(1.0f, 0.2f, 0.2f));
    }
}

namespace mygame
{
    /*************************************************************************************
     \brief  Configures game-specific startup behavior for the LogicSystem.
     \param  logic  The LogicSystem to receive startup and post-load hooks.
     \details
             - Registers game-only ECS components after the engine factory is created.
             - Sets the startup level for BloodyGoodCurry.
             - Installs editor-only startup helpers and post-load content hooks.
    *************************************************************************************/
    void ConfigureGameBootstrap(Framework::LogicSystem& logic)
    {
        GameComponentIDs::Get().Init();
        logic.SetFactorySetupCallback([](Framework::GameObjectFactory& factory)
        {
            auto registerGameComponent = [&factory](const std::string& name,
                Framework::ComponentTypeId typeId,
                std::unique_ptr<Framework::ComponentCreator> creator)
                {
                    (void)typeId;
                    factory.AddComponentCreator(name, std::move(creator));
                };

            registerGameComponent(
                "GlowComponent",
                mygame::CT_GlowComponent(),
                std::make_unique<Framework::ComponentCreatorType<Framework::GlowComponent>>(mygame::CT_GlowComponent()));
            registerGameComponent(
                "PlayerComponent",
                mygame::CT_PlayerComponent(),
                std::make_unique<Framework::ComponentCreatorType<Framework::PlayerComponent>>(mygame::CT_PlayerComponent()));
            registerGameComponent(
                "PlayerAttackComponent",
                mygame::CT_PlayerAttackComponent(),
                std::make_unique<Framework::ComponentCreatorType<Framework::PlayerAttackComponent>>(mygame::CT_PlayerAttackComponent()));
            registerGameComponent(
                "PlayerHealthComponent",
                mygame::CT_PlayerHealthComponent(),
                std::make_unique<Framework::ComponentCreatorType<Framework::PlayerHealthComponent>>(mygame::CT_PlayerHealthComponent()));
            factory.AddComponentCreator(
                "PlayerHUDComponent",
                std::make_unique<Framework::ComponentCreatorType<Framework::PlayerHUDComponent>>(
                    mygame::CT_PlayerHUDComponent()));
            registerGameComponent(
                "EnemyComponent",
                mygame::CT_EnemyComponent(),
                std::make_unique<Framework::ComponentCreatorType<Framework::EnemyComponent>>(mygame::CT_EnemyComponent()));
            registerGameComponent(
                "EnemyAttackComponent",
                mygame::CT_EnemyAttackComponent(),
                std::make_unique<Framework::ComponentCreatorType<Framework::EnemyAttackComponent>>(mygame::CT_EnemyAttackComponent()));
            registerGameComponent(
                "EnemyDecisionTreeComponent",
                mygame::CT_EnemyDecisionTreeComponent(),
                std::make_unique<Framework::ComponentCreatorType<Framework::EnemyDecisionTreeComponent>>(mygame::CT_EnemyDecisionTreeComponent()));
            registerGameComponent(
                "EnemyHealthComponent",
                mygame::CT_EnemyHealthComponent(),
                std::make_unique<Framework::ComponentCreatorType<Framework::EnemyHealthComponent>>(mygame::CT_EnemyHealthComponent()));
            registerGameComponent(
                "EnemyTypeComponent",
                mygame::CT_EnemyTypeComponent(),
                std::make_unique<Framework::ComponentCreatorType<Framework::EnemyTypeComponent>>(mygame::CT_EnemyTypeComponent()));
            registerGameComponent(
                "WayPointComponent",
                mygame::CT_WayPointComponent(),
                std::make_unique<Framework::ComponentCreatorType<Framework::WayPointComponent>>(mygame::CT_WayPointComponent()));
            registerGameComponent(
                "ZoomTriggerComponent",
                mygame::CT_ZoomTriggerComponent(),
                std::make_unique<Framework::ComponentCreatorType<Framework::ZoomTriggerComponent>>(mygame::CT_ZoomTriggerComponent()));
            registerGameComponent(
                "GateTargetComponent",
                mygame::CT_GateTargetComponent(),
                std::make_unique<Framework::ComponentCreatorType<Framework::GateTargetComponent>>(mygame::CT_GateTargetComponent()));

            InstallFactoryJsonHandlers(factory);
        });
        logic.SetFindPlayerCallback(&FindAlivePlayer);
        logic.SetPostAudioRestoreCallback(&RestoreMissingLevelAudio);
        logic.SetStartupLevelPath(
            logic.ResolveDataPath(ChooseProjectLevelFile({ "level_RealTutorial.json", "level.json" })));
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
        render.SetOverlayCallback([](Framework::RenderSystem& runtime)
        {
            DrawMyGameOverlay(runtime);
        });
#if SOFASPUDS_ENABLE_EDITOR
        Framework::ClearSpawnPanelExtensions();
        RegisterMyGameSpawnPanelExtensions();
        render.SetEditorPanelsCallback([]()
        {
            DrawPropertiesEditor();
        });
#endif
    }
}
