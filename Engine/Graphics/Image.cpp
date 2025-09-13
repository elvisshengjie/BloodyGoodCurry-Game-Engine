#include "Image.h"

Image::Image(const std::string& path) {
 data = stbi_load(path.c_str(), &width, &height, &channels, 0);
 if (!data) {std::cerr << "Failed to load image: " << path << std::endl;}
}

Image::~Image() {if (data) stbi_image_free(data);}