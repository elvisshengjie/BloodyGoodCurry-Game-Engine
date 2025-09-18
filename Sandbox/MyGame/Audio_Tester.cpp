#include "Audio_Tester.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace mygame
{
    void initializeAudio() 
    {
        std::cout << "Initializing audio system..." << std::endl;
        // Initialize the sound manager
        if (!SoundManager::getInstance().initialize()) {std::cerr << "Failed to initialize sound system!" << std::endl; return;}
        // Set master volume
        SoundManager::getInstance().setMasterVolume(0.7f);
        // Load all the audio files
        std::cout << "Loading audio files..." << std::endl;
        // Load coin/win sound
        if (SoundManager::getInstance().loadSound("coin", "badge-coin-win-14675.mp3", false)) {std::cout << "Loaded: badge-coin-win-14675.mp3 as 'coin'" << std::endl;}
        // Load footsteps (looping)
        if (SoundManager::getInstance().loadSound("footsteps", "footsteps-male.mp3", true)) {std::cout << "Loaded: footsteps-male.mp3 as 'footsteps' (looping)" << std::endl;}
        // Load level win sound
        if (SoundManager::getInstance().loadSound("level_win", "level-win.mp3", false)) {std::cout << "Loaded: level-win.mp3 as 'level_win'" << std::endl;}
        // Load losing horn
        if (SoundManager::getInstance().loadSound("lose", "losing-horn.mp3", false)) {std::cout << "Loaded: losing-horn.mp3 as 'lose'" << std::endl;}
        // Load mouse click
        if (SoundManager::getInstance().loadSound("click", "mouse-click.mp3", false)) { std::cout << "Loaded: mouse-click.mp3 as 'click'" << std::endl;}
        // Load win sound
        if (SoundManager::getInstance().loadSound("win", "win.mp3", false)) {std::cout << "Loaded: win.mp3 as 'win'" << std::endl;}
        std::cout << "Audio system initialized successfully!" << std::endl;
    }
    void cleanupAudio() {std::cout << "Cleaning up audio system..." << std::endl;SoundManager::getInstance().shutdown();}
    
    void startAudio()
    {
        std::cout << "Starting MyGame with Sound Support..." << std::endl;
        // Initialize audio
        initializeAudio();
        std::cout << "\n=== Audio Demo Controls ===" << std::endl;
        std::cout << "Press 1: Play coin sound" << std::endl;
        std::cout << "Press 2: Play/toggle footsteps (looping)" << std::endl;
        std::cout << "Press 3: Play level win sound" << std::endl;
        std::cout << "Press 4: Play losing horn" << std::endl;
        std::cout << "Press 5: Play mouse click" << std::endl;
        std::cout << "Press 6: Play win sound" << std::endl;
        std::cout << "Press M: Toggle master volume (0.2f / 0.7f)" << std::endl;
        std::cout << "Press S: Stop all sounds" << std::endl;
        std::cout << "Press ESC: Exit game" << std::endl;
        std::cout << "==========================" << std::endl;
    }

    void handleAudioInput(gfx::Window& win, std::array<bool, 10>& keysPressed)
    {
        if (win.isKeyPressed(GLFW_KEY_1) && !keysPressed[1]) {
            SoundManager::getInstance().playSound("coin", 0.8f);
            std::cout << "Playing coin sound!" << std::endl;
            keysPressed[1] = true;
        } else if (!win.isKeyPressed(GLFW_KEY_1)) {
            keysPressed[1] = false;
        }

        if (win.isKeyPressed(GLFW_KEY_2) && !keysPressed[2]) {
            if (SoundManager::getInstance().isSoundPlaying("footsteps")) {
                SoundManager::getInstance().stopSound("footsteps");
                std::cout << "Stopped footsteps" << std::endl;
            } else {
                SoundManager::getInstance().playSound("footsteps", 0.6f);
                std::cout << "Started footsteps (looping)" << std::endl;
            }
            keysPressed[2] = true;
        } else if (!win.isKeyPressed(GLFW_KEY_2)) {
            keysPressed[2] = false;
        }

        if (win.isKeyPressed(GLFW_KEY_3) && !keysPressed[3]) {
            SoundManager::getInstance().playSound("level_win", 0.9f);
            std::cout << "Playing level win sound!" << std::endl;
            keysPressed[3] = true;
        } else if (!win.isKeyPressed(GLFW_KEY_3)) {
            keysPressed[3] = false;
        }

        if (win.isKeyPressed(GLFW_KEY_4) && !keysPressed[4]) {
            SoundManager::getInstance().playSound("lose", 0.8f);
            std::cout << "Playing losing horn!" << std::endl;
            keysPressed[4] = true;
        } else if (!win.isKeyPressed(GLFW_KEY_4)) {
            keysPressed[4] = false;
        }

        if (win.isKeyPressed(GLFW_KEY_5) && !keysPressed[5]) {
            SoundManager::getInstance().playSound("click", 0.7f);
            std::cout << "Playing mouse click!" << std::endl;
            keysPressed[5] = true;
        } else if (!win.isKeyPressed(GLFW_KEY_5)) {
            keysPressed[5] = false;
        }

        if (win.isKeyPressed(GLFW_KEY_6) && !keysPressed[6]) {
            SoundManager::getInstance().playSound("win", 0.9f);
            std::cout << "Playing win sound!" << std::endl;
            keysPressed[6] = true;
        } else if (!win.isKeyPressed(GLFW_KEY_6)) {
            keysPressed[6] = false;
        }

        if (win.isKeyPressed(GLFW_KEY_M) && !keysPressed[7]) {
            static float currentVolume = 0.7f;
            currentVolume = (currentVolume > 0.5f) ? 0.2f : 0.7f;
            SoundManager::getInstance().setMasterVolume(currentVolume);
            std::cout << "Master volume set to: " << currentVolume << std::endl;
            keysPressed[7] = true;
        } else if (!win.isKeyPressed(GLFW_KEY_M)) {
            keysPressed[7] = false;
        }

        if (win.isKeyPressed(GLFW_KEY_S) && !keysPressed[8]) {
            SoundManager::getInstance().stopAllSounds();
            std::cout << "Stopped all sounds!" << std::endl;
            keysPressed[8] = true;
        } else if (!win.isKeyPressed(GLFW_KEY_S)) {
            keysPressed[8] = false;
        }
    }
}

