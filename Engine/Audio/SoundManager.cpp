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

SoundManager& SoundManager::getInstance()
{
    static SoundManager instance;
    return instance;
}

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

void SoundManager::shutdown()
{
    if (m_audioManager)
    {
        m_audioManager->shutdown();
        m_audioManager.reset();
        std::cout << "SoundManager shutdown complete" << std::endl;
    }
}

void SoundManager::update()
{
    if (m_audioManager)
    {
        m_audioManager->update();
    }
}

bool SoundManager::loadSound(const std::string& name, const std::string& filePath, bool loop)
{
    if (!m_audioManager)
    {
        std::cerr << "SoundManager not initialized" << std::endl;
        return false;
    }
    
    return m_audioManager->loadSound(name, filePath, loop);
}

void SoundManager::unloadSound(const std::string& name)
{
    if (m_audioManager)
    {
        m_audioManager->unloadSound(name);
    }
}

void SoundManager::unloadAllSounds()
{
    if (m_audioManager)
    {
        m_audioManager->unloadAllSounds();
    }
}

bool SoundManager::playSound(const std::string& name, float volume, float pitch)
{
    if (!m_audioManager)
    {
        std::cerr << "SoundManager not initialized" << std::endl;
        return false;
    }
    
    return m_audioManager->playSound(name, volume, pitch);
}

void SoundManager::stopSound(const std::string& name)
{
    if (m_audioManager)
    {
        m_audioManager->stopSound(name);
    }
}

void SoundManager::stopAllSounds()
{
    if (m_audioManager)
    {
        m_audioManager->stopAllSounds();
    }
}

void SoundManager::pauseSound(const std::string& name, bool pause)
{
    if (m_audioManager)
    {
        m_audioManager->pauseSound(name, pause);
    }
}

void SoundManager::pauseAllSounds(bool pause)
{
    if (m_audioManager)
    {
        m_audioManager->pauseAllSounds(pause);
    }
}

void SoundManager::setMasterVolume(float volume)
{
    if (m_audioManager)
    {
        m_audioManager->setMasterVolume(volume);
    }
}

void SoundManager::setSoundVolume(const std::string& name, float volume)
{
    if (m_audioManager)
    {
        m_audioManager->setSoundVolume(name, volume);
    }
}

void SoundManager::setSoundPitch(const std::string& name, float pitch)
{
    if (m_audioManager)
    {
        m_audioManager->setSoundPitch(name, pitch);
    }
}

bool SoundManager::isSoundLoaded(const std::string& name) const
{
    if (!m_audioManager)
    {
        return false;
    }
    
    return m_audioManager->isSoundLoaded(name);
}

bool SoundManager::isSoundPlaying(const std::string& name) const
{
    if (!m_audioManager)
    {
        return false;
    }
    
    return m_audioManager->isSoundPlaying(name);
}

std::vector<std::string> SoundManager::getLoadedSounds() const
{
    if (!m_audioManager)
    {
        return {};
    }
    
    return m_audioManager->getLoadedSounds();
}
