#include "Resource_Manager.h"
   //Helper Functions
    inline std::string Resource_Manager::GetExtentsion(const std::string& path)
    {
        size_t pos = path.find_last_of('.');
        if (pos == std::string::npos){return "";}
        std::string ext = path.substr(pos +1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }
    
    bool Resource_Manager::isGraphics(const std::string& ext)
     {return (ext == "png"||ext == "jpg");}

    bool Resource_Manager::isSound(const std::string& ext)
    {return ext == "mp3";}
  

    bool Resource_Manager::load(const std::string& name, const std::string& path, bool loop)
    {
        std::string ext = GetExtentsion(path);
        if (isGraphics(ext))
        {  
           unsigned int texID = gfx::Graphics::loadTexture(path.c_str());
           if (texID != 0){resources_map[name] = { texID, Resource_Type::Graphics }; return true;}
           return false;
        }
        else if (isSound(ext))
        {  
           bool success = SoundManager::getInstance().loadSound(name,path,loop); 
           if (success){resources_map[name] = { 0, Resource_Type::Sound };}
           return success;
        }
        else {std::cerr << "Unsupported file type: " << path << std::endl; return false;}
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
        for (auto it = resources_map.begin(); it != resources_map.end(); ) {
            Resources res = it->second;

            bool matchType = (type == Resource_Type::All || res.type == type);

            if (matchType) {
                std::cout << "[Resource_Manager] Removing resource: " << it->first << std::endl;
                it = resources_map.erase(it); // erase and move forward
            }
            else {
                ++it; // skip, not the type we want
            }
        }

        std::cout << "[Resource_Manager] UnloadAll finished." << std::endl;
    }





        
    
 