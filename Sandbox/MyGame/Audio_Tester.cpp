/*********************************************************************************************
 \file      AudioTester.cpp
 \par       SofaSpuds
 \author    Ho Jun(h.jun@digipen.edu) - Primary Author, 50%
            jianwei.c (jianwei.c@digipen.edu) - Secondary Author, 50%

 \brief     Implementation of audio testing utilities. Provides functions to initialize, clean
            up, start playback, and handle input for audio in the game. Integrates with
            MessageBus and Window systems for interactive audio control.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "Audio_Tester.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
namespace mygame
{
    /*****************************************************************************************
      \brief Initializes the audio system and loads necessary resources for playback.
    *****************************************************************************************/
    void initializeAudio()
    {
        std::cout << "Initializing audio system..." << std::endl;

        // Ensure FMOD system is initialized
        if (!SoundManager::getInstance().initialize()) 
        {
            std::cerr << "Failed to initialize sound system!" << std::endl;
            return;
        }
        SoundManager::getInstance().setMasterVolume(0.7f);


        Resource_Manager::loadAll("../../assets/Audio");
        std::cout << "Audio system initialized successfully!" << std::endl;
    }
    /*****************************************************************************************
      \brief Cleans up the audio system, releasing all loaded sounds and resources.
    *****************************************************************************************/
    void cleanupAudio() 
    {
        std::cout << "Cleaning up audio system..." << std::endl;
        Resource_Manager::unloadAll(Resource_Manager::Sound); // unload only sounds
    }
    /*****************************************************************************************
      \brief Starts audio playback and sets up any required channels or looping sounds.
      \param bus  Reference to the MessageBus for dispatching audio-related messages.
    *****************************************************************************************/
    void startAudio(MessageBus& bus)
    {
    std::cout << "Starting MyGame with Sound Support..." << std::endl;
    bus.subscribe(KEY_1, [](){SoundManager::getInstance().playSound("badge", 0.8f);
    std::cout << "Playing coin sound!" << std::endl;});

    bus.subscribe(KEY_2, []()
    {
        auto& sm = SoundManager::getInstance();
        if (sm.isSoundPlaying("footsteps")) {
            sm.stopSound("footsteps");
            std::cout << "Stopped footsteps" << std::endl;
        } else {
            sm.playSound("footsteps", 0.6f);
            std::cout << "Started footsteps (looping)" << std::endl;
        }
    });

    bus.subscribe(KEY_3, []()
    { SoundManager::getInstance().playSound("level", 0.9f);
      std::cout << "Playing level win sound!" << std::endl;});

    bus.subscribe(KEY_4, []()
    {  SoundManager::getInstance().playSound("losing", 0.8f); 
        std::cout << "Playing losing horn!" << std::endl;});

    bus.subscribe(KEY_5, []()
    { SoundManager::getInstance().playSound("mouse", 0.7f);
      std::cout << "Playing mouse click!" << std::endl;});

    bus.subscribe(KEY_6, []()
    { SoundManager::getInstance().playSound("win", 0.9f);
      std::cout << "Playing win sound!" << std::endl;});

    bus.subscribe(KEY_M, [](){static float currentVolume = 0.7f;
        currentVolume = (currentVolume > 0.5f) ? 0.2f : 0.7f;
        SoundManager::getInstance().setMasterVolume(currentVolume);
        std::cout << "Master volume set to: " << currentVolume << std::endl;});

    bus.subscribe(KEY_S, [](){SoundManager::getInstance().stopAllSounds();
        std::cout << "Stopped all sounds!" << std::endl;});
    
    std::cout << "\n=== Audio Demo Controls ===" << std::endl;
    std::cout << "Press 1: Play coin sound" << std::endl;
    std::cout << "Press 2: Play footsteps " << std::endl;
    std::cout << "Press 3: Play level win sound" << std::endl;
    std::cout << "Press 4: Play losing horn" << std::endl;
    std::cout << "Press 5: Play mouse click" << std::endl;
    std::cout << "Press 6: Play win sound" << std::endl;
    std::cout << "Press M: Toggle master volume (0.2f / 0.7f)" << std::endl;
    std::cout << "Press S: Stop all sounds" << std::endl;
    std::cout << "Press ESC: Exit game" << std::endl;
    std::cout << "==========================" << std::endl;
    }
    /*****************************************************************************************
     \brief Handles user input related to audio, such as key presses that trigger sounds.
     \param win          Reference to the game window for input polling.
     \param keysPressed  Array tracking the state of keys being pressed.
     \param bus          Reference to the MessageBus for dispatching audio-related messages.
    *****************************************************************************************/
    void handleAudioInput(gfx::Window& win, std::array<bool, 10>& keysPressed,MessageBus& bus)
    {
        auto processKey=[&](int glfwKey, int index, MessageID msgID)
        {
            if (win.isKeyPressed(glfwKey) && !keysPressed[index]) 
            {bus.publish(Message(msgID));keysPressed[index] = true;} else if (!win.isKeyPressed(glfwKey)) 
            {keysPressed[index] = false;}
        };
        processKey(GLFW_KEY_1, 1, KEY_1);
        processKey(GLFW_KEY_2, 2, KEY_2);
        processKey(GLFW_KEY_3, 3, KEY_3);
        processKey(GLFW_KEY_4, 4, KEY_4);
        processKey(GLFW_KEY_5, 5, KEY_5);
        processKey(GLFW_KEY_6, 6, KEY_6);
        processKey(GLFW_KEY_M, 7, KEY_M);
        processKey(GLFW_KEY_S, 8, KEY_S);
    }
}

