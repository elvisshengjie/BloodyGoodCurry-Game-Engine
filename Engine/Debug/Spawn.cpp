#include "Debug/Spawn.h"

#include "imgui.h"

// Engine & game headers
#include "Factory/Factory.h"                    // FACTORY, GOC, ComponentTypeId
#include "Composition/PrefabManager.h"          // master_copies, ClonePrefab

// Component types you currently support; add more here as you create them
#include "Component/TransformComponent.h"
#include "Component/RenderComponent.h"
#include "Component/CircleRenderComponent.h"

#include <vector>
#include <string>

namespace mygame {

    using namespace Framework;

    // ------------------------------------------------------------
    // Helper: spawn one prefab and apply settings to present components
    // ------------------------------------------------------------
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

        // TODO: when you add new component types, set their fields here as well:
        // if (auto* ai = obj->GetComponentType<AIComponent>(ComponentTypeId::CT_AI)) { /* apply */ }
    }

    // ------------------------------------------------------------
    // Panel state (persists across frames)
    // ------------------------------------------------------------
    static std::string gSelectedPrefab = "Rect"; // default choice
    static SpawnSettings gS;                     // live tunables

    // ------------------------------------------------------------
    // UI: Draw the spawn panel
    // ------------------------------------------------------------
    void DrawSpawnPanel() {
        ImGui::Begin("Spawn");

        // Prefab dropdown populated from your master_copies map
        {
            const char* preview = gSelectedPrefab.c_str();
            if (ImGui::BeginCombo("Prefab", preview)) {
                for (auto const& kv : master_copies) {
                    bool sel = (kv.first == gSelectedPrefab);
                    if (ImGui::Selectable(kv.first.c_str(), sel)) gSelectedPrefab = kv.first;
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        // Resolve master to know what components exist for this prefab
        GOC* master = nullptr;
        if (auto it = master_copies.find(gSelectedPrefab); it != master_copies.end())
            master = it->second;

        if (!master) {
            ImGui::TextDisabled("Missing master for '%s'", gSelectedPrefab.c_str());
            ImGui::End();
            return;
        }

        const bool hasTransform = (master->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent) != nullptr);
        const bool hasRender = (master->GetComponentType<RenderComponent>(ComponentTypeId::CT_RenderComponent) != nullptr);
        const bool hasCircle = (master->GetComponentType<CircleRenderComponent>(ComponentTypeId::CT_CircleRenderComponent) != nullptr);

        // Common transform if supported
        if (hasTransform) {
            ImGui::SeparatorText("Transform");
            ImGui::DragFloat("x", &gS.x, 0.005f, 0.0f, 1.0f);
            ImGui::DragFloat("y", &gS.y, 0.005f, 0.0f, 1.0f);
            ImGui::DragFloat("rot (rad)", &gS.rot, 0.01f, -3.14159f, 3.14159f);
        }

        // Rect controls
        if (hasRender) {
            ImGui::SeparatorText("Rect");
            ImGui::DragFloat("w", &gS.w, 0.005f, 0.01f, 1.0f);
            ImGui::DragFloat("h", &gS.h, 0.005f, 0.01f, 1.0f);
        }

        // Circle controls
        if (hasCircle) {
            ImGui::SeparatorText("Circle");
            ImGui::DragFloat("radius", &gS.radius, 0.005f, 0.01f, 1.0f);
        }

        // Color if any renderable component exists
        if (hasRender || hasCircle) {
            ImGui::SeparatorText("Color");
            ImGui::ColorEdit4("rgba", gS.rgba);
        }

        // Batch settings
        ImGui::SeparatorText("Batch");
        ImGui::DragInt("count", &gS.count, 1, 1, 500);
        ImGui::DragFloat("stepX", &gS.stepX, 0.005f);
        ImGui::DragFloat("stepY", &gS.stepY, 0.005f);

        // Actions
        if (ImGui::Button("Spawn")) {
            for (int i = 0; i < gS.count; ++i)
                SpawnOnePrefab(gSelectedPrefab.c_str(), gS, i);
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear All (keep masters)")) {
            std::vector<GOC*> toKill;
            toKill.reserve(FACTORY->Objects().size());
            for (auto& [id, obj] : FACTORY->Objects()) {
                bool isMaster = false;
                for (auto const& kv : master_copies) { if (kv.second == obj) { isMaster = true; break; } }
                if (!isMaster) toKill.push_back(obj);
            }
            for (auto* o : toKill) o->Destroy();
            FACTORY->Update(0.0f); // sweep now so we don't touch dead objects this frame
        }

        ImGui::End();
    }

} // namespace mygame
