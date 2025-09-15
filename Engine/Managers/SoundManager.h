#pragma once
#include "AudioManager.h"
#include <memory>

class SoundManager 
{
public:
    // Singleton pattern
    static SoundManager& getInstance();
    
    // Initialize and cleanup
    bool initialize();
    void shutdown();
    void update(); // Call this every frame
    
    // Sound loading and management
    bool loadSound(const std::string& name, const std::string& filePath, bool loop = false);
    void unloadSound(const std::string& name);
    void unloadAllSounds();
    
    // Sound playback
    bool playSound(const std::string& name, float volume = 1.0f, float pitch = 1.0f);
    void stopSound(const std::string& name);
    void stopAllSounds();
    void pauseSound(const std::string& name, bool pause = true);
    void pauseAllSounds(bool pause = true);
    
    // Volume and pitch control
    void setMasterVolume(float volume);
    void setSoundVolume(const std::string& name, float volume);
    void setSoundPitch(const std::string& name, float pitch);
    
    // Utility functions
    bool isSoundLoaded(const std::string& name) const;
    bool isSoundPlaying(const std::string& name) const;
    std::vector<std::string> getLoadedSounds() const;

private:
    SoundManager() = default;
    ~SoundManager() = default;
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    
    std::unique_ptr<AudioManager> m_audioManager;
};
