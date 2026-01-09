#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include "Resource_Manager.h"
class AssetManager
{
	public:
		enum class AssetType
		{Texture, SpriteSheet, Audio, Font, Shader, Prefab, Unknown};
		struct Asset
		{std::filesystem::path path; AssetType type; std::string name;};
		static bool ImportAsset(const std::filesystem::path& sourceFile);
		static bool DeleteAsset(const std::filesystem::path& assetPath);
		static AssetType IdentifyAssetType(const std::filesystem::path& assetPath);
		static bool CreatePrefab(const std::string& prefabName);
		static bool DeletePrefab(const std::string& prefabName);
		static const std::vector<Asset>& GetAllAssets();


};