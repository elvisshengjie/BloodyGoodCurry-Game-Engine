#include "Resource_Manager.h"
//Helper Functions
inline std::string Resource_Manager::GetExtension(const std::string& path)
{
    std::string ext = std::filesystem::path(path).extension().string();
    if (!ext.empty() && ext[0]=='.'){ext.erase(0,1);}
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}
bool Resource_Manager::isTexture(const std::string& ext){return (ext == "png"||ext == "jpg");}
bool Resource_Manager::isSound(const std::string& ext){return ext == "mp3";}


namespace fs = std::filesystem;

bool Resource_Manager::load(const std::string& id, const std::string& path, bool loop)
{
    fs::path filePath(path);
    if (!fs::exists(filePath) || !fs::is_regular_file(filePath))
    {std::cerr << "[Resource_Manager] File not found: " << path << std::endl; return false;}
    std::string ext = GetExtension(path);
    if (isTexture(ext))
    {  
        unsigned int texID = gfx::Graphics::loadTexture(path.c_str());
        if (texID != 0){ resources_map[id] = { id, Resource_Type::Graphics, texID };; return true;}
        return false;
    }
    else if (isSound(ext))
    {  
        bool success = SoundManager::getInstance().loadSound(id,path,loop); 
        if (success){resources_map[id] = { id, Resource_Type::Sound ,0};}
        return success;
    }
    else {std::cerr << "[Resource_Manager] Unsupported file type: "  << path << std::endl; return false;}
}

void Resource_Manager::loadAll(const std::string& directory)
{
    for (auto& entry : fs::directory_iterator(directory)) 
    {
        if (!entry.is_regular_file()) continue;
        fs::path path = entry.path();
        std::string ext = GetExtension(path.string());
        std::string name = path.stem().string(); // filename without extension
        std::string stem = path.stem().string();
        size_t pos = stem.find_first_of("-_.");
        std::string id = (pos == std::string::npos) ? stem : stem.substr(0, pos);
        bool loop = (stem.find("loop") != std::string::npos);
        if (isTexture(ext) || isSound(ext)) 
        {
            if (load(id, path.string(),loop)) 
            {
                std::cout << "[Resource_Manager] Loaded: " << id
                    << " (" << path.string() << ")"
                    << (loop && isSound(ext) ? " [looping]" : "")
                    << std::endl;
            }
        }
    }
}

void Resource_Manager::unloadAll(Resource_Type type)
{
    std::cout << "[Resource_Manager] Unloading resources of type: "
        << (type == Resource_Type::All ? "All"
            : (type == Resource_Type::Sound ? "Sound" : "Graphics"))
        << std::endl;

    // If type is Sound or All, ensure SoundManager unloads all sounds first
    if (type == Resource_Type::Sound) {
        std::cout << "[Resource_Manager] Stopping and unloading all sounds..." << std::endl;
        auto& audio = SoundManager::getInstance();
        SoundManager::getInstance().shutdown();
        std::cout << "[Resource_Manager] All sounds unloaded." << std::endl;
    }

    // If type is Graphics or All, call Graphics cleanup
    if (type == Resource_Type::Graphics) {
        std::cout << "[Resource_Manager] Cleaning up graphics..." << std::endl;
        gfx::Graphics::cleanup();
        std::cout << "[Resource_Manager] Graphics cleanup complete." << std::endl;
    }

    // Iterate resources map and remove entries of the requested type
    for (auto it = resources_map.begin(); it != resources_map.end(); ) 
    {
        Resources res = it->second;
        bool matchType = (type == Resource_Type::All || res.type == type);
        if (matchType) {
            std::cout << "[Resource_Manager] Removing resource: " << it->first << std::endl;
            it = resources_map.erase(it); // erase and move forward
        }
        else {++it;}
    }
    std::cout << "[Resource_Manager] UnloadAll finished." << std::endl;
}





    

