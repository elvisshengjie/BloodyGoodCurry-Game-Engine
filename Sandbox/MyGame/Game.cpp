// Sandbox/MyGame/Game.cpp
#include "../../Engine/Graphics/Window.hpp"  
#include "Game.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <Graphics/Graphics.hpp>

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

        // Initialize Graphics
        gfx::Graphics::initialize();
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
            gfx::Graphics::renderBackground();
            gfx::Graphics::renderRectangle();
            gfx::Graphics::renderCircle();

            win.endFrame();
            win.swapBuffers();
            lastkeysPressed = keysPressed;
        }
        gfx::Graphics::cleanup();
        cleanupAudio();
        std::cout << "Game ended." << std::endl;
    }



} // namespace mygame
