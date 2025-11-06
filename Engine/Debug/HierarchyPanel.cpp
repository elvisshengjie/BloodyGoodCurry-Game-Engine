/*********************************************************************************************
 \file      HierarchyPanel.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Implements the ImGui hierarchy panel that lists all active GameObjects
            with selection, hover, and context menu functionality.

 \details
            The Hierarchy panel displays a scrollable list of all GameObjects from
            Factory::Objects() in a two-column table (Name / ID). It synchronizes with
            the global editor selection and allows clicking to select objects, showing
            tooltips with name and ID details, and providing context menu actions such
            as Delete. Display names prefer object names, falling back to "<unnamed>"
            placeholders when not set.

            Responsibilities:
            - Render a live ImGui table of GameObjects.
            - Maintain valid global selection (clear if object deleted).
            - Display hover tooltips and highlight hovered items.
            - Provide context menu actions like Delete.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "HierarchyPanel.h"
#include "Factory/Factory.h"
#include "Composition/Composition.h"
#include "Selection.h"

#include <imgui.h>
#include <string>

namespace
{
    /*****************************************************************************************
      \brief Produces a stable display name for a GameObject.
      \param obj      Pointer to the GameObject (may be null).
      \param storage  Temporary string buffer that owns the returned memory.
      \return A const char* backed by \p storage; "<null object>" or "<unnamed>" if needed.
    *****************************************************************************************/
    [[nodiscard]] const char* DisplayName(const Framework::GOC* obj, std::string& storage)
    {
        if (!obj)
        {
            storage = "<null object>";
            return storage.c_str();
        }

        const std::string& name = obj->GetObjectName();
        if (!name.empty())
        {
            storage = name;
        }
        else
        {
            storage = "<unnamed>";
        }
        return storage.c_str();
    }
}

/*****************************************************************************************
  \brief Draws the Hierarchy panel window (Name/ID table + selection maintenance).
  \details
    - Verifies that the current global selection still exists; clears it if not.
    - Renders a 2-column table (Name, ID) with selectable rows.
    - Updates global selection via Selection.h helpers when a row is clicked.
    - Shows a tooltip with Name and ID on hover.
*****************************************************************************************/
void mygame::DrawHierarchyPanel()
{
    using namespace Framework;

    if (!FACTORY)
        return;

    mygame::SetHoverObjectId(0);

    const auto& objects = FACTORY->Objects();

    // Ensure global selection remains valid.
    if (mygame::HasSelectedObject())
    {
        Framework::GOCId selectedId = mygame::GetSelectedObjectId();
        if (objects.find(selectedId) == objects.end())
            mygame::ClearSelection();
    }

    if (ImGui::Begin("Hierarchy"))
    {
        ImGui::TextDisabled("Objects: %zu", objects.size());
        ImGui::Separator();

        if (objects.empty())
        {
            ImGui::TextDisabled("No objects available.");
        }
        else if (ImGui::BeginTable("HierarchyTable", 2,
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            std::string nameBuffer;
            for (const auto& [id, objPtr] : objects)
            {
                ImGui::TableNextRow();

                const GOC* obj = objPtr.get();
                const char* displayName = DisplayName(obj, nameBuffer);

                // Column 0: Name + selectable row
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(static_cast<int>(id));

                const bool isSelected = (mygame::GetSelectedObjectId() == id);
                if (ImGui::Selectable(displayName, isSelected, ImGuiSelectableFlags_SpanAllColumns))
                    mygame::SetSelectedObjectId(id);

                // Hover: show tooltip and highlight
                if (ImGui::IsItemHovered())
                {
                    mygame::SetHoverObjectId(id);

                    ImGui::BeginTooltip();
                    ImGui::Text("Name : %s", displayName);
                    ImGui::Text("ID   : %u", id);
                    ImGui::EndTooltip();
                }

                // Right-click context menu
                if (ImGui::BeginPopupContextItem("##hier_ctx"))
                {
                    if (ImGui::MenuItem("Delete"))
                    {
                        if (auto* obj = FACTORY->GetObjectWithId(id))
                            FACTORY->Destroy(obj);  // Deferred delete at end of frame

                        if (mygame::GetSelectedObjectId() == id)
                            mygame::ClearSelection();
                    }
                    ImGui::EndPopup();
                }

                ImGui::PopID();

                // Column 1: ID
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%u", id);
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();
}
