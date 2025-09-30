/*********************************************************************************************
 \file      AudioTester.h
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Declaration of utility functions for testing and handling audio in the game.
            Includes functions to initialize and clean up audio systems, start audio playback,
            and handle audio-related input events. Integrates with the MessageBus and Window
            systems for interactive audio testing.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
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