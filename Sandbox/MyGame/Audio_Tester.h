#pragma once
#include "Graphics/Window.hpp"
#include "Managers/SoundManager.h"
#include <iostream>
#include "Config/WindowConfig.h"
#include <chrono>
#include <array>

namespace mygame
{
    void initializeAudio();
    void cleanupAudio();
    void startAudio();
    void handleAudioInput(gfx::Window& win, std::array<bool, 10> keysPressed);
}