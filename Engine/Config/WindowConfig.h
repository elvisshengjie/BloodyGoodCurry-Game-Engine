#include <iostream>
#include <fstream>
#include "../ThirdParty/json_dep/json.hpp"


struct WindowConfig
{
    int width;
    int height;
    std::string title;
};
WindowConfig LoadWindowConfig(const std::string& filename);
