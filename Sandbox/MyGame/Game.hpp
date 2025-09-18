#pragma once
#include "Graphics/Window.hpp"
#include "Managers/SoundManager.h"
#include "Config/WindowConfig.h"

// All game-facing functions/structs live in this namespace.
namespace mygame {

    // A tiny 2D transform that we will control at runtime with the keyboard.
    // rotation is in radians; scale is uniform (if you need non-uniform, split into sx/sy).
    struct Transform2D
    {
        float posX{ 400.0f };   // default on-screen position (you can change this)
        float posY{ 300.0f };
        float rotationRad{ 0.0f }; // rotation around the sprite's center, in radians
        float scale{ 1.0f };       // uniform scale (clamped in update)
    };

    // ---- Lifecycle you will call from your game loop ----
    // Call once at the start of the program (e.g., inside run()).
    void init();

    // Call every frame with delta time in seconds (from your game loop).
    // This will read keyboard input and update rotation/scale of your object.
    void update(float dt);

    // Call every frame after update(). Draw using the current Transform2D.
    // (Implementation will use your renderer; pivot should be the sprite's center.)
    void draw();

    // ---- Audio helpers you already had ----
    void initializeAudio();
    void cleanupAudio();

    // ---- Entry point managed elsewhere (you already call this) ----
    void run();
}
