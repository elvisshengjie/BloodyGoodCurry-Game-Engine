#include "Asset_Manager.h"

std::filesystem::path AssetManager::ProjectRoot()
{
	static std::filesystem::path cachedRoot; // Cache it so we only search once
	static bool initialized = false;
	if (initialized)
		return cachedRoot;
	std::filesystem::path path = std::filesystem::current_path();
	while (true)
	{
		// Skip if we're in a build directory
		if (path.string().find("\\build\\") != std::string::npos ||
			path.string().find("/build/") != std::string::npos)
		{
			path = path.parent_path();
			continue;
		}

		if (std::filesystem::exists(path / "assets") &&
			std::filesystem::exists(path / "Data_Files"))
		{
			// Found the root - change working directory to it
			std::filesystem::current_path(path);
			cachedRoot = path;
			initialized = true;
			return path;
		}
		if (path == path.root_path()) // Reached filesystem root
			break;
		path = path.parent_path(); // Move up one directory
	}
	// If not found, throw or fallback
	throw std::runtime_error("Engine root not found! Make sure 'assets' and 'Data_Files' exist.");
}

bool AssetManager::IsValidAssetFile(const std::filesystem::path& path)
{
	if (!path.has_extension())
		return false;

	std::string ext = path.extension().string();
	if (!ext.empty() && ext[0] == '.')
		ext.erase(0, 1);

	// Allowed extensions ONLY
	static const std::unordered_set<std::string> allowedExtensions = {
		"png", "jpg", "jpeg",
		"wav", "mp3",
		"ttf", "otf",
		"vert", "frag",
		"json"
	};

	if (!allowedExtensions.contains(ext))
		return false;

	// Optional: block known internal/debug files
	static const std::unordered_set<std::string> blockedNames = {
		"error", "log", "debug"
	};

	if (blockedNames.contains(path.stem().string()))
		return false;

	return true;
}


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
	std::filesystem::path target = ProjectRoot() / "assets" / sourceFile.filename();
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
		basePath = ProjectRoot() / "Data_Files";
	else if (extension == "png")
		basePath = ProjectRoot() / "assets/Textures";
	else if (extension == "ttf" || extension == "otf")
		basePath = ProjectRoot() / "assets/Textures";
	else if (extension == "wav" || extension == "mp3")
		basePath = ProjectRoot() / "assets/Audio"; 
	else 
		basePath = ProjectRoot() / "assets/Others";
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
		file.write(reinterpret_cast<const char*>(pngData), sizeof(pngData));
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
		file.write(reinterpret_cast<const char*>(wavData), sizeof(wavData));
	}

	if (extension == "vert" || extension == "frag")
	{file << "// Shader: " << name << "\n";}
	file.close();
	return true;
}

bool AssetManager::CreatePrefab(const std::string& prefabName)
{
	std::filesystem::path prefabPath = ProjectRoot() / "Data_Files" / (prefabName + ".json");
	if (std::filesystem::exists(prefabPath)) return false;
	std::ofstream file(prefabPath);
	return file.is_open();
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
	std::filesystem::path assetsRoot = ProjectRoot() / "assets";
	if (std::filesystem::exists(assetsRoot) && std::filesystem::is_directory(assetsRoot))
	{
		for (auto& p : std::filesystem::recursive_directory_iterator(assetsRoot))
		{
			if (!p.is_regular_file())
				continue;
			if (!IsValidAssetFile(p.path()))
				continue;
			Asset a;
			a.path = p.path();
			a.name = p.path().stem().string();
			a.type = IdentifyAssetType(p.path());
			if (a.type == AssetType::Unknown)
				continue; // extra safety
			allAssets.push_back(a);
		}
	}

	// Scan Data_Files for prefabs and animations
	std::filesystem::path dataRoot = ProjectRoot() / "Data_Files";
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