/*********************************************************************************************
 \file      AudioManagerStub.cpp
 \brief     No-audio fallback implementation used when SOFASPUDS_DISABLE_AUDIO is enabled.
*********************************************************************************************/

#include "AudioManager.h"

#if !SOFASPUDS_DISABLE_AUDIO && !defined(__INTELLISENSE__)
#error "AudioManagerStub.cpp should only be compiled when SOFASPUDS_DISABLE_AUDIO=1"
#endif

struct AudioManager::Impl
{
};

AudioManager::AudioManager() : pImpl(new Impl())
{
}

AudioManager::~AudioManager()
{
    delete pImpl;
    pImpl = nullptr;
}

bool AudioManager::initialize()
{
    return true;
}

void AudioManager::shutdown()
{
}

void AudioManager::update(float dt)
{
    (void)dt;
}

bool AudioManager::loadSound(const std::string& name, const std::string& filePath, bool loop, bool is3D)
{
    (void)name;
    (void)filePath;
    (void)loop;
    (void)is3D;
    return true;
}

void AudioManager::setListenerPosition(const void* pos, const void* forward, const void* up)
{
    (void)pos;
    (void)forward;
    (void)up;
}

void AudioManager::setSoundPosition(const std::string& name, const void* pos, const void* vel)
{
    (void)name;
    (void)pos;
    (void)vel;
}

void AudioManager::unloadSound(const std::string& name)
{
    (void)name;
}

void AudioManager::unloadAllSounds()
{
}

bool AudioManager::playSound(const std::string& name, float volume, float pitch, bool loop)
{
    (void)name;
    (void)volume;
    (void)pitch;
    (void)loop;
    return false;
}

void AudioManager::stopSound(const std::string& name)
{
    (void)name;
}

void AudioManager::stopAllSounds()
{
}

void AudioManager::pauseSound(const std::string& name, bool pause)
{
    (void)name;
    (void)pause;
}

void AudioManager::pauseAllSounds(bool pause)
{
    (void)pause;
}

void AudioManager::setMasterVolume(float volume)
{
    (void)volume;
}

void AudioManager::setSoundVolume(const std::string& name, float volume)
{
    (void)name;
    (void)volume;
}

void AudioManager::setSoundPitch(const std::string& name, float pitch)
{
    (void)name;
    (void)pitch;
}

void AudioManager::setSoundLoop(const std::string& name, bool loop)
{
    (void)name;
    (void)loop;
}

bool AudioManager::isSoundLoaded(const std::string& name) const
{
    (void)name;
    return false;
}

bool AudioManager::isSoundPlaying(const std::string& name) const
{
    (void)name;
    return false;
}

void AudioManager::fadeInSound(const std::string& name, float duration, float targetVolume)
{
    (void)name;
    (void)duration;
    (void)targetVolume;
}

void AudioManager::fadeOutSound(const std::string& name, float duration)
{
    (void)name;
    (void)duration;
}

AudioManager::ChannelID AudioManager::playSoundChannel(
    const std::string& name,
    float volume,
    float pitch,
    bool loop,
    const FMOD_VECTOR* pos,
    const FMOD_VECTOR* vel)
{
    (void)name;
    (void)volume;
    (void)pitch;
    (void)loop;
    (void)pos;
    (void)vel;
    return 0;
}

void AudioManager::setChannel3DPosition(ChannelID id, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel)
{
    (void)id;
    (void)pos;
    (void)vel;
}

bool AudioManager::isChannelPlaying(ChannelID id)
{
    (void)id;
    return false;
}

std::vector<std::string> AudioManager::getLoadedSounds() const
{
    return {};
}

std::string AudioManager::getFullPath(const std::string& fileName) const
{
    return fileName;
}

void AudioManager::updateFades(float deltaTime)
{
    (void)deltaTime;
}

void AudioManager::checkFMODError(FMOD_RESULT result, const std::string& operation) const
{
    (void)result;
    (void)operation;
}

void AudioManager::pruneStoppedChannels()
{
}
