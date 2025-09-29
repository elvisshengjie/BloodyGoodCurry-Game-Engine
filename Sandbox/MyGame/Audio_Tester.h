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
    /*****************************************************************************************
      \brief Initializes the audio system and loads necessary resources for playback.
    *****************************************************************************************/
    void initializeAudio();
    /*****************************************************************************************
      \brief Cleans up the audio system, releasing all loaded sounds and resources.
    *****************************************************************************************/
    void cleanupAudio();
    /*****************************************************************************************
      \brief Starts audio playback and sets up any required channels or looping sounds.
      \param bus  Reference to the MessageBus for dispatching audio-related messages.
    *****************************************************************************************/
    void startAudio(MessageBus& bus);
    /*****************************************************************************************
     \brief Handles user input related to audio, such as key presses that trigger sounds.
     \param win          Reference to the game window for input polling.
     \param keysPressed  Array tracking the state of keys being pressed.
     \param bus          Reference to the MessageBus for dispatching audio-related messages.
    *****************************************************************************************/
    void handleAudioInput(gfx::Window& win, std::array<bool, 10>& keysPressed,MessageBus& bus);
}