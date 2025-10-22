#include "audioSystem.h"
#include <iostream>


namespace Framework {
    AudioSystem::AudioSystem(gfx::Window& window) :window(&window) {}

    void AudioSystem::Initialize()
    {
        AudioImGui::Initialize(*window);

     
    }

    void AudioSystem::Update(float dt){}
    void AudioSystem::draw() { AudioImGui::Render(); };

    void AudioSystem::Shutdown()
    {
        AudioImGui::Shutdown();
    }
};