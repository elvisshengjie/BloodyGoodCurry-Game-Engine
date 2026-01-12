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

	if (extension == "png")
	{
		// Minimal 1x1 transparent PNG
		const unsigned char pngData[] = {
			0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
			0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
			0x00, 0x00, 0x00, 0x01, // width = 1
			0x00, 0x00, 0x00, 0x01, // height = 1
			0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
			0x89, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41,
			0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
			0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,
			0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
			0x42, 0x60, 0x82
		};
		std::ofstream file(path, std::ios::binary);
		if (!file.is_open()) return false;
		file.write((const char*)pngData, sizeof(pngData));
		file.close();
		return true;
	}
	
	if (extension == "wav")
	{
		// 44-byte header + 2 bytes of silence for 1 sample (mono, 16-bit, 44100 Hz)
		const unsigned char wavData[46] = {
			'R','I','F','F', 0x2A,0x00,0x00,0x00, 'W','A','V','E',
			'f','m','t',' ', 0x10,0x00,0x00,0x00, 0x01,0x00,
			0x01,0x00, 0x44,0xAC,0x00,0x00, 0x88,0x58,0x01,0x00,
			0x02,0x00, 0x10,0x00, 'd','a','t','a', 0x02,0x00,0x00,0x00,
			0x00,0x00
		};
		std::ofstream file(path, std::ios::binary);
		if (!file.is_open()) return false;
		file.write((const char*)wavData, sizeof(wavData));
		file.close();
		return true;
	}

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