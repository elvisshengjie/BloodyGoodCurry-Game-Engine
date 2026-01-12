#include "Asset_Manager.h"
AssetManager::AssetType AssetManager::IdentifyAssetType(const std::filesystem::path& assetPath)
{
	std::string ext = assetPath.extension().string();
	if (!ext.empty() && ext[0] == '.')
		ext.erase(0, 1);
	std::string stem = assetPath.stem().string();
	if (Resource_Manager::isTexture(ext))
	{
		std::filesystem::path animMeta =
			assetPath.parent_path() /
			(stem + ".anim.json");
		if (std::filesystem::exists(animMeta))
			return AssetType::SpriteSheet;
		return AssetType::Texture;
	}
	if (Resource_Manager::isSound(ext)) return AssetType::Audio;
	if (ext == "ttf" || ext == "otf") return AssetType::Font;
	if (ext == "vert" || ext == "frag") return AssetType::Shader;
	if (ext == "json")
	{
		if (assetPath.string().find("Data_Files") != std::string::npos) 
			return AssetType::Prefab;
		else
			return AssetType::Json;
	}
	return AssetType::Unknown;
}
bool AssetManager::ImportAsset(const std::filesystem::path& sourceFile) 
{
	std::filesystem::path target = std::filesystem::path("assets") / sourceFile.filename();
	if (std::filesystem::exists(target)) return false;
	std::filesystem::copy_file(sourceFile, target, std::filesystem::copy_options::overwrite_existing);
	return true;
}
bool AssetManager::DeleteAsset(const std::filesystem::path& assetPath) 
{ 
	if (!std::filesystem::exists(assetPath)) return false;
	std::string id = assetPath.stem().string();
	Resource_Manager::Unload(id);
	std::filesystem::remove(assetPath);
	return true;
}
bool AssetManager::CreateEmptyAsset(const std::string& name,const std::string& extension)
{
	std::filesystem::path basePath;
	if (extension == "json")
		basePath = "Data_Files";
	else if (extension == "png")
		basePath = "assets/Textures";
	else if (extension == "ttf" || extension == "otf")
		basePath = "assets/Fonts";
	else if (extension == "wav" || extension == "mp3")
		basePath = "assets/Audio";
	else 
		basePath= "assets/Others";
	std::filesystem::create_directories(basePath);
	std::filesystem::path path = basePath / (name + "." + extension);
	if (std::filesystem::exists(path))
		return false;
	std::ofstream file(path);
	if (!file.is_open())return false;
	if (extension == "vert" || extension == "frag")
	{file << "// Shader: " << name << "\n";}
	file.close();
	return true;
}

bool AssetManager::CreatePrefab(const std::string& prefabName)
{
	std::filesystem::path prefabPath =
		std::filesystem::path("Data_Files") / (prefabName + ".json");
	if (std::filesystem::exists(prefabPath))
		return false;
	std::ofstream file(prefabPath);
	if (!file.is_open())
		return false;
	return true;
}
bool AssetManager::DeletePrefab(const std::string& prefabName)
{
	std::filesystem::path prefabPath =
		std::filesystem::path("Data_Files") / (prefabName + ".json");
	if (!std::filesystem::exists(prefabPath))
		return false;
	std::filesystem::remove(prefabPath);
	return true;
}
const std::vector<AssetManager::Asset>& AssetManager::GetAllAssets()
{
	static std::vector<Asset> allAssets;
	allAssets.clear();
	std::filesystem::path assetsRoot("assets");
	if (std::filesystem::exists(assetsRoot) && std::filesystem::is_directory(assetsRoot))
	{
		for (auto& p : std::filesystem::recursive_directory_iterator(assetsRoot))
		{
			if (p.is_regular_file())
			{
				Asset a;
				a.path = p.path();
				a.name = p.path().stem().string();
				a.type = IdentifyAssetType(p.path());
				allAssets.push_back(a);
			}
		}
	}

	// Scan Data_Files for prefabs and animations
	std::filesystem::path dataRoot("Data_Files");
	if (std::filesystem::exists(dataRoot) && std::filesystem::is_directory(dataRoot))
	{
		for (auto& p : std::filesystem::recursive_directory_iterator(dataRoot))
		{
			if (!p.is_regular_file()) continue;
			Asset a;
			a.path = p.path();
			a.name = p.path().stem().string();
			a.type = IdentifyAssetType(p.path());
			allAssets.push_back(a);
		}
	}

	return allAssets;
}