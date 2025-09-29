/*********************************************************************************************
 \file      SoundManager.h
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Declaration of the SoundManager singleton class, which acts as a wrapper around
            the AudioManager to provide global sound control for the application. The 
            SoundManager ensures only one instance of the AudioManager is used and simplifies
            access to sound loading, playback, pausing, stopping, and cleanup.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
**********************************************************************************************/
#pragma once
#include "AudioManager.h"
#include <memory>
/*********************************************************************************************
 \class SoundManager
 \brief Singleton wrapper around AudioManager for centralized sound management.

 The SoundManager ensures only one instance of AudioManager exists. It provides an easy-to-
 access global interface for loading, playing, pausing, stopping, and unloading sounds, as
 well as controlling volume and pitch. It also forwards utility functions to check sound 
 states and retrieve loaded sounds.
**********************************************************************************************/
class SoundManager 
{
public:
    /*****************************************************************************************
     \brief Get the singleton instance of the SoundManager.
     \return Reference to the single SoundManager instance.
    *****************************************************************************************/
    static SoundManager& getInstance();
    /*****************************************************************************************
     \brief Initialize the underlying AudioManager system.
     \return True if initialization succeeded, false otherwise.
    *****************************************************************************************/
    bool initialize();
    /*****************************************************************************************
     \brief Shutdown the AudioManager and release all resources.
    *****************************************************************************************/
    void shutdown();
    /*****************************************************************************************
     \brief Update the AudioManager. Should be called once per frame.
    *****************************************************************************************/
    void update();
    /*****************************************************************************************
     \brief Load a sound into memory.
     \param name The identifier for the sound.
     \param filePath Path to the audio file.
     \param loop Whether the sound should loop when played.
     \return True if the sound was successfully loaded.
    *****************************************************************************************/
    bool loadSound(const std::string& name, const std::string& filePath, bool loop = false);
    /*****************************************************************************************
     \brief Unload a specific sound by its identifier.
     \param name The identifier of the sound to unload.
    *****************************************************************************************/
    void unloadSound(const std::string& name);
    /*****************************************************************************************
     \brief Unload all currently loaded sounds.
    *****************************************************************************************/
    void unloadAllSounds();
    /*****************************************************************************************
     \brief Play a loaded sound.
     \param name The identifier of the sound.
     \param volume Playback volume (default 1.0f).
     \param pitch Playback pitch (default 1.0f).
     \return True if the sound was successfully played.
    *****************************************************************************************/
    bool playSound(const std::string& name, float volume = 1.0f, float pitch = 1.0f);
    /*****************************************************************************************
     \brief Stop playback of all instances of a specific sound.
     \param name The identifier of the sound to stop.
    *****************************************************************************************/
    void stopSound(const std::string& name);
    /*****************************************************************************************
     \brief Stop all currently playing sounds.
    *****************************************************************************************/
    void stopAllSounds();
    /*****************************************************************************************
     \brief Pause or resume a specific sound.
     \param name The identifier of the sound.
     \param pause True to pause, false to resume.
    *****************************************************************************************/
    void pauseSound(const std::string& name, bool pause = true);
    /*****************************************************************************************
     \brief Pause or resume all currently playing sounds.
     \param pause True to pause, false to resume.
    *****************************************************************************************/
    void pauseAllSounds(bool pause = true);
    /*****************************************************************************************
     \brief Set the global master volume.
     \param volume The new master volume level.
    *****************************************************************************************/
    void setMasterVolume(float volume);
    /*****************************************************************************************
     \brief Set the volume for all active instances of a specific sound.
     \param name The identifier of the sound.
     \param volume The new volume level.
    *****************************************************************************************/
    void setSoundVolume(const std::string& name, float volume);
    /*****************************************************************************************
     \brief Set the pitch for all active instances of a specific sound.
     \param name The identifier of the sound.
     \param pitch The new pitch level.
    *****************************************************************************************/
    void setSoundPitch(const std::string& name, float pitch);
    /*****************************************************************************************
     \brief Check whether a sound is currently loaded.
     \param name The identifier of the sound.
     \return True if the sound is loaded, false otherwise.
    *****************************************************************************************/
    bool isSoundLoaded(const std::string& name) const;
    /*****************************************************************************************
     \brief Check whether a sound is currently playing.
     \param name The identifier of the sound.
     \return True if the sound is playing, false otherwise.
    *****************************************************************************************/
    bool isSoundPlaying(const std::string& name) const;
    /*****************************************************************************************
     \brief Retrieve a list of all loaded sounds.
     \return A vector containing the identifiers of loaded sounds.
    *****************************************************************************************/
    std::vector<std::string> getLoadedSounds() const;

private:
    /*****************************************************************************************
     \brief Private constructor for singleton pattern.
    *****************************************************************************************/
    SoundManager() = default;
    /*****************************************************************************************
     \brief Private destructor for singleton pattern.
    *****************************************************************************************/
    ~SoundManager() = default;
    // Deleted copy operations to enforce singleton
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    /// Underlying AudioManager instance.
    std::unique_ptr<AudioManager> m_audioManager;
};
