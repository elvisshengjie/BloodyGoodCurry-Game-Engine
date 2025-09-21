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
    
    void Resource_Manager::unloadAll()
    {
        for (auto it = resources_map.begin(); it != resources_map.end(); ) {
            Resources res = it->second;

            if (res.type == Resource_Type::Graphics) {
                glDeleteTextures(1, &res.id);
            }
            else if (res.type == Resource_Type::Sound) {
                SoundManager::getInstance().unloadSound(it->first);
            }

            it = resources_map.erase(it); // erase and move to next
        }
    }
        
    
 