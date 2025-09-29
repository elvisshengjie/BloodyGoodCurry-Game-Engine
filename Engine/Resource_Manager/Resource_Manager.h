#pragma once
#include "../Audio/AudioManager.h"
#include "../Audio/SoundManager.h"
#include "../Graphics/Graphics.hpp"
#include <string>
#include <filesystem>
#include <algorithm>
#include <iostream>
class Resource_Manager
{
public:
    enum Resource_Type { Texture, Font, Graphics, Sound, All };
    struct Resources { std::string id; Resource_Type type; unsigned int handle;};
    static bool load(const std::string& name, const std::string& path, bool loop = false);
    static void loadAll(const std::string& directory);
    static void unloadAll(Resource_Type type); 
    static inline std::unordered_map<std::string, Resources> resources_map;
    
    // Helper Functions
    static inline std::string GetExtension(const std::string& path);
    static bool isTexture(const std::string& ext);
    static bool isSound(const std::string& ext);
};
