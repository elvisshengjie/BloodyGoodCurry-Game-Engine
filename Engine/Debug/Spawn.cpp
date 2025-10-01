/*********************************************************************************************
 \file      Spawn.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Implements a debug ImGui panel for spawning prefabs at runtime. Provides
            interactive controls for position, size, color, texture, and batch spawning.
            Integrates with the engine’s prefab and factory systems.

 \details   This module allows developers to spawn prefabs interactively during runtime
            for testing and debugging. Prefabs are drawn from PrefabManager’s registry
            (master_copies). The user selects a prefab type, configures its parameters
            (transform, render, circle, sprite), and spawns instances via ImGui. Supports
            batch spawning with configurable offsets.


 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "Debug/Spawn.h"

#include "imgui.h"
#include "Debug/Perf.h"
// Engine & game headers
#include "Factory/Factory.h"                    // FACTORY, GOC, ComponentTypeId
#include "Composition/PrefabManager.h"          // master_copies, ClonePrefab

// Component types you currently support
#include "Component/TransformComponent.h"
#include "Component/RenderComponent.h"
#include "Component/CircleRenderComponent.h"
#include "Component/SpriteComponent.h"

#include <vector>
#include <string>

namespace mygame {
    /// Currently selected sprite texture key (shared across panel sessions).
    static std::string sSpriteTexKey;
    using namespace Framework;

    /*************************************************************************************
      \brief Helper to spawn a single prefab and apply current SpawnSettings.
      \param prefab The name of the prefab to clone.
      \param s      The spawn settings (position, size, color, etc.).
      \param index  Index used for batch offsets.
      \details
        - Clones the prefab via PrefabManager.
        - Applies transform, render, circle, or sprite settings if present.
        - Extensible for future component types (AI, physics, etc.).
    *************************************************************************************/
    static void SpawnOnePrefab(const char* prefab, SpawnSettings const& s, int index) {
        GOC* obj = ClonePrefab(prefab);
        if (!obj) return;

        const float x = s.x + s.stepX * index;
        const float y = s.y + s.stepY * index;

        if (auto* tr = obj->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent)) {
            tr->x = x; tr->y = y; tr->rot = s.rot;
        }
        if (auto* rc = obj->GetComponentType<RenderComponent>(ComponentTypeId::CT_RenderComponent)) {
            rc->w = s.w; rc->h = s.h;
            rc->r = s.rgba[0]; rc->g = s.rgba[1]; rc->b = s.rgba[2]; rc->a = s.rgba[3];
        }
        if (auto* cc = obj->GetComponentType<CircleRenderComponent>(ComponentTypeId::CT_CircleRenderComponent)) {
            cc->radius = s.radius;
            cc->r = s.rgba[0]; cc->g = s.rgba[1]; cc->b = s.rgba[2]; cc->a = s.rgba[3];
        }

        if (auto* sp = obj->GetComponentType<SpriteComponent>(ComponentTypeId::CT_SpriteComponent)) {
            if (!sSpriteTexKey.empty()) {
                sp->texture_key = sSpriteTexKey;
                sp->texture_id = Resource_Manager::getTexture(sSpriteTexKey);
            }
        }

        // TODO: when you add new component types, set their fields here as well:
        // if (auto* ai = obj->GetComponentType<AIComponent>(ComponentTypeId::CT_AI)) { /* apply */ }
    }

    /// Panel state (persists across frames).
    static std::string gSelectedPrefab = "Rect"; ///< Default prefab choice.
    static SpawnSettings gS;                     ///< Live settings bound to ImGui controls.

    /*************************************************************************************
      \brief Draws the "Spawn" ImGui panel and handles prefab spawning actions.
      \details
        - Displays a dropdown of available prefabs from master_copies.
        - Shows ImGui controls for transform, render, circle, sprite components.
        - Provides color editing and batch spawning options.
        - "Spawn" button creates prefab instances.
        - "Clear All" removes all non-master objects from the factory.

      Example:
      \code
        // inside game loop
        ImGuiLayer::BeginFrame();
        mygame::DrawSpawnPanel();
        ImGuiLayer::EndFrame();
      \endcode
    *************************************************************************************/
    void DrawSpawnPanel() {
        ImGui::Begin("Spawn"); // Begin a new ImGui window called "Spawn"

        // Prefab dropdown populated from your master_copies map
        {
            const char* preview = gSelectedPrefab.c_str();
            if (ImGui::BeginCombo("Prefab", preview)) { // Begin a combo box (dropdown) with preview text
                for (auto const& kv : master_copies) {
                    bool sel = (kv.first == gSelectedPrefab);
                    if (ImGui::Selectable(kv.first.c_str(), sel)) // Create selectable items in dropdown
                        gSelectedPrefab = kv.first;
                    if (sel) ImGui::SetItemDefaultFocus(); // Highlight currently selected prefab
                }
                ImGui::EndCombo(); // End combo box
            }
        }

        // Resolve master prefab...
        GOC* master = nullptr;
        if (auto it = master_copies.find(gSelectedPrefab); it != master_copies.end())
            master = it->second;

        if (!master) {
            ImGui::TextDisabled("Missing master for '%s'", gSelectedPrefab.c_str()); // Display disabled text
            ImGui::End(); // End the "Spawn" window
            return;
        }

        const bool hasTransform = (master->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent) != nullptr);
        const bool hasRender = (master->GetComponentType<RenderComponent>(ComponentTypeId::CT_RenderComponent) != nullptr);
        const bool hasCircle = (master->GetComponentType<CircleRenderComponent>(ComponentTypeId::CT_CircleRenderComponent) != nullptr);
        const bool hasSprite = (master->GetComponentType<SpriteComponent>(ComponentTypeId::CT_SpriteComponent) != nullptr);

        // Sprite controls
        if (hasSprite) {
            ImGui::SeparatorText("Sprite"); // Draw a labeled separator line with "Sprite"

            const char* preview = sSpriteTexKey.empty() ? "<none>" : sSpriteTexKey.c_str();
            if (ImGui::BeginCombo("Texture", preview)) { // Dropdown for selecting a texture key
                for (auto const& kv : Resource_Manager::resources_map) {
                    if (kv.second.type != Resource_Manager::Resource_Type::Graphics) continue;
                    bool sel = (kv.first == sSpriteTexKey);
                    if (ImGui::Selectable(kv.first.c_str(), sel)) // Show texture options
                        sSpriteTexKey = kv.first;
                    if (sel) ImGui::SetItemDefaultFocus(); // Keep selected texture highlighted
                }
                ImGui::EndCombo();
            }
        }

        // Transform controls
        if (hasTransform) {
            ImGui::SeparatorText("Transform");
            ImGui::DragFloat("x", &gS.x, 0.005f, 0.0f, 1.0f);   // Slider-like float drag for X position
            ImGui::DragFloat("y", &gS.y, 0.005f, 0.0f, 1.0f);   // Slider-like float drag for Y position
            ImGui::DragFloat("rot (rad)", &gS.rot, 0.01f, -3.14159f, 3.14159f); // Rotation in radians
        }

        // Rect controls
        if (hasRender) {
            ImGui::SeparatorText("Rect");
            ImGui::DragFloat("w", &gS.w, 0.005f, 0.01f, 1.0f); // Adjust width
            ImGui::DragFloat("h", &gS.h, 0.005f, 0.01f, 1.0f); // Adjust height
        }

        // Circle controls
        if (hasCircle) {
            ImGui::SeparatorText("Circle");
            ImGui::DragFloat("radius", &gS.radius, 0.005f, 0.01f, 1.0f); // Adjust circle radius
        }

        // Color picker
        if (hasRender || hasCircle) {
            ImGui::SeparatorText("Color");
            ImGui::ColorEdit4("rgba", gS.rgba); // RGBA color picker with preview
        }

        // Batch settings
        ImGui::SeparatorText("Batch");
        ImGui::DragInt("count", &gS.count, 1, 1, 500); // Number of objects to spawn
        ImGui::DragFloat("stepX", &gS.stepX, 0.005f); // X offset step between spawns
        ImGui::DragFloat("stepY", &gS.stepY, 0.005f); // Y offset step between spawns

        // Action buttons
        if (ImGui::Button("Spawn")) { // Button to spawn objects
            for (int i = 0; i < gS.count; ++i)
                SpawnOnePrefab(gSelectedPrefab.c_str(), gS, i);
        }

        ImGui::SameLine(); // Place next widget on same horizontal line
        if (ImGui::Button("Clear All (keep masters)")) { // Button to delete all non-master objects
            std::vector<GOC*> toKill;
            toKill.reserve(FACTORY->Objects().size());
            for (auto& [id, obj] : FACTORY->Objects()) {
                bool isMaster = false;
                for (auto const& kv : master_copies) { if (kv.second == obj) { isMaster = true; break; } }
                if (!isMaster) toKill.push_back(obj);
            }
            for (auto* o : toKill) o->Destroy();
            FACTORY->Update(0.0f); // Immediately sweep destroyed objects
        }
        ImGui::SeparatorText("Counts");
        size_t totalObjs = Framework::FACTORY ? Framework::FACTORY->Objects().size() : 0;
   

  
        ImGui::Text("Total objects:   %zu", totalObjs);
        //In-game performance window
        Framework::DrawInCurrentWindow();


        ImGui::End(); // End the "Spawn" window
    }


} // namespace mygame
