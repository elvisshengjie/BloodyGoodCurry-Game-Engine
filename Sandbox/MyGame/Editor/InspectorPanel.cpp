/*********************************************************************************************
 \file      InspectorPanel.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu) - Primary Author, 100%
 \brief     Implements a basic ImGui inspector window for viewing and editing the currently
            selected GameObject's properties and common component data.

 \copyright
            All content (c) 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "InspectorPanel.h"

#if SOFASPUDS_ENABLE_EDITOR

#include "Composition/Composition.h"
#include "Component/TransformComponent.h"
#include "Component/RenderComponent.h"
#include "Component/CircleRenderComponent.h"
#include "Component/FlashComponent.h"
#include "Component/BehaviourComponent.h"
#include "Component/ShadowComponent.h"
#include "Components/GlowComponent.h"
#include "Components/GateTargetComponent.h"
#include "Component/SpriteComponent.h"
#include "Component/HitBoxComponent.h"
#include "Component/HitBoxComponent.h"
#include "Components/PlayerAttackComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/EnemyAttackComponent.h"
#include "Components/PlayerHealthComponent.h"
#include "Components/EnemyHealthComponent.h"
#include "Components/EnemyTypeComponent.h"
#include "Physics/Dynamics/RigidBodyComponent.h"
#include "Core/PathUtils.h"
#include "Debug/Selection.h"
#include "Factory/Factory.h"
#include "Debug/UndoStack.h"

#include <imgui.h>
#include <array>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdio>
#include "Common/CRTDebug.h"   // <- bring in DBG_NEW

#ifdef _DEBUG
#define new DBG_NEW       // <- redefine new AFTER all includes
#endif
namespace
{
    using namespace Framework;

    std::vector<std::string> GetAvailableLevelFiles()
    {
        std::vector<std::string> levels;
        std::error_code ec;
        const auto dataRoot = Framework::ResolveDataPath("");
        if (dataRoot.empty() || !std::filesystem::exists(dataRoot, ec))
            return levels;

        for (const auto& entry : std::filesystem::directory_iterator(dataRoot, ec))
        {
            if (ec || !entry.is_regular_file(ec))
                continue;

            const auto& path = entry.path();
            if (path.extension() != ".json")
                continue;

            levels.push_back(path.filename().string());
        }

        std::sort(levels.begin(), levels.end());
        return levels;
    }

    std::vector<const char*> GetAllowedBehaviourKeys(const GOC& object, const BehaviourComponent& behaviour)
    {
        if (object.GetComponentAs<PlayerComponent>(ComponentTypeId::CT_PlayerComponent))
            return { "PlayerController" };

        if (object.GetComponentAs<GateTargetComponent>(ComponentTypeId::CT_GateTargetComponent))
            return { "GateLogic", "KeyDoorLogic" };

        std::string loweredName = object.GetObjectName();
        std::transform(loweredName.begin(), loweredName.end(), loweredName.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (behaviour.behaviourKey == "KeyPickupLogic" || loweredName.find("key") != std::string::npos)
            return { "KeyPickupLogic" };

        return { "GameDirector", "CombatDirector", "VfxCleanup" };
    }

    /*************************************************************************************
      \brief Draws ImGui controls for the AudioComponent.

      Exposes:
      - Master volume slider.
      - Per-action (e.g., footsteps, slash) sound resource selection and loop toggle.
    *************************************************************************************/
    void DrawAudioSection(AudioComponent& audio)
    {
        if (!ImGui::CollapsingHeader("Audio Component", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        // 1. Global master volume control (0..1)
        ImGui::DragFloat("Master Volume", &audio.volume, 0.01f, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::TextDisabled("Sound Actions");

        // 2. Iterate via the public accessor — m_sounds is private.
        //    GetSounds() returns a const ref so we can read keys and SoundInfo,
        //    but we only mutate the loop flag which goes through AudioComponent's
        //    own data (loop is stored inside SoundInfo which is value-type in the map).
        //    To allow toggling loop in the inspector we need a mutable ref, so we use
        //    a small helper: re-register the sound with the updated flag via AddSound().
        std::vector<std::pair<std::string, Framework::SoundInfo>> soundEntries;
        soundEntries.reserve(audio.GetSounds().size());
        for (const auto& [action, info] : audio.GetSounds())
            soundEntries.emplace_back(action, info);

        for (const auto& [action, info] : soundEntries)
        {
            if (ImGui::TreeNode(action.c_str()))
            {
                // Sound ID is data-driven from the prefab JSON — show as read-only.
                std::array<char, 256> idBuffer{};
                std::snprintf(idBuffer.data(), idBuffer.size(), "%s", info.id.c_str());
                std::string idLabel = "Sound ID##" + action;
                if (ImGui::InputText(idLabel.c_str(), idBuffer.data(), idBuffer.size()))
                {
                    audio.AddSound(action, idBuffer.data(), info.loop, info.spatial, info.volume);
                }

                float actionVolume = info.volume;
                std::string volumeLabel = "Action Volume##" + action;
                if (ImGui::DragFloat(volumeLabel.c_str(), &actionVolume, 0.01f, 0.0f, 3.0f, "%.2f"))
                {
                    audio.AddSound(action, info.id, info.loop, info.spatial, actionVolume);
                }

                bool loopFlag = info.loop;
                std::string loopLabel = "Loop##" + action;
                if (ImGui::Checkbox(loopLabel.c_str(), &loopFlag))
                {
                    audio.AddSound(action, info.id, loopFlag, info.spatial, info.volume);
                }

                ImGui::TextDisabled("Spatial: %s", info.spatial ? "true" : "false");

                ImGui::TreePop();
            }
        }
    }

    /*************************************************************************************
      \brief Draws common ImGui controls for a HitBoxComponent.

      \param hb          HitBoxComponent being edited.
      \param labelPrefix Optional label prefix to disambiguate controls when reused.
                         (e.g., "HB " ? "HB Size##HitBox")
    *************************************************************************************/
    void DrawHitBoxFields(HitBoxComponent& hb, const char* labelPrefix = "")
    {
        // Optional prefix so we can reuse this for nested hitboxes if needed.
        // The lambda builds a combined label once per call using a static buffer.
        auto MakeLabel = [labelPrefix](const char* base) -> const char*
            {
                static char buf[64];
                if (labelPrefix && *labelPrefix) {
                    std::snprintf(buf, sizeof(buf), "%s%s", labelPrefix, base);
                    return (const char*)buf;   // ensure const-correctness
                }
                return base;
            };

        // Hitbox size (width, height)
        float size[2] = { hb.width, hb.height };
        if (ImGui::DragFloat2(MakeLabel("Size##HitBox"), size, 0.01f, 0.0f, 1000.0f, "%.3f"))
        {
            hb.width = size[0];
            hb.height = size[1];
        }

        // Spawn offset relative to owner
        float offset[2] = { hb.spawnX, hb.spawnY };
        if (ImGui::DragFloat2(MakeLabel("Spawn Offset"), offset, 0.01f, -1000.0f, 1000.0f, "%.3f"))
        {
            hb.spawnX = offset[0];
            hb.spawnY = offset[1];
        }

        // Lifetime and damage
        ImGui::DragFloat(MakeLabel("Duration"), &hb.duration, 0.01f, 0.0f, 100.0f, "%.2f");
        ImGui::DragFloat(MakeLabel("Damage##HitBox"), &hb.damage, 0.1f, 0.0f, 1000.0f, "%.1f");

        // Team filter
        const char* teamLabels[] = { "Player", "Enemy", "Neutral", "Thrown" };
        int teamIndex = static_cast<int>(hb.team);
        if (ImGui::Combo(MakeLabel("Team"), &teamIndex, teamLabels, IM_ARRAYSIZE(teamLabels)))
        {
            hb.team = static_cast<HitBoxComponent::Team>(teamIndex);
        }

        // Active flag
        ImGui::Checkbox(MakeLabel("Active"), &hb.active);
    }

    /*************************************************************************************
      \brief Draws the collapsible "HitBox" section for a standalone HitBoxComponent.
    *************************************************************************************/
    void DrawHitBoxSection(HitBoxComponent& hb)
    {
        if (!ImGui::CollapsingHeader("HitBox", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        DrawHitBoxFields(hb);
    }

    /*************************************************************************************
      \brief Draws ImGui controls for PlayerAttackComponent.

      Exposes:
      - Damage and attack speed.
      - Embedded default HitBox settings (if a hitbox template exists).
    *************************************************************************************/
    void DrawPlayerAttackSection(PlayerAttackComponent& atk)
    {
        if (!ImGui::CollapsingHeader("PlayerAttack", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::DragInt("Damage##PlayerAttack", &atk.damage, 1, 0, 999);
        ImGui::DragFloat("Attack Speed", &atk.attack_speed, 0.01f, 0.0f, 10.0f, "%.2f");

        // Optional embedded default hitbox editor
        if (atk.hitbox)
        {
            if (ImGui::TreeNode("HitBox Defaults"))
            {
                DrawHitBoxFields(*atk.hitbox, "HB ");
                ImGui::TreePop();
            }
        }
    }

    /*************************************************************************************
      \brief Draws ImGui controls for EnemyAttackComponent.

      Exposes:
      - Damage and attack speed.
      - Embedded default HitBox settings (if a hitbox template exists).
    *************************************************************************************/
    void DrawEnemyAttackSection(EnemyAttackComponent& atk)
    {
        if (!ImGui::CollapsingHeader("EnemyAttack", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::DragInt("Damage##EnemyAttack", &atk.damage, 1, 0, 999);
        ImGui::DragFloat("Attack Speed", &atk.attack_speed, 0.01f, 0.0f, 10.0f, "%.2f");

        // Optional embedded default hitbox editor
        if (atk.hitbox)
        {
            if (ImGui::TreeNode("HitBox Defaults"))
            {
                DrawHitBoxFields(*atk.hitbox, "HB ");
                ImGui::TreePop();
            }
        }
    }

    /*************************************************************************************
      \brief Draws ImGui controls for PlayerHealthComponent.

      Exposes current and max HP. Current HP is clamped by max HP.
    *************************************************************************************/
    void DrawPlayerHealthSection(PlayerHealthComponent& hp)
    {
        if (!ImGui::CollapsingHeader("PlayerHealth", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::DragInt("Current HP", &hp.playerHealth, 1, 0, hp.playerMaxhealth);
        ImGui::DragInt("Max HP", &hp.playerMaxhealth, 1, 1, 999);
    }

    /*************************************************************************************
      \brief Draws ImGui controls for EnemyHealthComponent.

      Exposes current and max HP. Current HP is clamped by max HP.
    *************************************************************************************/
    void DrawEnemyHealthSection(EnemyHealthComponent& hp)
    {
        if (!ImGui::CollapsingHeader("EnemyHealth", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::DragInt("Current HP", &hp.enemyHealth, 1, 0, hp.enemyMaxhealth);
        ImGui::DragInt("Max HP", &hp.enemyMaxhealth, 1, 1, 999);
    }

    /*************************************************************************************
      \brief Draws ImGui controls for RigidBodyComponent.

      Exposes:
      - Velocity vector (velX, velY).
      - Collider size (width, height).

      \param owner GOC owning this RigidBody; currently unused but passed so it can be
                   hooked up to UndoStack in the future.
    *************************************************************************************/
    void DrawRigidBodySection(Framework::GOC& owner, RigidBodyComponent& rb)
    {
        if (!ImGui::CollapsingHeader("RigidBody", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        // Velocity (x, y)
        float velocity[2] = { rb.velX, rb.velY };
        if (ImGui::DragFloat2("Velocity", velocity, 0.1f, -1000.0f, 1000.0f, "%.2f"))
        {
            rb.velX = velocity[0];
            rb.velY = velocity[1];
        }

        // Collider size (width, height)
        float size[2] = { rb.width, rb.height };
        if (ImGui::DragFloat2("Collider Size", size, 0.01f, 0.0f, 1000.0f, "%.3f"))
        {
            rb.width = size[0];
            rb.height = size[1];
        }

        // (owner is passed in so later you can hook this into UndoStack if you want)
        (void)owner;
    }

    /*************************************************************************************
      \brief Draws ImGui controls for TransformComponent with undo support.

      Uses:
      - CaptureTransformSnapshot before the first edit.
      - RecordTransformChange after edits, so the whole drag can be undone in one step.
    *************************************************************************************/
    void DrawTransformSection(Framework::GOC& owner, TransformComponent& transform)
    {
        if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        mygame::editor::TransformSnapshot before{};
        bool captured = false;

        // Helper lambda: capture the "before" state only once.
        auto capture = [&]()
            {
                if (!captured)
                {
                    before = mygame::editor::CaptureTransformSnapshot(owner);
                    captured = true;
                }
            };

        // Position
        float position[2] = { transform.x, transform.y };
        if (ImGui::DragFloat2("Position", position, 0.1f))
        {
            capture();
            transform.x = position[0];
            transform.y = position[1];
        }

        // Rotation
        if (ImGui::DragFloat("Rotation", &transform.rot, 0.5f, -360.0f, 360.0f, "%.2f"))
            capture();

        // If we captured a "before" snapshot, push the change to the UndoStack.
        if (captured)
            mygame::editor::RecordTransformChange(owner, before);
    }

    /*************************************************************************************
      \brief Draws ImGui controls for RenderComponent with undo support.

      Exposes:
      - Size (w, h) as a 2D drag.
      - Color tint (rgba).
      - Visibility toggle.

      Uses a transform snapshot so size edits can participate in transform undo.
    *************************************************************************************/
    void DrawRenderSection(Framework::GOC& owner, RenderComponent& render)
    {
        if (!ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        mygame::editor::TransformSnapshot before{};
        bool captured = false;

        auto capture = [&]()
            {
                if (!captured)
                {
                    before = mygame::editor::CaptureTransformSnapshot(owner);
                    captured = true;
                }
            };

        // Logical quad size
        float size[2] = { render.w, render.h };
        if (ImGui::DragFloat2("Size", size, 1.0f, 0.0f, 10000.0f, "%.1f"))
        {
            capture();
            render.w = size[0];
            render.h = size[1];
        }

        // Tint color
        float color[4] = { render.r, render.g, render.b, render.a };
        if (ImGui::ColorEdit4("Tint", color))
        {
            render.r = color[0];
            render.g = color[1];
            render.b = color[2];
            render.a = color[3];
        }

        // Visibility flag
        ImGui::Checkbox("Visible", &render.visible);

        int blendModeIndex = static_cast<int>(render.blendMode);
        if (ImGui::Combo("Blend Mode", &blendModeIndex, Framework::kBlendModeLabels.data(),
            static_cast<int>(Framework::kBlendModeLabels.size())))
        {
            render.blendMode = static_cast<Framework::BlendMode>(blendModeIndex);
        }

        if (captured)
            mygame::editor::RecordTransformChange(owner, before);
    }

    /*************************************************************************************
      \brief Draws ImGui controls for CircleRenderComponent with undo support.

      Exposes:
      - Radius of the circle.
      - RGBA color.
    *************************************************************************************/
    void DrawCircleRenderSection(Framework::GOC& owner, CircleRenderComponent& circle)
    {
        if (!ImGui::CollapsingHeader("Circle Render", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        mygame::editor::TransformSnapshot before{};
        bool captured = false;
        auto capture = [&]()
            {
                if (!captured)
                {
                    before = mygame::editor::CaptureTransformSnapshot(owner);
                    captured = true;
                }
            };

        // Radius
        if (ImGui::DragFloat("Radius", &circle.radius, 0.05f, 0.0f, 1000.0f, "%.2f"))
            capture();

        // Color
        float color[4] = { circle.r, circle.g, circle.b, circle.a };
        if (ImGui::ColorEdit4("Color", color))
        {
            circle.r = color[0];
            circle.g = color[1];
            circle.b = color[2];
            circle.a = color[3];
        }

        if (captured)
            mygame::editor::RecordTransformChange(owner, before);
    }

    /*************************************************************************************
      \brief Draws ImGui controls for GlowComponent.

      Exposes:
      - Color, opacity, and brightness.
      - Inner/outer radius and falloff exponent.
      - Visibility toggle and point count.
    *************************************************************************************/
    void DrawGlowSection(Framework::GOC& owner, GlowComponent& glow)
    {
        if (!ImGui::CollapsingHeader("Glow", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        mygame::editor::TransformSnapshot before{};
        bool captured = false;
        auto capture = [&]()
            {
                if (!captured)
                {
                    before = mygame::editor::CaptureTransformSnapshot(owner);
                    captured = true;
                }
            };

        float color[3] = { glow.r, glow.g, glow.b };
        if (ImGui::ColorEdit3("Color", color))
        {
            glow.r = color[0];
            glow.g = color[1];
            glow.b = color[2];
        }

        ImGui::DragFloat("Opacity", &glow.opacity, 0.01f, 0.0f, 1.0f, "%.2f");
        ImGui::DragFloat("Brightness", &glow.brightness, 0.05f, 0.0f, 10.0f, "%.2f");

        if (ImGui::DragFloat("Inner Radius", &glow.innerRadius, 0.005f, 0.0f, 1000.0f, "%.3f"))
            capture();
        if (ImGui::DragFloat("Outer Radius", &glow.outerRadius, 0.005f, 0.0f, 1000.0f, "%.3f"))
            capture();
        ImGui::DragFloat("Falloff Exponent", &glow.falloffExponent, 0.05f, 0.01f, 8.0f, "%.2f");
        ImGui::Checkbox("Visible", &glow.visible);

        ImGui::Text("Points: %zu", glow.points.size());
        if (ImGui::Button("Clear Points"))
            glow.points.clear();

        if (captured)
            mygame::editor::RecordTransformChange(owner, before);
    }

    /*************************************************************************************
      \brief Draws ImGui controls for FlashComponent.

      Exposes:
      - Flicker frequency.
      - Total flash duration (0 = indefinite).
      - Whether the flash starts on the visible half-cycle.
      - Whether flashing begins when enemies are cleared.
      - Whether the object stays hidden until flashing is activated.
    *************************************************************************************/
    void DrawFlashSection(FlashComponent& flash)
    {
        if (!ImGui::CollapsingHeader("Flash", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::DragFloat("Frequency", &flash.frequency, 0.1f, 0.0f, 60.0f, "%.2f");
        ImGui::DragFloat("Duration", &flash.duration, 0.1f, 0.0f, 120.0f, "%.2f");
        ImGui::Checkbox("Start Visible", &flash.start_visible);
        ImGui::Checkbox("Activate On Enemy Clear", &flash.activate_on_enemy_clear);
        ImGui::Checkbox("Hide Until Activated", &flash.hide_until_activated);
        ImGui::TextDisabled("Duration 0 means flash forever.");
    }

    /*************************************************************************************
      \brief Draws ImGui controls for SpriteComponent.

      Exposes:
      - texture_key used by Resource_Manager.
      - path used to load that texture (for hot-reload / reimport).
    *************************************************************************************/
    void DrawSpriteSection(SpriteComponent& sprite)
    {
        if (!ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        // Local buffers used to edit string fields.
        static std::array<char, 256> keyBuffer{};
        static std::array<char, 256> pathBuffer{};

        std::snprintf(keyBuffer.data(), keyBuffer.size(), "%s", sprite.texture_key.c_str());
        std::snprintf(pathBuffer.data(), pathBuffer.size(), "%s", sprite.path.c_str());

        if (ImGui::InputText("Texture Key", keyBuffer.data(), keyBuffer.size()))
            sprite.texture_key = keyBuffer.data();
        if (ImGui::InputText("Texture Path", pathBuffer.data(), pathBuffer.size()))
            sprite.path = pathBuffer.data();
    }

    void DrawShadowSection(Framework::GOC& owner, ShadowComponent& shadow)
    {
        if (!ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::Checkbox("Enabled##Shadow", &shadow.enabled);

        float offset[2] = { shadow.offsetX, shadow.offsetY };
        if (ImGui::DragFloat2("Offset##Shadow", offset, 0.01f, -1000.0f, 1000.0f, "%.3f"))
        {
            shadow.offsetX = offset[0];
            shadow.offsetY = offset[1];
        }

        float scale[2] = { shadow.scaleX, shadow.scaleY };
        if (ImGui::DragFloat2("Scale##Shadow", scale, 0.01f, -1000.0f, 1000.0f, "%.3f"))
        {
            shadow.scaleX = scale[0];
            shadow.scaleY = scale[1];
        }

        ImGui::Checkbox("Flip Y##Shadow", &shadow.flipY);

        float color[4] = { shadow.r, shadow.g, shadow.b, shadow.a };
        if (ImGui::ColorEdit4("Tint##Shadow", color))
        {
            shadow.r = color[0];
            shadow.g = color[1];
            shadow.b = color[2];
            shadow.a = color[3];
        }

        int blendModeIndex = static_cast<int>(shadow.blendMode);
        if (ImGui::Combo("Blend Mode##Shadow", &blendModeIndex, Framework::kBlendModeLabels.data(),
            static_cast<int>(Framework::kBlendModeLabels.size())))
        {
            shadow.blendMode = static_cast<Framework::BlendMode>(blendModeIndex);
        }

        static Framework::GOCId lastShadowSelection = 0;
        static std::array<char, 64> shadowLayerBuffer{};
        if (lastShadowSelection != owner.GetId())
        {
            std::snprintf(shadowLayerBuffer.data(), shadowLayerBuffer.size(), "%s", shadow.layerName.c_str());
            lastShadowSelection = owner.GetId();
        }

        if (ImGui::InputText("Shadow Layer", shadowLayerBuffer.data(), shadowLayerBuffer.size()))
            shadow.layerName = shadowLayerBuffer.data();

        ImGui::TextDisabled("Leave blank to use the object's layer.");
    }

    /*************************************************************************************
      \brief Draws ImGui controls for EnemyTypeComponent (physical / ranged).
    *************************************************************************************/
    void DrawEnemyTypeSection(EnemyTypeComponent& type)
    {
        if (!ImGui::CollapsingHeader("EnemyType", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        const char* typeItems[] = { "physical", "ranged" };
        int current = (type.Etype == EnemyTypeComponent::EnemyType::physical) ? 0 : 1;
        if (ImGui::Combo("Type", &current, typeItems, IM_ARRAYSIZE(typeItems)))
        {
            type.Etype = (current == 0)
                ? EnemyTypeComponent::EnemyType::physical
                : EnemyTypeComponent::EnemyType::ranged;
        }
    }

    void DrawBehaviourSection(const GOC& owner, BehaviourComponent& behaviour)
    {
        if (!ImGui::CollapsingHeader("Behaviour", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        const auto behaviourKeys = GetAllowedBehaviourKeys(owner, behaviour);
        const char* preview = behaviour.behaviourKey.empty() ? "<none>" : behaviour.behaviourKey.c_str();

        if (ImGui::BeginCombo("Behaviour Key", preview))
        {
            for (const char* key : behaviourKeys)
            {
                const bool selected = (behaviour.behaviourKey == key);
                if (ImGui::Selectable(key, selected))
                    behaviour.behaviourKey = key;
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::TextDisabled("Started: %s", behaviour.started ? "true" : "false");
    }

    void DrawGateTargetSection(GateTargetComponent& gate)
    {
        if (!ImGui::CollapsingHeader("Gate Target", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        const auto levels = GetAvailableLevelFiles();
        const char* preview = gate.levelPath.empty() ? "<none>" : gate.levelPath.c_str();

        if (levels.empty())
        {
            ImGui::TextDisabled("No level files found.");
            return;
        }

        if (ImGui::BeginCombo("Level Path", preview))
        {
            for (const auto& level : levels)
            {
                const bool selected = (gate.levelPath == level);
                if (ImGui::Selectable(level.c_str(), selected))
                    gate.levelPath = level;
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

} // anonymous namespace

namespace mygame
{
    /*************************************************************************************
      \brief Main entry point for the Properties Editor ImGui window.

      Responsibilities:
      - Validate the current selection against the Factory.
      - Allow renaming of the selected object and changing its logical layer.
      - Show basic metadata (ID).
      - Dispatch to per-component drawer functions if components exist:
        * Transform / Render / CircleRender / Sprite / RigidBody / HitBox
        * PlayerAttack / EnemyAttack / PlayerHealth / EnemyHealth / EnemyType
        * Audio
    *************************************************************************************/
    void DrawPropertiesEditor()
    {
        using namespace Framework;

        // If factory is not initialized, nothing to inspect.
        if (!FACTORY)
            return;

        // Validate that the selected object still exists.
        if (mygame::HasSelectedObject())
        {
            const auto selectedId = mygame::GetSelectedObjectId();
            const auto& objects = FACTORY->Objects();
            if (objects.find(selectedId) == objects.end())
            {
                // Selected object was deleted; clear selection.
                mygame::ClearSelection();
            }
        }

        // Resolve pointer to selected object (if any).
        GOC* object = nullptr;
        if (mygame::HasSelectedObject())
        {
            object = FACTORY->GetObjectWithId(mygame::GetSelectedObjectId());
        }

        // Begin properties window
        if (!ImGui::Begin("Properties Editor"))
        {
            ImGui::End();
            return;
        }

        // No object ? show hint text and early out.
        if (!object)
        {
            ImGui::TextDisabled("No object selected.");
            ImGui::End();
            return;
        }

        // ---------------------------------------------------------------------
        // Object header: name, layer, id
        // ---------------------------------------------------------------------
        static GOCId lastSelection = 0;
        static std::array<char, 128> nameBuffer{};
        static std::array<char, 64> layerBuffer{};

        // If user selected a different object, refresh the edit buffers.
        if (lastSelection != object->GetId())
        {
            std::snprintf(nameBuffer.data(), nameBuffer.size(), "%s", object->GetObjectName().c_str());
            std::snprintf(layerBuffer.data(), layerBuffer.size(), "%s", object->GetLayerName().c_str());
            lastSelection = object->GetId();
        }

        // Editable object name
        if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size()))
            object->SetObjectName(nameBuffer.data());

        // Editable layer name (maps to your engine's logical layer system)
        if (ImGui::InputText("Layer", layerBuffer.data(), layerBuffer.size()))
            object->SetLayerName(layerBuffer.data());

        // Display read-only ID
        ImGui::TextDisabled("ID: %u", object->GetId());
        ImGui::Separator();

        // ---------------------------------------------------------------------
        // Component sections (each is optional and only drawn if present)
        // ---------------------------------------------------------------------
        if (auto* transform = object->GetComponentAs<TransformComponent>(ComponentTypeId::CT_TransformComponent))
            DrawTransformSection(*object, *transform);

        if (auto* render = object->GetComponentAs<RenderComponent>(ComponentTypeId::CT_RenderComponent))
            DrawRenderSection(*object, *render);

        if (auto* circle = object->GetComponentAs<CircleRenderComponent>(ComponentTypeId::CT_CircleRenderComponent))
            DrawCircleRenderSection(*object, *circle);

        if (auto* glow = object->GetComponentAs<GlowComponent>(ComponentTypeId::CT_GlowComponent))
            DrawGlowSection(*object, *glow);

        if (auto* flash = object->GetComponentAs<FlashComponent>(ComponentTypeId::CT_FlashComponent))
            DrawFlashSection(*flash);

        if (auto* sprite = object->GetComponentAs<SpriteComponent>(ComponentTypeId::CT_SpriteComponent))
            DrawSpriteSection(*sprite);

        if (auto* shadow = object->GetComponentAs<ShadowComponent>(ComponentTypeId::CT_ShadowComponent))
            DrawShadowSection(*object, *shadow);

        if (auto* rb = object->GetComponentAs<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent))
            DrawRigidBodySection(*object, *rb);

        if (auto* hit = object->GetComponentAs<HitBoxComponent>(ComponentTypeId::CT_HitBoxComponent))
            DrawHitBoxSection(*hit);

        if (auto* pAtk = object->GetComponentAs<PlayerAttackComponent>(ComponentTypeId::CT_PlayerAttackComponent))
            DrawPlayerAttackSection(*pAtk);

        if (auto* eAtk = object->GetComponentAs<EnemyAttackComponent>(ComponentTypeId::CT_EnemyAttackComponent))
            DrawEnemyAttackSection(*eAtk);

        if (auto* pHp = object->GetComponentAs<PlayerHealthComponent>(ComponentTypeId::CT_PlayerHealthComponent))
            DrawPlayerHealthSection(*pHp);

        if (auto* eHp = object->GetComponentAs<EnemyHealthComponent>(ComponentTypeId::CT_EnemyHealthComponent))
            DrawEnemyHealthSection(*eHp);

        if (auto* eType = object->GetComponentAs<EnemyTypeComponent>(ComponentTypeId::CT_EnemyTypeComponent))
            DrawEnemyTypeSection(*eType);

        if (auto* behaviour = object->GetComponentAs<BehaviourComponent>(ComponentTypeId::CT_BehaviourComponent))
            DrawBehaviourSection(*object, *behaviour);

        if (auto* gate = object->GetComponentAs<GateTargetComponent>(ComponentTypeId::CT_GateTargetComponent))
            DrawGateTargetSection(*gate);

        if (auto* audio = object->GetComponentAs<AudioComponent>(ComponentTypeId::CT_AudioComponent))
            DrawAudioSection(*audio);

        ImGui::End();
    }
}

#endif // SOFASPUDS_ENABLE_EDITOR

