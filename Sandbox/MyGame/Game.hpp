#pragma once
#include "Graphics/Window.hpp"
#include "Managers/SoundManager.h"

namespace mygame {
    
    void init(gfx::Window& win);
    void update(float dt);
    void draw();
    void shutdown();

   
    void initializeAudio();
    void cleanupAudio();
}
