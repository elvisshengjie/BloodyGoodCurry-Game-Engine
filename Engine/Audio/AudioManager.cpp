#include "AudioManager.h"
#include <iostream>
#include <filesystem>
#include "fmod.h"
#include "fmod_errors.h"


AudioManager::AudioManager() : m_system(nullptr) {}
AudioManager::~AudioManager() {shutdown();}

bool AudioManager::initialize()
{
    // Create FMOD system with correct version
    FMOD_RESULT result = FMOD_System_Create(&m_system, FMOD_VERSION);
    if (result != FMOD_OK) {
    std::cerr << "Failed to create FMOD system: " << FMOD_ErrorString(result) <<
    std::endl;
    return false;
    }
    // Initialize FMOD system
    result = FMOD_System_Init(m_system, 32, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
    std::cerr << "Failed to initialize FMOD system: " << FMOD_ErrorString(result) << std::endl;
    return false;
    }
    std::cout << "AudioManager initialized successfully" << std::endl;
    return true;
}

void AudioManager::shutdown() 
{
    if (m_system) 
    {
        stopAllSounds();       // stop active channels
        unloadAllSounds();     // release FMOD sounds
        // Close and release FMOD system
        FMOD_System_Close(m_system);
        FMOD_System_Release(m_system);

        m_system = nullptr;
        std::cout << "AudioManager shutdown complete" << std::endl;
    }
}

void AudioManager::update() 
{if (m_system) {FMOD_System_Update(m_system);}}

bool AudioManager::loadSound(const std::string& name, const std::string& filePath, bool loop) 
{
    if (!m_system) {std::cerr << "AudioManager not initialized" << std::endl; return false;}
    // Check if sound is already loaded
    if (m_sounds.find(name) != m_sounds.end()) { std::cout << "Sound '" << name << "' is already loaded" << std::endl; return true;}

    std::string fullPath = getFullPath(filePath);
    // Check if file exists
    if (!std::filesystem::exists(fullPath)) {std::cerr << "Audio file not found: " << fullPath << std::endl; return false;}
    FMOD_SOUND* sound = nullptr;
    FMOD_MODE mode = FMOD_DEFAULT;
    if (loop) {mode |= FMOD_LOOP_NORMAL;}
    FMOD_RESULT result = FMOD_System_CreateSound(m_system, fullPath.c_str(), mode, nullptr,&sound);
    if (result != FMOD_OK) {std::cerr << "Failed to load sound '" << name << "': " << FMOD_ErrorString(result)<< std::endl; return false;}
    
    m_sounds[name] = sound;
    
    std::cout << "Loaded sound: " << name << " from " << fullPath << std::endl;
    return true;
}

void AudioManager::unloadSound(const std::string& name)
{
    auto it = m_sounds.find(name);
    if (it!= m_sounds.end()) 
    { 
        FMOD_Sound_Release(it->second);
        m_sounds.erase(it);
        std::cout << "Unloaded sound: " << name << std::endl;
    }
    else {std::cerr << "Sound:"<<name << "is not found in Audio Manager\n";}
}

void AudioManager::unloadAllSounds()
{
    for (auto& [name,sound]: m_sounds)
    {
        FMOD_Sound_Release(sound);
        std::cout << "Unloaded sound: "<< name<< std::endl;
    }
    m_sounds.clear();
}

bool AudioManager::playSound(const std::string& name, float volume, float pitch)
{
    if (!m_system)
    {std::cerr<< "AudioManager not initalized"<<std::endl;return false;}
    auto it = m_sounds.find(name);
    if (it == m_sounds.end()) 
    {
        std::cerr << "Sound '" << name << "' not loaded" << std::endl;return false;}
        FMOD_CHANNEL* channel = nullptr;
        FMOD_RESULT result = FMOD_System_PlaySound(m_system, it->second, nullptr, false,
        &channel);
        if (result != FMOD_OK) {
        std::cerr << "Failed to play sound '" << name << "': " << FMOD_ErrorString(result)
        << std::endl;
        return false;
    }
    // Set volume and pitch
    FMOD_Channel_SetVolume(channel, volume);
    FMOD_Channel_SetPitch(channel, pitch);

    auto iterator = m_channels.find(name);
    
    // channel of the same name is playing.
    if (iterator != m_channels.end()) {
        FMOD_Channel_Stop(iterator->second);
        m_channels.erase(iterator);
    }

    // Store channel for later control
    m_channels[name] = channel;
    std::cout << "Playing sound: " << name << std::endl;
    return true;
}
void AudioManager::stopSound(const std::string& name)
{
    auto it = m_channels.find(name);
    if (it == m_channels.end()){std::cerr << "No active channel for sound '" << name << "'" << std::endl; return;}
    FMOD_RESULT result =FMOD_Channel_Stop(it->second);
    if (result == FMOD_OK)
    { std::cout << "Stopped sound: " << name << std::endl; m_channels.erase(it);}
    else 
    {std::cerr << "Failed to stop sound '" << name << "': " << FMOD_ErrorString(result) << std::endl;} 
}
void AudioManager::stopAllSounds()
{
    for (auto& [name, channel] : m_channels)
    {
        if (channel) {
            FMOD_BOOL isPlaying = 0;
            FMOD_RESULT result = FMOD_Channel_IsPlaying(channel, &isPlaying);

            if (result == FMOD_OK && isPlaying) {
                result = FMOD_Channel_Stop(channel);
                if (result == FMOD_OK) {
                    std::cout << "Stopped sound: " << name << std::endl;
                } else {
                    std::cerr << "Failed to stop sound '" << name
                              << "': " << FMOD_ErrorString(result) << std::endl;
                }
            }
        }
    }
    m_channels.clear();
}


void AudioManager::pauseSound(const std::string& name, bool pause)
{
    auto it = m_channels.find(name);
    if (it == m_channels.end()){std::cerr << "No active channel for sound '" << name << "'" << std::endl; return;}
    FMOD_RESULT result =FMOD_Channel_SetPaused(it->second,pause);
    if (result == FMOD_OK)
    { std::cout << (pause ? "Paused" : "Resumed")<<"sound: "<< name <<std::endl;}
    else 
    {std::cerr << "Failed to " << (pause ? "pause" : "resume")<< " sound '" << name << "': " << FMOD_ErrorString(result) << std::endl;} 
}


void AudioManager::pauseAllSounds(bool pause)
{
  for (auto& [name, channel] : m_channels)
  {FMOD_Channel_SetPaused(channel, pause);std::cout << (pause ? "Paused" : "Resumed") << " sound: " << name << std::endl;}
}

void AudioManager::setMasterVolume(float volume) 
{
    if (m_system) 
    {
        FMOD_CHANNELGROUP* masterGroup = nullptr;
        FMOD_RESULT result = FMOD_System_GetMasterChannelGroup(m_system, &masterGroup);
        if (result == FMOD_OK && masterGroup) {FMOD_ChannelGroup_SetVolume(masterGroup, volume);std::cout << "Set master volume to: " << volume << std::endl;} 
        else {std::cerr << "Failed to get master channel group: " << FMOD_ErrorString(result) << std::endl;}
    }
}

void AudioManager::setSoundVolume(const std::string& name, float volume)
{
    auto it = m_channels.find(name);
    if (it == m_channels.end()){ std::cerr << "No active channel for sound '" << name << "'" << std::endl; return;}

    FMOD_RESULT result = FMOD_Channel_SetVolume(it->second, volume);
    if (result == FMOD_OK){std::cout << "Set volume of '" << name << "' to " << volume << std::endl;}
    else{std::cerr << "Failed to set volume for '" << name << "': " << FMOD_ErrorString(result) << std::endl;}
}

void AudioManager::setSoundPitch(const std::string& name, float pitch)
{
    auto it = m_channels.find(name);
    if (it == m_channels.end()){ std::cerr << "No active channel for sound '" << name << "'" << std::endl; return;}
    FMOD_RESULT result = FMOD_Channel_SetPitch(it->second, pitch);
    if (result == FMOD_OK) {std::cout << "Set pitch of '" << name << "' to " << pitch << std::endl;}
    else{ std::cerr << "Failed to set pitch for '" << name << "': " << FMOD_ErrorString(result) << std::endl;}
}
bool AudioManager::isSoundLoaded(const std::string& name) const
{return m_sounds.find(name) != m_sounds.end();}

bool AudioManager::isSoundPlaying(const std::string& name) const
{
    auto it = m_channels.find(name);
    if (it == m_channels.end() || it->second == nullptr)
        return false;
    FMOD_BOOL playing = false;
    FMOD_RESULT result = FMOD_Channel_IsPlaying(it->second, &playing);
    if (result != FMOD_OK) {
        std::cerr << "Failed to check if sound '" << name << "' is playing: "
            << FMOD_ErrorString(result) << std::endl;
        return false;
    }
    return playing != 0;
}

std::vector<std::string> AudioManager::getLoadedSounds() const
{
    std::vector<std::string> sounds;
    for (const auto& [name, sound] : m_sounds) {
        sounds.push_back(name);
    }
    return sounds;
}
std::string AudioManager::getFullPath(const std::string& fileName) const 
{
    // Try to find the audio file in the game-assets directory
    std::filesystem::path currentPath = std::filesystem::current_path();
    // Try different possible paths
    std::vector<std::filesystem::path> possiblePaths = {
    currentPath / "game-assests" / "audio" / "sfx" / fileName,
    currentPath / ".." / "game-assests" / "audio" / "sfx" / fileName,
    currentPath / ".." / ".." / "game-assests" / "audio" / "sfx" / fileName,
    currentPath / ".." / ".." / ".." / "game-assests" / "audio" / "sfx" / fileName
    };
    for (const auto& path: possiblePaths)
    {if (std::filesystem::exists(path)){return path.string();}}
    
    // If no path found, return the original filename
    return fileName;
}

void AudioManager::checkFMODError(FMOD_RESULT result, const std::string& operation) const
{
    if (result != FMOD_OK) {
        std::cerr << "FMOD Error during '" << operation << "': " << FMOD_ErrorString(result) << " (code " << result << ")" << std::endl;
    }
}

