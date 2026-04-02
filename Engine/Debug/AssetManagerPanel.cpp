/*********************************************************************************************
 \file      AssetManagerPanel.cpp
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%


 \brief     Implements the ImGui Asset Manager editor panel.

 \details   This panel gives editor users a compact workflow for browsing the current
            AssetManager registry, filtering entries by name, loading selected assets,
            deleting assets with confirmation, and creating prefab JSON files from the
            built-in object/enemy templates. It also refreshes related editor panels
            after mutating asset data so their file listings remain current.

 \copyright
            All content 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#if SOFASPUDS_ENABLE_EDITOR
#include "Resource_Asset_Manager/Asset_Manager.h"
#include "Resource_Asset_Manager/Resource_Manager.h"
#include "JsonEditorPanel.h"
#include "AssetBrowserPanel.h"
#include <filesystem>
#include <vector>
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include "Common/CRTDebug.h"   // <- bring in DBG_NEW
#ifdef _DEBUG
#define new DBG_NEW       // <- redefine new AFTER all includes
#endif

namespace mygame
{
	/*****************************************************************************************
	  \brief Draws the Asset Manager editor panel using ImGui.
	  \param jsonPanel Optional JsonEditorPanel to refresh after create/delete operations.
	  \param assetBrowserPanel Optional AssetBrowserPanel to refresh after asset deletion.
	  \details
		  - Renders a refreshable list of all assets known to AssetManager.
		  - Filters asset names through a case-insensitive search field.
		  - Shows the selected asset path and asset-type enum value.
		  - Allows the selected asset to be loaded through Resource_Manager.
		  - Deletes the selected asset only after explicit popup confirmation.
		  - Creates object or enemy prefab JSON assets from editor-side templates.
		  - Refreshes linked editor panels after successful create/delete operations.
	*****************************************************************************************/
	void DrawAssetManagerPanel(JsonEditorPanel* jsonPanel, AssetBrowserPanel* assetBrowserPanel)
	{
		ImGui::Begin("Debug Asset Manager");

		static std::vector<AssetManager::Asset> assets;
		static int selected = -1;
		if (ImGui::Button("Refresh Assets"))
		{
			assets = AssetManager::GetAllAssets();
			selected = -1;
		}

		// Search field used to filter the asset list by name.
		static char searchBuffer[128] = "";
		ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer));

		ImGui::Separator();
		ImGui::BeginChild("AssetList", ImVec2(0, 200), true);

		for (int i = 0; i < static_cast<int>(assets.size()); ++i)
		{
			if (searchBuffer[0] != '\0')
			{
				std::string nameLower = assets[i].name;
				std::string searchLower = searchBuffer;

				// Preserve the full byte range while lowercasing to avoid narrowing warnings.
				std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
					[](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });
				std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(),
					[](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

				if (nameLower.find(searchLower) == std::string::npos)
					continue;
			}

			const bool isSelected = (selected == i);
			if (ImGui::Selectable(assets[i].name.c_str(), isSelected))
				selected = i;
		}

		ImGui::EndChild();
		ImGui::Separator();

		if (selected >= 0 && selected < static_cast<int>(assets.size()))
		{
			const auto& asset = assets[selected];
			ImGui::Text("Path: %s", asset.path.string().c_str());
			ImGui::Text("Type: %d", static_cast<int>(asset.type));

			if (ImGui::Button("Load Asset"))
			{
				Resource_Manager::LoadAsset(asset.path);
			}

			ImGui::SameLine();
			if (ImGui::Button("Delete Asset"))
			{
				ImGui::OpenPopup("ConfirmDelete");
			}

			if (ImGui::BeginPopupModal("ConfirmDelete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Delete '%s'?", asset.name.c_str());
				ImGui::Text("This action cannot be undone.");
				ImGui::Separator();

				if (ImGui::Button("Delete", ImVec2(120, 0)))
				{
					const bool deleted = AssetManager::DeleteAsset(asset.path);
					if (deleted && assetBrowserPanel)
						assetBrowserPanel->ForceRefresh();
					if (jsonPanel)
						jsonPanel->RefreshFiles();

					assets = AssetManager::GetAllAssets();
					selected = -1;
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}

		ImGui::Separator();
		ImGui::TextDisabled("Only Prefabs (JSON) can be created via the editor.");
		ImGui::TextDisabled("Binary assets must be imported externally.");

		static char prefabName[128] = "";
		ImGui::InputText("Prefab Name", prefabName, sizeof(prefabName));

		const bool createObject = ImGui::Button("Create Object Prefab");
		ImGui::SameLine();
		const bool createEnemy = ImGui::Button("Create Enemy Prefab");

		if ((createObject || createEnemy) && prefabName[0] != '\0')
		{
			bool success = false;
			if (createObject)
			{
				success = AssetManager::CreateObjectAsset(prefabName, "json");
			}
			else if (createEnemy)
			{
				success = AssetManager::CreateEnemyAsset(prefabName, "json");
			}

			if (success)
			{
				prefabName[0] = '\0';
				assets = AssetManager::GetAllAssets();
				if (jsonPanel)
					jsonPanel->RefreshFiles();
			}
			else
			{
				ImGui::TextColored(
					ImVec4(1, 0, 0, 1),
					"Failed to create prefab. Name might exist or template missing."
				);
			}
		}

		ImGui::End();
	}
}
#endif
