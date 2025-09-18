// Sandbox/MyGame/Game.cpp
#include "../../Engine/Graphics/Window.hpp"  
#include "Game.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include "Input/Input.h"

using Clock = std::chrono::steady_clock;
using SecondsF = std::chrono::duration<float>;

namespace mygame {
    
    void run() 
    {
        initializeAudio();
        cleanupAudio();
        startAudio();
        //gfx::Window win(800, 600, "MyGame - Audio Demo");
        WindowConfig cfg = LoadWindowConfig("../../Data_Files/window.json");
        gfx::Window win(cfg.width, cfg.height, cfg.title.c_str());

        // Track key states to prevent multiple triggers
        static std::array<bool, 10>keysPressed = { false }, lastkeysPressed = {false};
        
        auto t_prev = Clock::now();
        while (!win.shouldClose()) {
            win.pollEvents();

            const auto t_now = Clock::now();
            const float dt = std::chrono::duration_cast<SecondsF>(t_now - t_prev).count();
            t_prev = t_now;
            handleAudioInput(win, keysPressed);
            
            // ---- update(dt) ----
            (void)dt;
            win.beginFrame();
            // ---- render() ----
            win.endFrame();
            win.swapBuffers();
            lastkeysPressed = keysPressed;
        }
        
         cleanupAudio();
        std::cout << "Game ended." << std::endl;
    }



} // namespace mygame
