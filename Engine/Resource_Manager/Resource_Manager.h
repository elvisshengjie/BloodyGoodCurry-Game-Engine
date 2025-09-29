/*********************************************************************************************
 \file      Resource_Manager.h
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Declaration of the Resource_Manager class, which provides a centralized system 
            for loading, tracking, and unloading game resources. Supports textures, fonts, 
            graphics, and sounds. Also includes utility functions for file extension checks 
            and type validation.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
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
    /// Enum describing supported resource types
    enum Resource_Type { Texture, Font, Graphics, Sound, All };
    /// Structure representing a loaded resource
    struct Resources 
    { 
        std::string id{}; ///Unique identifier for the resource
        Resource_Type type{ Resource_Type::All }; /// Type of the resource
        unsigned int handle{};  ///Handle or pointer to the actual resource
    };
    /*****************************************************************************************
      \brief Load a single resource by name and path.
      \param name  Unique identifier for the resource.
      \param path  Path to the resource file.
      \param loop  Optional flag for sound looping (default false).
      \return true if the resource was successfully loaded, false otherwise.
    *****************************************************************************************/
    static bool load(const std::string& name, const std::string& path, bool loop = false);
    /*****************************************************************************************
      \brief Load all resources from a specified directory.
      \param directory  Path to the directory containing resource files.
    *****************************************************************************************/
    static void loadAll(const std::string& directory);
    /*****************************************************************************************
      \brief Unload all resources of a specified type.
      \param type  Resource type to unload (Texture, Font, Graphics, Sound, or All).
    *****************************************************************************************/
    static void unloadAll(Resource_Type type); 
    /// Map storing all loaded resources with unique identifiers
    static inline std::unordered_map<std::string, Resources> resources_map;
    
    // Helper Functions

    /*****************************************************************************************
      \brief Get the file extension from a path string.
      \param path  Path to the file.
      \return File extension string (e.g., "png", "wav").
    *****************************************************************************************/
    static inline std::string GetExtension(const std::string& path);
    /*****************************************************************************************
      \brief Check if a given file extension corresponds to a texture type.
      \param ext  File extension string.
      \return true if it is a texture, false otherwise.
    *****************************************************************************************/
    static bool isTexture(const std::string& ext);
    /*****************************************************************************************
      \brief Check if a given file extension corresponds to a sound type.
      \param ext  File extension string.
      \return true if it is a sound, false otherwise.
    *****************************************************************************************/
    static bool isSound(const std::string& ext);
    /*****************************************************************************************
      \brief Retrieve the handle of a texture resource by its unique key.
      \param key  Resource identifier.
      \return Handle of the texture, or 0 if not found.
    *****************************************************************************************/
    static unsigned int getTexture(const std::string& key);
};
