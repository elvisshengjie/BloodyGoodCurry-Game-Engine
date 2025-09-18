#include "WindowConfig.h"
WindowConfig LoadWindowConfig(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {throw std::runtime_error("Could not open " + filename);}
    nlohmann::json j;
    file >> j;  
    WindowConfig WConfig;
    if (!j.contains("window")) {throw std::runtime_error("JSON does not contain 'window' object");}
    WConfig.width  = j["window"]["width"].get<int>();
    WConfig.height = j["window"]["height"].get<int>();
    WConfig.title  = j["window"]["title"].get<std::string>();
    return WConfig;
}