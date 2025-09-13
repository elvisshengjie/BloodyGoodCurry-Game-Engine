#include "stb_image.h"
#include <string>
#include <iostream>
struct Image {
    int width = 0, height = 0, channels = 0;
    unsigned char* data = nullptr;
    Image(const std::string& path);
    ~Image();
};