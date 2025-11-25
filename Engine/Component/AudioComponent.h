#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include "Audio/SoundManager.h"
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
namespace Framework
{
    struct SoundInfo
    {
        std::string id;
        bool loop{ false };
    };

    class AudioComponent : public GameComponent
    {
        public:
        std::unordered_map<std::string, SoundInfo> sounds;
        std::unordered_map<std::string, bool> playing;
        float volume{ 1.0f }; 
        std::string entityType;

        AudioComponent() = default;
        
        void initialize() override
        {
            sounds.clear();
            playing.clear();


            if (entityType == "player")
            {
                sounds["footsteps"] = { "footsteps", true };
                sounds["Slash1"] = { "Slash1", false };
                sounds["GrappleShoot1"] = { "GrappleShoot1", false };
            }
            else if (entityType == "enemy")
            {
                sounds["GhostSounds"] = { "GhostSounds", false };
            }

            // Build playing map
            for (auto& [action, info] : sounds)
                playing[action] = false;
        }
    
        void Play(const std::string& action)
        {
            auto it = sounds.find(action);
            if (it != sounds.end() && SoundManager::getInstance().isSoundLoaded(it->second.id))
            {SoundManager::getInstance().playSound(it->second.id, volume, 1.0f, it->second.loop); playing[action] = true;}
        }
        
        void Stop(const std::string& action)
        {
            auto it = sounds.find(action);
            if (it != sounds.end())
            {
                SoundManager::getInstance().stopSound(it->second.id);
                playing[action] = false;
            }
        }

        void TriggerSound(const std::string& action)
        {
            auto it = sounds.find(action);
            if (it != sounds.end()) { SoundManager::getInstance().playSound(it->second.id, volume, 1.f, it->second.loop);}
        }

        void Serialize(ISerializer& s) override
        {
            if (s.HasKey("entityType"))
                StreamRead(s, "entityType", entityType);

            if (s.EnterObject("sounds"))
            {
                for (auto& [action, info] : sounds)
                {
                    if (s.EnterObject(action))
                    {
                        StreamRead(s, "id", info.id);
                        int loopInt = info.loop ? 1 : 0;
                        StreamRead(s, "loop", loopInt);
                        info.loop = (loopInt != 0);
                        s.ExitObject();
                    }
                }
                s.ExitObject();
            }

            if (s.HasKey("volume"))
                StreamRead(s, "volume", volume);
        }


        std::unique_ptr<GameComponent> Clone() const override
        {
            auto copy = std::make_unique<AudioComponent>();
            copy->sounds = sounds;
            copy->volume = volume;
            copy->playing = playing;
            return copy;
        }

        void Update(float dt) 
        {(void)dt;}
        ~AudioComponent() override 
        {
            for (auto& [action, isPlaying] : playing)
            {
                if (isPlaying)
                {
                    auto it = sounds.find(action);
                    if (it != sounds.end())
                    {
                        SoundManager::getInstance().stopSound(it->second.id);
                    }
                }
            }
        }
    };
}