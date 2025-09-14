#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <iostream>
#include "Image.h"
class ImagesManager
{
    public:
        ImagesManager() : m_id(0) {}
        ~ImagesManager() { release(); }
        bool createFromImage(const Image& image); // upload image to GPU
        void bind(unsigned int slot = 0) const;
        void release();

    private:
        GLuint m_id;
};