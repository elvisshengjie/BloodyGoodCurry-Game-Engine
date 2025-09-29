/*********************************************************************************************
 \file      AudioManager.h
 \par       SofaSpuds
 \author    jianwei.c(jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Declaration of the AudioManager class, which manages audio playback using the FMOD 
            audio library. This includes loading, playing, pausing, stopping, and unloading 
            sounds, as well as volume and pitch control.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore. 
            All rights reserved.
**********************************************************************************************/
#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "fmod.h"
// Forward declarations for FMOD
struct FMOD_SYSTEM;
struct FMOD_SOUND;
struct FMOD_CHANNEL;
/*********************************************************************************************
  \class AudioManager
  \brief Manages the initialization, loading, playback, and cleanup of audio using FMOD.
  
  The AudioManager provides functions to control sounds including volume, pitch, pausing,
  and stopping playback. It also tracks currently loaded sounds and manages channels 
  associated with them.
**********************************************************************************************/
class AudioManager 
{
    public:
    /*****************************************************************************************
      \brief Constructor for AudioManager.
    *****************************************************************************************/
    AudioManager();
    /*****************************************************************************************
      \brief Destructor for AudioManager. Ensures cleanup of FMOD resources.
    *****************************************************************************************/
    ~AudioManager();
    /*****************************************************************************************
      \brief Initializes the FMOD system for audio playback.
      \return True if initialization succeeds, false otherwise.
    *****************************************************************************************/
    bool initialize();
    /*****************************************************************************************
      \brief Shuts down the FMOD system and releases all associated resources.
    *****************************************************************************************/
    void shutdown();
    /*****************************************************************************************
    \brief Updates the FMOD system. 
           Should be called once per frame in the main game loop.
    *****************************************************************************************/
    void update();
    /*****************************************************************************************
      \brief Loads a sound from a given file path.
      \param name      Name to identify the sound.
      \param filePath  Path to the audio file.
      \param loop      Whether the sound should loop during playback.
      \return True if the sound was successfully loaded, false otherwise.
    *****************************************************************************************/
    bool loadSound(const std::string& name, const std::string& filePath, bool loop = false);
    /*****************************************************************************************
      \brief Unloads a specific sound by name.
      \param name  Identifier of the sound to unload.
    *****************************************************************************************/
    void unloadSound(const std::string& name);
    /*****************************************************************************************
      \brief Unloads all currently loaded sounds.
    *****************************************************************************************/
    void unloadAllSounds();
    /*****************************************************************************************
      \brief Plays a sound by name.
      \param name      Identifier of the sound to play.
      \param volume    Playback volume (default = 1.0f).
      \param pitch     Playback pitch (default = 1.0f).
      \return True if the sound started playing successfully, false otherwise.
    *****************************************************************************************/
    bool playSound(const std::string& name, float volume = 1.0f, float pitch = 1.0f);
    /*****************************************************************************************
      \brief Stops playback of a specific sound.
      \param name  Identifier of the sound to stop.
    *****************************************************************************************/
    void stopSound(const std::string& name);
    /*****************************************************************************************
      \brief Stops playback of all sounds currently playing.
    *****************************************************************************************/
    void stopAllSounds();
    /*****************************************************************************************
      \brief Pauses or unpauses a specific sound.
      \param name   Identifier of the sound to pause.
      \param pause  True to pause, false to resume.
    *****************************************************************************************/
    void pauseSound(const std::string& name, bool pause = true);
    /*****************************************************************************************
      \brief Pauses or unpauses all sounds.
      \param pause  True to pause, false to resume.
    *****************************************************************************************/
    void pauseAllSounds(bool pause = true);
    /*****************************************************************************************
      \brief Sets the master volume for all sounds.
      \param volume  New master volume level.
    *****************************************************************************************/
    void setMasterVolume(float volume);
    /*****************************************************************************************
      \brief Sets the volume of a specific sound.
      \param name    Identifier of the sound.
      \param volume  New volume level.
    *****************************************************************************************/
    void setSoundVolume(const std::string& name, float volume);
    /*****************************************************************************************
      \brief Sets the pitch of a specific sound.
      \param name   Identifier of the sound.
      \param pitch  New pitch value.
    *****************************************************************************************/
    void setSoundPitch(const std::string& name, float pitch);
    /*****************************************************************************************
      \brief Checks if a sound is currently playing.
      \param name  Identifier of the sound.
      \return True if the sound is playing, false otherwise.
    *****************************************************************************************/
    bool isSoundLoaded(const std::string& name) const;
    /*****************************************************************************************
      \brief Checks if a sound is currently playing.
      \param name  Identifier of the sound.
      \return True if the sound is playing, false otherwise.
    *****************************************************************************************/
    bool isSoundPlaying(const std::string& name) const;
    /*****************************************************************************************
      \brief Retrieves a list of all loaded sounds.
      \return Vector containing identifiers of loaded sounds.
    *****************************************************************************************/
    std::vector<std::string> getLoadedSounds() const;
    private:
    FMOD_SYSTEM* m_system;///Pointer to the FMOD system instance.
    std::unordered_map<std::string, FMOD_SOUND*> m_sounds;///Map of loaded sounds by name.
    std::unordered_map<std::string, std::vector<FMOD_CHANNEL*>> m_channels;///Map of channels for each sound.
    /*****************************************************************************************
      \brief Constructs the full file path for a given file.
      \param fileName  Name of the file.
      \return Full path string.
    *****************************************************************************************/
    std::string getFullPath(const std::string& fileName) const;
    /*****************************************************************************************
      \brief Checks the result of an FMOD operation and logs errors if any.
      \param result     FMOD operation result.
      \param operation  Description of the operation attempted.
    *****************************************************************************************/
    void checkFMODError(FMOD_RESULT result, const std::string& operation) const;
};