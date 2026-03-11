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
#include "Editor/ParticlePresetEditor.h"
#include "Editor/SpawnExtensions.h"
#include "Game.hpp"
#include "Component/BehaviourComponent.h"
#include "Component/RenderComponent.h"
#include "Component/TransformComponent.h"
#include "Core/PathUtils.h"
#include "Graphics/Graphics.hpp"
#include "Resource_Asset_Manager/Resource_Manager.h"
#include "Systems/LogicSystem.h"
#include "Systems/RenderSystem.h"

#include <array>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <vector>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>

namespace
{
    struct ObjectiveTabUiState
    {
        float reveal{ 0.0f };
        double lastFrameTime{ -1.0 };
        bool pauseMouseDownPrev{ false };
    };

    ObjectiveTabUiState gObjectiveTabUiState;

    float AdvanceObjectiveTabReveal(bool hovered)
    {
        const double now = glfwGetTime();
        if (gObjectiveTabUiState.lastFrameTime < 0.0)
            gObjectiveTabUiState.lastFrameTime = now;

        const float dt = std::clamp(
            static_cast<float>(now - gObjectiveTabUiState.lastFrameTime),
            0.0f,
            0.05f);
        gObjectiveTabUiState.lastFrameTime = now;

        const float speed = hovered ? 8.0f : 6.0f;
        const float target = hovered ? 1.0f : 0.0f;
        if (gObjectiveTabUiState.reveal < target)
            gObjectiveTabUiState.reveal = std::min(target, gObjectiveTabUiState.reveal + dt * speed);
        else
            gObjectiveTabUiState.reveal = std::max(target, gObjectiveTabUiState.reveal - dt * speed);

        const float t = std::clamp(gObjectiveTabUiState.reveal, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    std::vector<std::string> WrapObjectiveText(const std::string& text, std::size_t preferredLineLength = 18)
    {
        auto trim = [](std::string value)
            {
                const std::size_t first = value.find_first_not_of(' ');
                if (first == std::string::npos)
                    return std::string{};
                const std::size_t last = value.find_last_not_of(' ');
                return value.substr(first, last - first + 1);
            };

        std::vector<std::string> lines;
        const std::string cleaned = trim(text);
        if (cleaned.empty())
            return lines;

        if (cleaned.size() <= preferredLineLength)
        {
            lines.push_back(cleaned);
            return lines;
        }

        std::size_t split = cleaned.rfind(' ', preferredLineLength);
        if (split == std::string::npos)
            split = cleaned.find(' ', preferredLineLength);

        if (split == std::string::npos)
        {
            lines.push_back(cleaned);
            return lines;
        }

        lines.push_back(trim(cleaned.substr(0, split)));
        const std::string remaining = trim(cleaned.substr(split + 1));
        if (!remaining.empty())
            lines.push_back(remaining);
        return lines;
    }

    unsigned ResolveObjectiveTabTexture()
    {
        constexpr const char* kTextureKey = "objective_tab_ui";
        if (const unsigned cached = Resource_Manager::getTexture(kTextureKey))
            return cached;

        Resource_Manager::load(
            kTextureKey,
            Framework::ResolveProjectAssetPath("Textures/UI/Objective Tab.png").string());
        return Resource_Manager::getTexture(kTextureKey);
    }

    unsigned ResolvePauseButtonTexture()
    {
        constexpr const char* kTextureKey = "pause_button_ui";
        if (const unsigned cached = Resource_Manager::getTexture(kTextureKey))
            return cached;

        Resource_Manager::load(
            kTextureKey,
            Framework::ResolveProjectAssetPath("Textures/UI/Pause Button.png").string());
        return Resource_Manager::getTexture(kTextureKey);
    }

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
            audio->AddSound(action, info.id, info.loop, info.spatial, info.volume);
    }

    void ForceAllSoundsSpatial(Framework::AudioComponent& audio)
    {
        std::vector<std::tuple<std::string, std::string, bool, float>> sounds;
        sounds.reserve(audio.GetSounds().size());

        for (const auto& [action, info] : audio.GetSounds())
            sounds.emplace_back(action, info.id, info.loop, info.volume);

        for (const auto& [action, id, loop, actionVolume] : sounds)
            audio.AddSound(action, id, loop, true, actionVolume);
    }

    bool IsEnemyObject(const Framework::GOC& obj)
    {
        return obj.GetComponentType<Framework::EnemyComponent>(mygame::CT_EnemyComponent()) != nullptr;
    }

    bool IsEnemyObject(const Framework::json& objectData)
    {
        const auto compsIt = objectData.find("Components");
        if (compsIt == objectData.end() || !compsIt->is_object())
            return false;

        return compsIt->contains("EnemyComponent") || compsIt->contains("EnemyTypeComponent");
    }

    void ForceSavedEnemyAudioSpatial(Framework::json& gameObjects)
    {
        if (!gameObjects.is_array())
            return;

        for (auto& go : gameObjects)
        {
            if (!go.is_object() || !IsEnemyObject(go))
                continue;

            auto compsIt = go.find("Components");
            if (compsIt == go.end() || !compsIt->is_object())
                continue;

            auto audioIt = compsIt->find("AudioComponent");
            if (audioIt == compsIt->end() || !audioIt->is_object())
                continue;

            auto soundsIt = audioIt->find("sounds");
            if (soundsIt == audioIt->end() || !soundsIt->is_object())
                continue;

            for (auto& [actionName, soundData] : soundsIt->items())
            {
                (void)actionName;
                if (!soundData.is_object())
                    continue;

                soundData["spatial"] = true;
            }
        }
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
            if (obj->GetComponentType<Framework::PlayerComponent>(mygame::CT_PlayerComponent()))
            {
                if (audio && !audio->GetSounds().empty())
                    continue;

                RestoreAudioFromPrefab(obj, "player");
                continue;
            }

            if (!IsEnemyObject(*obj))
                continue;

            if (!audio || audio->GetSounds().empty())
            {
                auto* enemyType = obj->GetComponentType<Framework::EnemyTypeComponent>(
                    mygame::CT_EnemyTypeComponent());
                if (enemyType && enemyType->Etype == Framework::EnemyTypeComponent::EnemyType::ranged)
                    RestoreAudioFromPrefab(obj, "enemyranged");
                else
                    RestoreAudioFromPrefab(obj, "enemy");

                audio = obj->GetComponentType<Framework::AudioComponent>(
                    Framework::ComponentTypeId::CT_AudioComponent);
            }

            if (audio)
                ForceAllSoundsSpatial(*audio);
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
            ForceSavedEnemyAudioSpatial(gameObjects);
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
        
        Resource_Manager::load(
            "talisman_throw",
            Framework::ResolveProjectAssetPath("Textures/Character/Ming_Sprite/Shooting Talismans_Sprite.png").string());
        Resource_Manager::load(
            "player_walkback",
            Framework::ResolveProjectAssetPath("Textures/Character/Ming_Sprite/Jogging Backwards_Sprite.png").string());

        render.SetLegacyAnimationTextures(
            LoadTextureHandle("ming_idle", "Textures/Idle Sprite .png"),
            LoadTextureHandle("ming_run", "Textures/Running Sprite .png"),
            attackTextures,
            LoadTextureHandle("ming_knockback", "Textures/Character/Ming_Sprite/Knockback_Sprite.png"),
            LoadTextureHandle("ming_knife", "Textures/Character/Ming_Sprite/Knife_Sprite.png"),
            LoadTextureHandle("ming_talisman", "Textures/Character/Ming_Sprite/Flying Talisman_Sprite .png"),
            LoadTextureHandle("fire_projectile", "Textures/Character/Fire Enemy_Sprite/FireProjectileSprite.png")
        );


        Resource_Manager::load(
            "impact_vfx_sheet",
            Framework::ResolveProjectAssetPath("Textures/Character/Ming_Sprite/ImpactVFX_Sprite.png").string());

    }

    /*************************************************************************************
     \brief  Draws BloodyGoodCurry's objective overlay inside the engine render frame.
     \param  render  Active render system providing text renderers and screen size.
    *************************************************************************************/
    void DrawMyGameOverlay(Framework::RenderSystem& render)
    {
        int enemiesLeft = 0;
        bool hasGateDoorObjective = false;
        if (Framework::FACTORY)
        {
            for (const auto& [id, objPtr] : Framework::FACTORY->Objects())
            {
                (void)id;
                auto* obj = objPtr.get();
                if (!obj)
                    continue;

                auto* behaviour = obj->GetComponentType<Framework::BehaviourComponent>(
                    Framework::ComponentTypeId::CT_BehaviourComponent);
                auto* gateTarget = obj->GetComponentType<Framework::GateTargetComponent>(
                    mygame::CT_GateTargetComponent());
                if (!hasGateDoorObjective && behaviour && gateTarget && behaviour->behaviourKey == "GateLogic")
                {
                    std::string objectName = obj->GetObjectName();
                    std::transform(objectName.begin(), objectName.end(), objectName.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                    bool looksLikeDoor = (objectName.find("door") != std::string::npos);
                    if (!looksLikeDoor)
                    {
                        if (auto* renderComp = obj->GetComponentType<Framework::RenderComponent>(
                            Framework::ComponentTypeId::CT_RenderComponent))
                        {
                            std::string textureKey = renderComp->texture_key;
                            std::string texturePath = renderComp->texture_path;
                            std::transform(textureKey.begin(), textureKey.end(), textureKey.begin(),
                                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                            std::transform(texturePath.begin(), texturePath.end(), texturePath.begin(),
                                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                            looksLikeDoor = textureKey.find("door") != std::string::npos ||
                                texturePath.find("door") != std::string::npos;
                        }
                    }

                    if (looksLikeDoor)
                        hasGateDoorObjective = true;
                }

                if (!obj->GetComponentType<Framework::EnemyComponent>(mygame::CT_EnemyComponent()))
                    continue;

                auto* health = obj->GetComponentType<Framework::EnemyHealthComponent>(
                    mygame::CT_EnemyHealthComponent());
                if (health && health->enemyHealth > 0)
                    ++enemiesLeft;
            }
        }

        std::string text = hasGateDoorObjective
            ? "Go to the door"
            : "Go to the gate";
        if (enemiesLeft > 0)
        {
            text = "Kill all enemies (" + std::to_string(enemiesLeft) + " left)";
        }

        const int screenW = render.ScreenWidth();
        const int screenH = render.ScreenHeight();
        const float uiScale = std::max(0.75f, static_cast<float>(screenH) / 1080.0f);
        int viewportX = 0;
        int viewportY = 0;
        int viewportW = screenW;
        int viewportH = screenH;
        render.GetGameViewportRect(viewportX, viewportY, viewportW, viewportH);

        const unsigned objectiveTabTexture = ResolveObjectiveTabTexture();
        if (objectiveTabTexture != 0u)
        {
            int texW = 0;
            int texH = 0;
            if (!gfx::Graphics::getTextureSize(objectiveTabTexture, texW, texH) || texW <= 0 || texH <= 0)
            {
                texW = 480;
                texH = 160;
            }

            const float tabW = texW * 0.72f * uiScale;
            const float tabH = texH * 0.72f * uiScale;
            const float tabY = viewportY + viewportH - tabH - (168.0f * uiScale);
            const float handleWidth = std::min(tabW * 0.2f, 92.0f * uiScale);
            const float rightInset = 28.0f * uiScale;
            const float expandedTabX = viewportX + viewportW - tabW - rightInset;
            const float collapsedTabX = viewportX + viewportW - handleWidth - rightInset;

            double mouseX = -1000.0;
            double mouseY = -1000.0;
            bool leftMouseDown = false;
            if (GLFWwindow* window = glfwGetCurrentContext())
            {
                glfwGetCursorPos(window, &mouseX, &mouseY);
                mouseY = static_cast<double>(screenH) - mouseY;
                leftMouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            }

            const bool mouseOverHandle =
                mouseX >= (viewportX + viewportW - handleWidth) &&
                mouseX <= (viewportX + viewportW) &&
                mouseY >= tabY &&
                mouseY <= (tabY + tabH);

            const float currentTabXPreAnim =
                collapsedTabX + (expandedTabX - collapsedTabX) * gObjectiveTabUiState.reveal;
            const bool mouseOverOpenTab =
                mouseX >= currentTabXPreAnim &&
                mouseX <= (currentTabXPreAnim + tabW) &&
                mouseY >= tabY &&
                mouseY <= (tabY + tabH);

            const float reveal = AdvanceObjectiveTabReveal(mouseOverHandle || mouseOverOpenTab);
            const float tabX = collapsedTabX + (expandedTabX - collapsedTabX) * reveal;

            const unsigned pauseButtonTexture = ResolvePauseButtonTexture();
            if (pauseButtonTexture != 0u)
            {
                int pauseTexW = 0;
                int pauseTexH = 0;
                if (!gfx::Graphics::getTextureSize(pauseButtonTexture, pauseTexW, pauseTexH) ||
                    pauseTexW <= 0 || pauseTexH <= 0)
                {
                    pauseTexW = 120;
                    pauseTexH = 120;
                }

                const float pauseScale = 0.72f * uiScale;
                const float pauseW = pauseTexW * pauseScale;
                const float pauseH = pauseTexH * pauseScale;
                const float pauseX = viewportX + viewportW - pauseW - rightInset;
                const float pauseY = viewportY + viewportH - pauseH - (22.0f * uiScale);
                const bool pauseHovered =
                    mouseX >= pauseX && mouseX <= (pauseX + pauseW) &&
                    mouseY >= pauseY && mouseY <= (pauseY + pauseH);
                const bool pauseClicked = pauseHovered && leftMouseDown && !gObjectiveTabUiState.pauseMouseDownPrev;

                gfx::Graphics::renderSpriteUI(
                    pauseButtonTexture,
                    pauseX,
                    pauseY,
                    pauseW,
                    pauseH,
                    1.0f, 1.0f, 1.0f, pauseHovered ? 1.0f : 0.94f,
                    screenW, screenH);

                if (pauseClicked)
                    mygame::RequestPauseMenu();
            }

            gObjectiveTabUiState.pauseMouseDownPrev = leftMouseDown;

            gfx::Graphics::renderSpriteUI(
                objectiveTabTexture,
                tabX,
                tabY,
                tabW,
                tabH,
                1.0f, 1.0f, 1.0f, 0.96f,
                screenW, screenH);

            if (render.IsTextReadyHint() && reveal > 0.2f)
            {
                const std::vector<std::string> wrappedText = WrapObjectiveText(text);
                const float textX = tabX + tabW * 0.24f;
                const glm::vec3 textColor(1.0f, 0.95f, 0.85f);

                if (wrappedText.size() > 1)
                {
                    render.GetTextHint().RenderText(
                        wrappedText[0],
                        textX,
                        tabY + tabH * 0.54f,
                        0.46f * uiScale,
                        textColor);
                    render.GetTextHint().RenderText(
                        wrappedText[1],
                        textX,
                        tabY + tabH * 0.31f,
                        0.46f * uiScale,
                        textColor);
                }
                else if (!wrappedText.empty())
                {
                    render.GetTextHint().RenderText(
                        wrappedText[0],
                        textX,
                        tabY + tabH * 0.43f,
                        0.56f * uiScale,
                        textColor);
                }
            }
            return;
        }

        if (render.IsTextReadyHint())
        {
            render.GetTextHint().RenderText(
                text,
                static_cast<float>(screenW) / 3.0f,
                static_cast<float>(screenH) - 64.0f,
                0.75f,
                glm::vec3(1.0f, 0.2f, 0.2f));
        }
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
            DrawParticlePresetEditor();
        });
#endif
    }
}
