/*********************************************************************************************
 \file      SpawnExtensions.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Implements BloodyGoodCurry's game-side spawn-panel extension callbacks.
 \details   Keeps player, enemy, and gate-specific spawn controls out of the engine-owned
            panel while still allowing the shared editor workflow to spawn and batch-edit
            current-game prefabs.

 \copyright
            All content (c) 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#if SOFASPUDS_ENABLE_EDITOR

#include "SpawnExtensions.h"

#include "Common/GameComponentIDs.h"
#include "Components/EnemyAttackComponent.h"
#include "Components/EnemyHealthComponent.h"
#include "Components/GateTargetComponent.h"
#include "Components/PlayerAttackComponent.h"
#include "Components/PlayerHealthComponent.h"
#include "Debug/SpawnPanel.h"

#include "imgui.h"

#include <algorithm>
#include <string>

namespace mygame
{
    namespace
    {
        struct MyGameSpawnSettings
        {
            bool overrideEnemyAttack = false;
            int enemyAttackDamage = 10;
            float enemyAttackSpeed = 1.0f;

            bool overrideEnemyHealth = false;
            int enemyHealth = 0;
            int enemyMaxHealth = 0;

            bool overridePlayerAttack = false;
            int playerAttackDamage = 0;
            float playerAttackSpeed = 1.0f;

            bool overridePlayerHealth = false;
            int playerHealth = 0;
            int playerMaxHealth = 0;

            bool applyGateTargetOnSpawn = true;
            std::string gateTargetLevel = "level.json";
        };

        MyGameSpawnSettings gSpawnSettings;

        /*************************************************************************************
         \brief  Select a stable default gate target from the available level list.
         \param  context  Current spawn panel context.
         \return Preferred gate target filename.
        *************************************************************************************/
        std::string ChooseDefaultGateTarget(const Framework::SpawnPanelContext& context)
        {
            const auto preferred = std::find(context.levelFiles.begin(), context.levelFiles.end(), "RealLevel1.json");
            if (preferred != context.levelFiles.end())
                return *preferred;

            if (!context.levelFiles.empty())
                return context.levelFiles.front();

            return "level.json";
        }

        /*************************************************************************************
         \brief  Clamp the current gate target selection to a valid level filename.
         \param  context  Current spawn panel context.
        *************************************************************************************/
        void SyncGateTargetSelection(const Framework::SpawnPanelContext& context)
        {
            if (context.levelFiles.empty())
            {
                if (gSpawnSettings.gateTargetLevel.empty())
                    gSpawnSettings.gateTargetLevel = "level.json";
                return;
            }

            const auto it = std::find(context.levelFiles.begin(), context.levelFiles.end(),
                gSpawnSettings.gateTargetLevel);
            if (it == context.levelFiles.end())
                gSpawnSettings.gateTargetLevel = ChooseDefaultGateTarget(context);
        }

        /*************************************************************************************
         \brief  Update extension UI defaults when the selected prefab changes.
         \param  prefab   Selected prefab master.
         \param  context  Current spawn panel context.
        *************************************************************************************/
        void OnPrefabSelected(const Framework::GOC& prefab, const Framework::SpawnPanelContext& context)
        {
            if (auto* attack = prefab.GetComponentType<Framework::EnemyAttackComponent>(CT_EnemyAttackComponent()))
            {
                gSpawnSettings.enemyAttackDamage = attack->damage;
                gSpawnSettings.enemyAttackSpeed = attack->attack_speed;
            }

            if (auto* health = prefab.GetComponentType<Framework::EnemyHealthComponent>(CT_EnemyHealthComponent()))
            {
                gSpawnSettings.enemyMaxHealth = health->enemyMaxhealth;
                gSpawnSettings.enemyHealth = health->enemyMaxhealth;
            }

            if (auto* attack = prefab.GetComponentType<Framework::PlayerAttackComponent>(CT_PlayerAttackComponent()))
            {
                gSpawnSettings.playerAttackDamage = attack->damage;
                gSpawnSettings.playerAttackSpeed = attack->attack_speed;
            }

            if (auto* health = prefab.GetComponentType<Framework::PlayerHealthComponent>(CT_PlayerHealthComponent()))
            {
                gSpawnSettings.playerMaxHealth = health->playerMaxhealth;
                gSpawnSettings.playerHealth = health->playerMaxhealth;
            }

            if (auto* gate = prefab.GetComponentType<Framework::GateTargetComponent>(CT_GateTargetComponent()))
            {
                if (!gate->levelPath.empty())
                    gSpawnSettings.gateTargetLevel = gate->levelPath;
            }

            SyncGateTargetSelection(context);
        }

        /*************************************************************************************
         \brief  Draw BloodyGoodCurry-specific spawn controls for the selected prefab.
         \param  prefab   Selected prefab master.
         \param  context  Current spawn panel context.
        *************************************************************************************/
        void DrawMyGameSpawnUi(const Framework::GOC& prefab, const Framework::SpawnPanelContext& context)
        {
            const bool hasEnemyAttack =
                (prefab.GetComponentType<Framework::EnemyAttackComponent>(CT_EnemyAttackComponent()) != nullptr);
            const bool hasEnemyHealth =
                (prefab.GetComponentType<Framework::EnemyHealthComponent>(CT_EnemyHealthComponent()) != nullptr);
            const bool hasPlayerAttack =
                (prefab.GetComponentType<Framework::PlayerAttackComponent>(CT_PlayerAttackComponent()) != nullptr);
            const bool hasPlayerHealth =
                (prefab.GetComponentType<Framework::PlayerHealthComponent>(CT_PlayerHealthComponent()) != nullptr);
            const bool hasGateTarget =
                (prefab.GetComponentType<Framework::GateTargetComponent>(CT_GateTargetComponent()) != nullptr);

            if (hasEnemyAttack)
            {
                ImGui::SeparatorText("Enemy Attack");
                ImGui::Checkbox("Override enemy attack", &gSpawnSettings.overrideEnemyAttack);
                if (!gSpawnSettings.overrideEnemyAttack)
                    ImGui::BeginDisabled();
                ImGui::DragInt("Enemy Damage", &gSpawnSettings.enemyAttackDamage, 1, 0, 100000);
                ImGui::DragFloat("Enemy Attack Speed (s)", &gSpawnSettings.enemyAttackSpeed, 0.01f, 0.01f, 10.0f);
                if (!gSpawnSettings.overrideEnemyAttack)
                    ImGui::EndDisabled();
            }

            if (hasEnemyHealth)
            {
                ImGui::SeparatorText("Enemy Health");
                ImGui::Checkbox("Override enemy health", &gSpawnSettings.overrideEnemyHealth);
                if (!gSpawnSettings.overrideEnemyHealth)
                    ImGui::BeginDisabled();
                ImGui::DragInt("Enemy Health", &gSpawnSettings.enemyHealth, 1, 0, 100000);
                ImGui::DragInt("Enemy Max Health", &gSpawnSettings.enemyMaxHealth, 1, 0, 100000);
                if (!gSpawnSettings.overrideEnemyHealth)
                    ImGui::EndDisabled();
            }

            if (hasPlayerAttack)
            {
                ImGui::SeparatorText("Player Attack");
                ImGui::Checkbox("Override player attack", &gSpawnSettings.overridePlayerAttack);
                if (!gSpawnSettings.overridePlayerAttack)
                    ImGui::BeginDisabled();
                ImGui::DragInt("Player Damage", &gSpawnSettings.playerAttackDamage, 1, 0, 100000);
                ImGui::DragFloat("Player Attack Speed (s)", &gSpawnSettings.playerAttackSpeed, 0.01f, 0.01f, 10.0f);
                if (!gSpawnSettings.overridePlayerAttack)
                    ImGui::EndDisabled();
            }

            if (hasPlayerHealth)
            {
                ImGui::SeparatorText("Player Health");
                ImGui::Checkbox("Override player health", &gSpawnSettings.overridePlayerHealth);
                if (!gSpawnSettings.overridePlayerHealth)
                    ImGui::BeginDisabled();
                ImGui::DragInt("Player Health", &gSpawnSettings.playerHealth, 1, 0, 100000);
                ImGui::DragInt("Player Max Health", &gSpawnSettings.playerMaxHealth, 1, 0, 100000);
                if (!gSpawnSettings.overridePlayerHealth)
                    ImGui::EndDisabled();
            }

            if (hasGateTarget)
            {
                SyncGateTargetSelection(context);
                ImGui::SeparatorText("Gate Target");
                if (context.levelFiles.empty())
                {
                    ImGui::TextDisabled("No level files available for gate targets.");
                }
                else if (ImGui::BeginCombo("Gate Target Level", gSpawnSettings.gateTargetLevel.c_str()))
                {
                    for (const auto& level : context.levelFiles)
                    {
                        const bool selected = (level == gSpawnSettings.gateTargetLevel);
                        if (ImGui::Selectable(level.c_str(), selected))
                            gSpawnSettings.gateTargetLevel = level;
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::Checkbox("Apply gate target on spawn", &gSpawnSettings.applyGateTargetOnSpawn);
            }
        }

        /*************************************************************************************
         \brief  Apply BloodyGoodCurry-specific spawn settings to a live object.
         \param  object                  Target live object.
         \param  context                 Current spawn panel context.
         \param  applyTransformAndLayer  True when handling a new spawn rather than editing.
        *************************************************************************************/
        void ApplyToObject(Framework::GOC& object,
            const Framework::SpawnPanelContext& context,
            bool applyTransformAndLayer)
        {
            (void)context;

            if (auto* attack = object.GetComponentType<Framework::EnemyAttackComponent>(CT_EnemyAttackComponent()))
            {
                if (gSpawnSettings.overrideEnemyAttack)
                {
                    attack->damage = gSpawnSettings.enemyAttackDamage;
                    attack->attack_speed = gSpawnSettings.enemyAttackSpeed;
                }
            }

            if (auto* health = object.GetComponentType<Framework::EnemyHealthComponent>(CT_EnemyHealthComponent()))
            {
                if (gSpawnSettings.overrideEnemyHealth)
                {
                    health->enemyHealth = gSpawnSettings.enemyHealth;
                    health->enemyMaxhealth = gSpawnSettings.enemyMaxHealth;
                }
                else
                {
                    health->enemyHealth = health->enemyMaxhealth;
                }
            }

            if (auto* attack = object.GetComponentType<Framework::PlayerAttackComponent>(CT_PlayerAttackComponent()))
            {
                if (gSpawnSettings.overridePlayerAttack)
                {
                    attack->damage = gSpawnSettings.playerAttackDamage;
                    attack->attack_speed = gSpawnSettings.playerAttackSpeed;
                }
            }

            if (auto* health = object.GetComponentType<Framework::PlayerHealthComponent>(CT_PlayerHealthComponent()))
            {
                if (gSpawnSettings.overridePlayerHealth)
                {
                    health->playerHealth = gSpawnSettings.playerHealth;
                    health->playerMaxhealth = gSpawnSettings.playerMaxHealth;
                }
                else
                {
                    health->playerHealth = health->playerMaxhealth;
                }
            }

            if (applyTransformAndLayer && gSpawnSettings.applyGateTargetOnSpawn)
            {
                if (auto* gate = object.GetComponentType<Framework::GateTargetComponent>(CT_GateTargetComponent()))
                {
                    if (!gSpawnSettings.gateTargetLevel.empty())
                        gate->levelPath = gSpawnSettings.gateTargetLevel;
                }
            }
        }

        /*************************************************************************************
         \brief  Apply BloodyGoodCurry-specific spawn settings to serialized level JSON.
         \param  components     JSON `Components` object for the level entry.
         \param  context        Current spawn panel context.
         \param  objectChanged  Set to true when any field is updated.
        *************************************************************************************/
        void ApplyToLevelJson(Framework::json& components,
            const Framework::SpawnPanelContext& context,
            bool& objectChanged)
        {
            SyncGateTargetSelection(context);

            auto setJsonFloat = [&objectChanged](Framework::json& obj, const char* key, float value)
            {
                auto it = obj.find(key);
                if (it == obj.end() || !it->is_number() || static_cast<float>(it->get<double>()) != value)
                {
                    obj[key] = value;
                    objectChanged = true;
                }
            };

            auto setJsonInt = [&objectChanged](Framework::json& obj, const char* key, int value)
            {
                auto it = obj.find(key);
                if (it == obj.end() || !it->is_number_integer() || it->get<int>() != value)
                {
                    obj[key] = value;
                    objectChanged = true;
                }
            };

            auto setJsonString = [&objectChanged](Framework::json& obj, const char* key, const std::string& value)
            {
                auto it = obj.find(key);
                if (it == obj.end() || !it->is_string() || it->get<std::string>() != value)
                {
                    obj[key] = value;
                    objectChanged = true;
                }
            };

            if (auto it = components.find("EnemyAttackComponent"); it != components.end() && it->is_object())
            {
                auto& attack = *it;
                if (gSpawnSettings.overrideEnemyAttack)
                {
                    setJsonInt(attack, "damage", gSpawnSettings.enemyAttackDamage);
                    setJsonFloat(attack, "attack_speed", gSpawnSettings.enemyAttackSpeed);
                }
            }

            if (auto it = components.find("EnemyHealthComponent"); it != components.end() && it->is_object())
            {
                auto& health = *it;
                if (gSpawnSettings.overrideEnemyHealth)
                {
                    setJsonInt(health, "enemyHealth", gSpawnSettings.enemyHealth);
                    setJsonInt(health, "enemyMaxhealth", gSpawnSettings.enemyMaxHealth);
                }
                else if (auto maxIt = health.find("enemyMaxhealth");
                    maxIt != health.end() && maxIt->is_number_integer())
                {
                    setJsonInt(health, "enemyHealth", maxIt->get<int>());
                }
            }

            if (auto it = components.find("PlayerAttackComponent"); it != components.end() && it->is_object())
            {
                auto& attack = *it;
                if (gSpawnSettings.overridePlayerAttack)
                {
                    setJsonInt(attack, "damage", gSpawnSettings.playerAttackDamage);
                    setJsonFloat(attack, "attack_speed", gSpawnSettings.playerAttackSpeed);
                }
            }

            if (auto it = components.find("PlayerHealthComponent"); it != components.end() && it->is_object())
            {
                auto& health = *it;
                if (gSpawnSettings.overridePlayerHealth)
                {
                    setJsonInt(health, "playerHealth", gSpawnSettings.playerHealth);
                    setJsonInt(health, "playerMaxhealth", gSpawnSettings.playerMaxHealth);
                }
                else if (auto maxIt = health.find("playerMaxhealth");
                    maxIt != health.end() && maxIt->is_number_integer())
                {
                    setJsonInt(health, "playerHealth", maxIt->get<int>());
                }
            }

            if (auto it = components.find("GateTargetComponent"); it != components.end() && it->is_object())
            {
                auto& gate = *it;
                if (gSpawnSettings.applyGateTargetOnSpawn && !gSpawnSettings.gateTargetLevel.empty())
                    setJsonString(gate, "level_path", gSpawnSettings.gateTargetLevel);
            }
        }
    }

    /*************************************************************************************
     \brief  Register BloodyGoodCurry's spawn-panel extension callbacks with the engine.
    *************************************************************************************/
    void RegisterMyGameSpawnPanelExtensions()
    {
        Framework::RegisterSpawnPanelExtension({
            "BloodyGoodCurrySpawnExtension",
            &OnPrefabSelected,
            &DrawMyGameSpawnUi,
            &ApplyToObject,
            &ApplyToLevelJson
            });
    }
}

#endif
