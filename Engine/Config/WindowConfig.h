#pragma once

#include <iostream>
#include <fstream>
#pragma warning(push)           // Save current warning state
#pragma warning(disable:26819) 
#include "../ThirdParty/json_dep/json.hpp"
#pragma warning(pop) 


struct WindowConfig
{
    int width;
    int height;
    std::string title;
};
WindowConfig LoadWindowConfig(const std::string& filename);
