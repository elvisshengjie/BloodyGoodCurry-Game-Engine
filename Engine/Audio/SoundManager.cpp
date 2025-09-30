/*********************************************************************************************
 \file      SoundManager.cpp
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Implementation of the SoundManager singleton class. This class provides a global 
            access point for managing audio in the game by delegating operations to the 
            AudioManager. It handles initialization, updates, cleanup, as well as loading, 
            playing, pausing, stopping, and unloading sounds.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#include "SoundManager.h"
#include <iostream>
/*****************************************************************************************
 \brief Get the singleton instance of the SoundManager.
 \return Reference to the single SoundManager instance.
*****************************************************************************************/
SoundManager& SoundManager::getInstance()
{
    static SoundManager instance;
    return instance;
}
/*****************************************************************************************
 \brief Initialize the underlying AudioManager system.
 \return True if initialization succeeded, false otherwise.
*****************************************************************************************/
bool SoundManager::initialize()
{
    if (!m_audioManager)
    {
        m_audioManager = std::make_unique<AudioManager>();
    }
    
    bool success = m_audioManager->initialize();
    if (success)
    {
        std::cout << "SoundManager initialized successfully" << std::endl;
    }
    else
    {
        std::cerr << "Failed to initialize SoundManager" << std::endl;
    }
    
    return success;
}
/*****************************************************************************************
 \brief Shutdown the AudioManager and release all resources.
*****************************************************************************************/
void SoundManager::shutdown()
{
    if (m_audioManager)
    {
        m_audioManager->shutdown();
        m_audioManager.reset();
        std::cout << "SoundManager shutdown complete" << std::endl;
    }
}
/*****************************************************************************************
 \brief Update the AudioManager. Should be called once per frame.
*****************************************************************************************/
void SoundManager::update()
{
    if (m_audioManager)
    {
        m_audioManager->update();
    }
}
/*****************************************************************************************
 \brief Load a sound into memory.
 \param name The identifier for the sound.
 \param filePath Path to the audio file.
 \param loop Whether the sound should loop when played.
 \return True if the sound was successfully loaded.
*****************************************************************************************/
bool SoundManager::loadSound(const std::string& name, const std::string& filePath, bool loop)
{
    if (!m_audioManager)
    {
        std::cerr << "SoundManager not initialized" << std::endl;
        return false;
    }
    
    return m_audioManager->loadSound(name, filePath, loop);
}
/*****************************************************************************************
 \brief Unload a specific sound by its identifier.
 \param name The identifier of the sound to unload.
*****************************************************************************************/
void SoundManager::unloadSound(const std::string& name)
{
    if (m_audioManager)
    {
        m_audioManager->unloadSound(name);
    }
}
/*****************************************************************************************
 \brief Play a loaded sound.
 \param name The identifier of the sound.
 \param volume Playback volume (default 1.0f).
 \param pitch Playback pitch (default 1.0f).
 \return True if the sound was successfully played.
*****************************************************************************************/
void SoundManager::unloadAllSounds()
{
    if (m_audioManager)
    {
        m_audioManager->unloadAllSounds();
    }
}
/*****************************************************************************************
 \brief Play a loaded sound.
 \param name The identifier of the sound.
 \param volume Playback volume (default 1.0f).
 \param pitch Playback pitch (default 1.0f).
 \return True if the sound was successfully played.
*****************************************************************************************/
bool SoundManager::playSound(const std::string& name, float volume, float pitch)
{
    if (!m_audioManager)
    {
        std::cerr << "SoundManager not initialized" << std::endl;
        return false;
    }
    
    return m_audioManager->playSound(name, volume, pitch);
}
/*****************************************************************************************
 \brief Stop playback of all instances of a specific sound.
 \param name The identifier of the sound to stop.
*****************************************************************************************/
void SoundManager::stopSound(const std::string& name)
{
    if (m_audioManager)
    {
        m_audioManager->stopSound(name);
    }
}
/*****************************************************************************************
 \brief Stop all currently playing sounds.
*****************************************************************************************/
void SoundManager::stopAllSounds()
{
    if (m_audioManager)
    {
        m_audioManager->stopAllSounds();
    }
}
/*****************************************************************************************
 \brief Pause or resume a specific sound.
 \param name The identifier of the sound.
 \param pause True to pause, false to resume.
*****************************************************************************************/
void SoundManager::pauseSound(const std::string& name, bool pause)
{
    if (m_audioManager)
    {
        m_audioManager->pauseSound(name, pause);
    }
}
/*****************************************************************************************
 \brief Pause or resume all currently playing sounds.
 \param pause True to pause, false to resume.
*****************************************************************************************/
void SoundManager::pauseAllSounds(bool pause)
{
    if (m_audioManager)
    {
        m_audioManager->pauseAllSounds(pause);
    }
}
/*****************************************************************************************
 \brief Set the global master volume.
 \param volume The new master volume level.
*****************************************************************************************/
void SoundManager::setMasterVolume(float volume)
{
    if (m_audioManager)
    {
        m_audioManager->setMasterVolume(volume);
    }
}
/*****************************************************************************************
 \brief Set the volume for all active instances of a specific sound.
 \param name The identifier of the sound.
 \param volume The new volume level.
*****************************************************************************************/
void SoundManager::setSoundVolume(const std::string& name, float volume)
{
    if (m_audioManager)
    {
        m_audioManager->setSoundVolume(name, volume);
    }
}
/*****************************************************************************************
 \brief Set the pitch for all active instances of a specific sound.
 \param name The identifier of the sound.
 \param pitch The new pitch level.
*****************************************************************************************/
void SoundManager::setSoundPitch(const std::string& name, float pitch)
{
    if (m_audioManager)
    {
        m_audioManager->setSoundPitch(name, pitch);
    }
}
/*****************************************************************************************
 \brief Check whether a sound is currently loaded.
 \param name The identifier of the sound.
 \return True if the sound is loaded, false otherwise.
*****************************************************************************************/
bool SoundManager::isSoundLoaded(const std::string& name) const
{
    if (!m_audioManager)
    {
        return false;
    }
    
    return m_audioManager->isSoundLoaded(name);
}
/*****************************************************************************************
 \brief Check whether a sound is currently playing.
 \param name The identifier of the sound.
 \return True if the sound is playing, false otherwise.
*****************************************************************************************/
bool SoundManager::isSoundPlaying(const std::string& name) const
{
    if (!m_audioManager)
    {
        return false;
    }
    
    return m_audioManager->isSoundPlaying(name);
}
/*****************************************************************************************
 \brief Retrieve a list of all loaded sounds.
 \return A vector containing the identifiers of loaded sounds.
*****************************************************************************************/
std::vector<std::string> SoundManager::getLoadedSounds() const
{
    if (!m_audioManager)
    {
        return {};
    }
    
    return m_audioManager->getLoadedSounds();
}
