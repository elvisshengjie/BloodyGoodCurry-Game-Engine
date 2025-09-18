#pragma once
#include "Graphics/Window.hpp"


namespace mygame {
    
    void init(gfx::Window& win);
    void update(float dt);
    void draw();
    void shutdown();

   
    void initializeAudio();
    void cleanupAudio();
}
