/*********************************************************************************************
 \file      ParticlePresetEditor.cpp
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Primary Author, 100%
 \brief     Implements a live particle/VFX preset editor with preview controls.

 \copyright
            All content (c) 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "ParticlePresetEditor.h"

#if SOFASPUDS_ENABLE_EDITOR

#include "ParticlePresets.hpp"
#include "ParticleVfxPresetPersistence.h"
#include "VfxPresets.hpp"

#include "Component/TransformComponent.h"
#include "Debug/Selection.h"
#include "Factory/Factory.h"
#include "Systems/ParticleSystem.h"

#include <imgui.h>
#include <glm/vec2.hpp>
#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>

namespace mygame
{
    namespace
    {
        struct PreviewControlState
        {
            bool autoPreview{ false };
            float interval{ 0.45f };
            float timer{ 0.0f };
        };

        struct ParticleEditorState
        {
            bool useSelectedObject{ true };
            glm::vec2 manualPreviewPos{ 0.0f, 0.0f };
            float runFacingDir{ 1.0f };
            std::string persistenceStatus{};
            bool persistenceStatusIsError{ false };
            PreviewControlState enemyDeath{};
            PreviewControlState runTrail{};
            PreviewControlState hitImpact{};
        };

        ParticleEditorState gEditorState;

        void SetPersistenceStatus(std::string status, bool isError)
        {
            gEditorState.persistenceStatus = std::move(status);
            gEditorState.persistenceStatusIsError = isError;
        }

        bool ResolveSelectedObjectPreviewPos(glm::vec2& outPos)
        {
            if (!Framework::FACTORY || !mygame::HasSelectedObject())
                return false;

            const Framework::GOCId selectedId = mygame::GetSelectedObjectId();
            const auto& objects = Framework::FACTORY->Objects();
            const auto it = objects.find(selectedId);
            if (it == objects.end() || !it->second)
                return false;

            auto* transform = it->second->GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent);
            if (!transform)
                return false;

            outPos = { transform->x, transform->y };
            return true;
        }

        glm::vec2 ResolvePreviewPosition(bool& usingSelectedObject)
        {
            glm::vec2 previewPos = gEditorState.manualPreviewPos;
            usingSelectedObject = false;

            if (gEditorState.useSelectedObject && ResolveSelectedObjectPreviewPos(previewPos))
                usingSelectedObject = true;

            return previewPos;
        }

        bool HasParticleSystem()
        {
            return Framework::ParticleSystem::Instance() != nullptr;
        }

        void DrawRangeControl(const char* label, float& minValue, float& maxValue, float speed,
            float clampMin, float clampMax, const char* format = "%.3f")
        {
            ImGui::PushID(label);
            ImGui::TextUnformatted(label);
            ImGui::DragFloat("Min", &minValue, speed, clampMin, clampMax, format);
            ImGui::DragFloat("Max", &maxValue, speed, clampMin, clampMax, format);
            if (minValue > maxValue)
                std::swap(minValue, maxValue);
            ImGui::PopID();
        }

        bool DrawPreviewControls(PreviewControlState& state, const char* previewButtonLabel)
        {
            ImGui::PushID(previewButtonLabel);
            const bool toggled = ImGui::Checkbox("Auto Preview", &state.autoPreview);
            if (toggled && state.autoPreview)
                state.timer = 0.0f;
            ImGui::DragFloat("Preview Interval", &state.interval, 0.01f, 0.05f, 3.0f, "%.2fs");
            state.interval = std::clamp(state.interval, 0.05f, 3.0f);
            if (ImGui::Button(previewButtonLabel))
            {
                state.timer = 0.0f;
                ImGui::PopID();
                return true;
            }

            ImGui::PopID();
            return false;
        }

        bool ShouldFirePreview(PreviewControlState& state, float dt, bool manualTriggered)
        {
            if (manualTriggered)
                return true;

            if (!state.autoPreview)
                return false;

            state.timer -= dt;
            if (state.timer > 0.0f)
                return false;

            state.timer = state.interval;
            return true;
        }

        void SpawnRunTrailPreview(Framework::ParticleSystem& particleSystem, const glm::vec2& worldPos, float facingDir)
        {
            const RunParticlePreset& preset = GetRunParticlePreset();
            const std::size_t previewCount = std::max<std::size_t>(preset.count, 5);
            const float dir = (facingDir >= 0.0f) ? 1.0f : -1.0f;

            for (int i = 0; i < 4; ++i)
            {
                glm::vec2 samplePos = worldPos;
                samplePos.x -= dir * (0.025f * static_cast<float>(i));
                samplePos.y -= 0.005f * static_cast<float>(i);
                SpawnRunParticles(particleSystem, samplePos, facingDir, previewCount);
            }
        }
    }

    void DrawParticlePresetEditor()
    {
        if (!ImGui::Begin("Particle/VFX Preset Editor"))
        {
            ImGui::End();
            return;
        }

        const float dt = ImGui::GetIO().DeltaTime;
        const bool hasParticleSystem = HasParticleSystem();
        bool usingSelectedObject = false;
        const glm::vec2 previewPos = ResolvePreviewPosition(usingSelectedObject);
        const auto presetFilePath = GetParticleVfxPresetFilePath();
        const std::string presetFilePathText = presetFilePath.string();

        ImGui::TextDisabled("Live editor for gameplay particle presets and combat impact VFX.");
        ImGui::TextWrapped("Saved presets are stored separately from level files: %s",
            presetFilePathText.c_str());
        if (ImGui::Button("Load Saved Presets"))
        {
            const bool fileExists = std::filesystem::exists(presetFilePath);
            if (!fileExists)
            {
                SetPersistenceStatus("No saved preset file found.", true);
            }
            else if (LoadParticleVfxPresetsFromDisk())
            {
                SetPersistenceStatus("Loaded particle/VFX presets from disk.", false);
            }
            else
            {
                SetPersistenceStatus("Failed to load particle/VFX presets from disk.", true);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Presets"))
        {
            if (SaveParticleVfxPresetsToDisk())
                SetPersistenceStatus("Saved particle/VFX presets to disk.", false);
            else
                SetPersistenceStatus("Failed to save particle/VFX presets to disk.", true);
        }
        if (!gEditorState.persistenceStatus.empty())
        {
            const ImVec4 statusColor = gEditorState.persistenceStatusIsError
                ? ImVec4(1.0f, 0.45f, 0.45f, 1.0f)
                : ImVec4(0.45f, 0.9f, 0.55f, 1.0f);
            ImGui::TextColored(statusColor, "%s", gEditorState.persistenceStatus.c_str());
        }
        ImGui::Checkbox("Use Selected Object", &gEditorState.useSelectedObject);
        ImGui::DragFloat2("Manual Preview Pos", &gEditorState.manualPreviewPos.x, 0.01f, -100.0f, 100.0f, "%.2f");
        ImGui::Text("Preview Target: %s", usingSelectedObject ? "Selected Object" : "Manual Position");
        ImGui::Text("Preview World Pos: %.2f, %.2f", previewPos.x, previewPos.y);
        if (!hasParticleSystem)
            ImGui::TextDisabled("ParticleSystem not available.");

        if (ImGui::CollapsingHeader("Enemy Death Burst", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& preset = GetEnemyDeathParticlePreset();
            int count = static_cast<int>(preset.count);
            ImGui::DragInt("Count", &count, 1.0f, 0, 256);
            preset.count = static_cast<std::size_t>(std::max(0, count));
            DrawRangeControl("Speed Range", preset.speedMin, preset.speedMax, 0.005f, 0.0f, 5.0f);
            DrawRangeControl("Life Range", preset.lifeMin, preset.lifeMax, 0.005f, 0.01f, 5.0f);
            DrawRangeControl("Radius Range", preset.radiusMin, preset.radiusMax, 0.001f, 0.001f, 1.0f);
            ImGui::DragFloat("Upward Bias", &preset.upwardVelocityBias, 0.005f, -2.0f, 2.0f, "%.3f");
            ImGui::ColorEdit3("Base Color", &preset.red);
            DrawRangeControl("Green Jitter", preset.greenJitterMin, preset.greenJitterMax, 0.001f, -1.0f, 1.0f);
            ImGui::DragFloat("Start Alpha", &preset.startAlpha, 0.01f, 0.0f, 1.0f, "%.2f");
            ImGui::DragFloat("End Alpha", &preset.endAlpha, 0.01f, 0.0f, 1.0f, "%.2f");
            ImGui::DragFloat("End Radius Scale", &preset.endRadiusScale, 0.01f, 0.0f, 4.0f, "%.2f");

            const bool previewNow = DrawPreviewControls(gEditorState.enemyDeath, "Preview Enemy Death");
            ImGui::SameLine();
            if (ImGui::Button("Reset Enemy Death"))
                ResetEnemyDeathParticlePreset();

            if (hasParticleSystem && ShouldFirePreview(gEditorState.enemyDeath, dt, previewNow))
                SpawnEnemyDeathParticles(*Framework::ParticleSystem::Instance(), previewPos);
        }

        if (ImGui::CollapsingHeader("Run Trail", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& preset = GetRunParticlePreset();
            int count = static_cast<int>(preset.count);
            ImGui::DragInt("Count##Run", &count, 1.0f, 0, 128);
            preset.count = static_cast<std::size_t>(std::max(0, count));
            DrawRangeControl("Speed Range##Run", preset.speedMin, preset.speedMax, 0.005f, 0.0f, 5.0f);
            DrawRangeControl("Life Range##Run", preset.lifeMin, preset.lifeMax, 0.005f, 0.01f, 5.0f);
            DrawRangeControl("Size Range##Run", preset.sizeMin, preset.sizeMax, 0.001f, 0.001f, 2.0f);
            DrawRangeControl("Jitter Range##Run", preset.jitterMin, preset.jitterMax, 0.001f, -1.0f, 1.0f);
            DrawRangeControl("Rise Range##Run", preset.riseMin, preset.riseMax, 0.001f, -2.0f, 2.0f);
            ImGui::DragFloat("Offset X", &preset.offsetX, 0.005f, -2.0f, 2.0f, "%.3f");
            ImGui::DragFloat("Offset Y", &preset.offsetY, 0.005f, -2.0f, 2.0f, "%.3f");
            ImGui::DragFloat("End Size Scale", &preset.endSizeScale, 0.01f, 0.0f, 4.0f, "%.2f");
            ImGui::ColorEdit3("Tint##Run", &preset.red);
            ImGui::DragFloat("Start Alpha##Run", &preset.startAlpha, 0.01f, 0.0f, 1.0f, "%.2f");
            ImGui::DragFloat("End Alpha##Run", &preset.endAlpha, 0.01f, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Facing Dir", &gEditorState.runFacingDir, -1.0f, 1.0f, "%.2f");

            const bool previewNow = DrawPreviewControls(gEditorState.runTrail, "Preview Run Trail");
            ImGui::SameLine();
            if (ImGui::Button("Reset Run Trail"))
                ResetRunParticlePreset();

            if (hasParticleSystem && ShouldFirePreview(gEditorState.runTrail, dt, previewNow))
                SpawnRunTrailPreview(*Framework::ParticleSystem::Instance(), previewPos, gEditorState.runFacingDir);
        }

        if (ImGui::CollapsingHeader("Hit Impact VFX", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& preset = GetHitImpactBurstPreset();
            ImGui::DragInt("Count##Hit", &preset.count, 1.0f, 0, 128);
            DrawRangeControl("Speed Range##Hit", preset.speedMin, preset.speedMax, 0.005f, 0.0f, 5.0f);
            DrawRangeControl("Life Range##Hit", preset.lifeMin, preset.lifeMax, 0.005f, 0.01f, 5.0f);
            DrawRangeControl("Radius Range##Hit", preset.radiusMin, preset.radiusMax, 0.001f, 0.001f, 1.0f);
            DrawRangeControl("Spawn Offset##Hit", preset.offsetMin, preset.offsetMax, 0.001f, -1.0f, 1.0f);
            ImGui::DragFloat("End Radius Scale##Hit", &preset.endRadiusScale, 0.01f, 0.0f, 4.0f, "%.2f");
            ImGui::ColorEdit3("Burst Color", &preset.red);
            ImGui::DragFloat("Start Alpha##Hit", &preset.startAlpha, 0.01f, 0.0f, 1.0f, "%.2f");
            ImGui::DragFloat("End Alpha##Hit", &preset.endAlpha, 0.01f, 0.0f, 1.0f, "%.2f");

            const bool previewNow = DrawPreviewControls(gEditorState.hitImpact, "Preview Hit Impact");
            ImGui::SameLine();
            if (ImGui::Button("Reset Hit Impact"))
                ResetHitImpactBurstPreset();

            if (ShouldFirePreview(gEditorState.hitImpact, dt, previewNow))
                SpawnHitImpactPreview(previewPos);
        }

        ImGui::End();
    }
}

#else

namespace mygame
{
    void DrawParticlePresetEditor() {}
}

#endif
