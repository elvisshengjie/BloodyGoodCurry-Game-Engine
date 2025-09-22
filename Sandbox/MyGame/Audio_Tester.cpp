#include "Audio_Tester.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace mygame
{
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

        std::cout << "Loading audio files via Resource_Manager..." << std::endl;

        // List of audio files and resource names
        struct AudioEntry { std::string name; std::string path; bool loop; };
        std::vector<AudioEntry> audioFiles = {
            {"coin", "badge-coin-win-14675.mp3", false},
            {"footsteps", "footsteps-male.mp3", true},
            {"level_win", "level-win.mp3", false},
            {"lose", "losing-horn.mp3", false},
            {"click", "mouse-click.mp3", false},
            {"win", "win.mp3", false}
        };

        for (auto& entry : audioFiles)
        {
            if (Resource_Manager::load(entry.name, entry.path, entry.loop))
                std::cout << "Loaded: " << entry.path << " as '" << entry.name << "'" << (entry.loop ? " (looping)" : "") << std::endl;
            else
                std::cerr << "Failed to load: " << entry.path << std::endl;
        }

        std::cout << "Audio system initialized successfully!" << std::endl;
    }
    
    void cleanupAudio() 
    {
        std::cout << "Cleaning up audio system..." << std::endl;
        Resource_Manager::unloadAll(Resource_Manager::Sound); // unload only sounds
    }
    
    void startAudio(MessageBus& bus)
    {
    std::cout << "Starting MyGame with Sound Support..." << std::endl;
    initializeAudio();

    bus.subscribe(KEY_1, [](){SoundManager::getInstance().playSound("coin", 0.8f);
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
    { SoundManager::getInstance().playSound("level_win", 0.9f);
      std::cout << "Playing level win sound!" << std::endl;});

    bus.subscribe(KEY_4, []()
    {  SoundManager::getInstance().playSound("lose", 0.8f); 
        std::cout << "Playing losing horn!" << std::endl;});

    bus.subscribe(KEY_5, []()
    { SoundManager::getInstance().playSound("click", 0.7f);
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

