#pragma once
#include "Graphics/Window.hpp"
#include "Audio/SoundManager.h"
#include <iostream>
#include "Config/WindowConfig.h"
#include "Messaging_System/Message.hpp"
#include "Messaging_System/Messager_Bus.hpp"
#include "Resource_Manager/Resource_Manager.h"
#include <chrono>
#include <array>

namespace mygame
{
    void initializeAudio();
    void cleanupAudio();
    void startAudio(MessageBus& bus);
    void handleAudioInput(gfx::Window& win, std::array<bool, 10>& keysPressed,MessageBus& bus);
}