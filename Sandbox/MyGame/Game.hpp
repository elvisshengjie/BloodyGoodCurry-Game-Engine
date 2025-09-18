#pragma once
#include "Graphics/Window.hpp"
#include "Managers/SoundManager.h"
#include <iostream>
#include "Config/WindowConfig.h"
namespace mygame {
    void run();
    void initializeAudio();
    void cleanupAudio();
}
