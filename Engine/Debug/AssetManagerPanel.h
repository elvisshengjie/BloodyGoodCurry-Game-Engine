/*********************************************************************************************
 \file      AssetManagerPanel.h
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%


 \brief     Declares the editor UI entry point for the Asset Manager panel.

 \details   The Asset Manager panel exposes a lightweight ImGui workflow for inspecting
            registered assets, loading them through the resource manager, deleting them
            from disk, and creating prefab JSON assets from editor templates. The panel
            coordinates with other editor surfaces such as the JSON editor and asset
            browser so they stay in sync after asset operations complete.

 \copyright
            All content 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

#include "JsonEditorPanel.h"

namespace mygame {
    class AssetBrowserPanel;

    /*****************************************************************************************
      \brief Draws the Asset Manager editor panel using ImGui.
      \param jsonPanel Optional JsonEditorPanel to refresh after create/delete operations.
      \param assetBrowserPanel Optional AssetBrowserPanel to refresh after asset deletion.
      \details
          - Shows a refreshable list of assets returned by AssetManager.
          - Provides a search box to filter entries by name.
          - Displays metadata for the currently selected asset.
          - Allows loading the selected asset through Resource_Manager.
          - Supports deleting assets with a confirmation popup.
          - Supports creating object or enemy prefab JSON assets from editor templates.
    *****************************************************************************************/
    void DrawAssetManagerPanel(JsonEditorPanel* jsonPanel, AssetBrowserPanel* assetBrowserPanel);
}
