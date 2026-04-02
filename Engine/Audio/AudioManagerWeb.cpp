/*********************************************************************************************
 \file      AudioManagerWeb.cpp
 \par       SofaSpuds
 \author    OpenAI Codex - Documentation pass, 100%
 \brief     Implements the browser-backed AudioManager used by Emscripten/web builds.
            Replaces the native FMOD runtime with a JavaScript/HTML audio bridge while
            preserving the same engine-facing AudioManager API used by the rest of the codebase.
 \details   Responsibilities:
            - Initialize and tear down a shared browser-side audio state object.
            - Load packaged audio assets from Emscripten's virtual filesystem.
            - Play, pause, stop, unload, and query sounds through HTMLAudioElement instances.
            - Preserve existing SoundManager / AudioManager call patterns so gameplay code
              does not need separate desktop vs web branches.
            - Track lightweight per-sound/channel state in C++ for fades, volume changes,
              and ChannelID-based bookkeeping used by existing game systems.
            - Gracefully handle browser autoplay restrictions by retrying blocked playback
              after the user's next input event.
            Current limitations:
            - Web playback is non-spatial for now; 3D listener/source updates are accepted
              but treated as no-ops so the public API stays platform-consistent.
            - FMOD-specific features are intentionally not used in this backend.
 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "AudioManager.h"

#if !defined(__EMSCRIPTEN__) && !defined(__INTELLISENSE__)
#error "AudioManagerWeb.cpp should only be compiled for Emscripten builds"
#endif

#include "Core/PathUtils.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(__INTELLISENSE__) && !defined(__EMSCRIPTEN__)
// Let Visual Studio browse this file on non-web targets without trying to parse
// Emscripten's JavaScript bridge bodies.
#define EM_JS(ret, name, params, ...) ret name params
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace
{
    /*************************************************************************************
      \brief Clamp a floating-point volume to the valid browser audio range.

      \param value  Requested volume value.
      \return Volume clamped into the inclusive range [0.0f, 1.0f].

      Used by the web backend before forwarding volume values into JavaScript so the
      browser always receives a safe gain value even if gameplay/UI code overshoots.
    *************************************************************************************/
    float ClampVolume(float value)
    {
        return std::clamp(value, 0.0f, 1.0f);
    }

    /*************************************************************************************
      \brief Normalize a pitch/playback-rate value for browser audio playback.

      \param value  Requested playback-rate multiplier.
      \return The requested value when positive, otherwise 1.0f.

      HTML audio expects a positive playback rate. This mirrors the defensive behavior
      of the native backend by falling back to normal speed when the input is invalid.
    *************************************************************************************/
    float NormalizePitch(float value)
    {
        return value > 0.0f ? value : 1.0f;
    }
}

// Each EM_JS block exposes a tiny bridge from C++ into browser-side audio control.
// We keep the browser state in one shared JS object so the C++ AudioManager can stay
// close to the FMOD-facing shape used elsewhere in the engine.
/*************************************************************************************
  \brief Initialize the shared browser-side audio state used by the web backend.

  \return Non-zero on success, zero on failure.

  Steps:
  - Reuse an existing `window.SofaSpudsWebAudio` state object if one already exists.
  - Create per-sound and per-channel maps used by later JS bridge calls.
  - Define helper functions for volume updates, cleanup, stop/unload, and autoplay retry.
  - Register lightweight input listeners so blocked autoplay can resume after user input.
  - Publish the state object on `window` for all subsequent bridge calls.
*************************************************************************************/
EM_JS(int, SofaWebAudio_Init, (), {
    if (typeof window === 'undefined') return 0;
    if (window.SofaSpudsWebAudio) return 1;

    var state = {
        sounds: new Map(),
        channels: new Map(),
        masterVolume: 1.0,
        pausedAll: false,
        unlockEvents: ['pointerdown', 'keydown', 'touchstart', 'mousedown']
    };

    state.clamp01 = function (value) {
        value = Number(value);
        if (!isFinite(value)) return 0;
        if (value < 0) return 0;
        if (value > 1) return 1;
        return value;
    };

    state.guessMime = function (path) {
        path = String(path || "").toLowerCase();
        if (path.endsWith('.mp3')) return 'audio/mpeg';
        if (path.endsWith('.wav')) return 'audio/wav';
        return 'application/octet-stream';
    };

    state.cleanupPlayer = function (player) {
        if (!player || player.__sofaDestroyed) return;

        player.__sofaDestroyed = true;
        player.__sofaWaitingForGesture = false;

        if (player.__sofaEndedHandler) {
            player.removeEventListener('ended', player.__sofaEndedHandler);
            player.__sofaEndedHandler = null;
        }
        if (player.__sofaErrorHandler) {
            player.removeEventListener('error', player.__sofaErrorHandler);
            player.__sofaErrorHandler = null;
        }

        var entry = state.sounds.get(player.__sofaSoundName);
        if (entry) entry.players.delete(player);

        if (player.__sofaChannelId) {
            state.channels.delete(player.__sofaChannelId);
        }
    };

    state.updatePlayerVolume = function (player) {
        if (!player || player.__sofaDestroyed) return;
        var baseVolume = state.clamp01(player.__sofaVolume);
        player.volume = state.clamp01(baseVolume * state.masterVolume);
    };

    state.tryPlayPlayer = function (player) {
        if (!player || player.__sofaDestroyed) return;
        if (state.pausedAll || player.__sofaPausedBySound) return;

        player.__sofaWaitingForGesture = false;
        try {
            var promise = player.play();
            if (promise && typeof promise.catch === 'function') {
                promise.catch(function () {
                    // Browsers may reject autoplay until the user interacts once.
                    // We remember blocked players and retry them on the next click/key.
                    if (!player.__sofaDestroyed && !state.pausedAll && !player.__sofaPausedBySound) {
                        player.__sofaWaitingForGesture = true;
                    }
                });
            }
        } catch (error) {
            if (!player.__sofaDestroyed) {
                player.__sofaWaitingForGesture = true;
            }
        }
    };

    state.flushBlockedPlayers = function () {
        state.sounds.forEach(function (entry) {
            entry.players.forEach(function (player) {
                if (!player.__sofaDestroyed &&
                    player.__sofaWaitingForGesture &&
                    !state.pausedAll &&
                    !player.__sofaPausedBySound) {
                    state.tryPlayPlayer(player);
                }
            });
        });
    };

    state.stopPlayer = function (player) {
        if (!player || player.__sofaDestroyed) return;
        try { player.pause(); } catch (error) {}
        try { player.currentTime = 0; } catch (error) {}
        state.cleanupPlayer(player);
    };

    state.stopSound = function (name) {
        var entry = state.sounds.get(name);
        if (!entry) return;

        Array.from(entry.players).forEach(function (player) {
            state.stopPlayer(player);
        });
    };

    state.unloadSound = function (name) {
        var entry = state.sounds.get(name);
        if (!entry) return;

        state.stopSound(name);

        if (entry.url) {
            try { URL.revokeObjectURL(entry.url); } catch (error) {}
        }

        state.sounds.delete(name);
    };

    state.createPlayer = function (entry, name, channelId, volume, pitch, loop) {
        var player = new Audio(entry.url);
        player.preload = 'auto';
        player.loop = !!loop;
        player.playbackRate = pitch > 0 ? pitch : 1.0;
        player.__sofaSoundName = name;
        player.__sofaChannelId = channelId | 0;
        player.__sofaVolume = state.clamp01(volume);
        player.__sofaPausedBySound = false;
        player.__sofaDestroyed = false;
        player.__sofaWaitingForGesture = false;

        player.__sofaEndedHandler = function () {
            state.cleanupPlayer(player);
        };
        player.__sofaErrorHandler = function () {
            state.cleanupPlayer(player);
        };

        player.addEventListener('ended', player.__sofaEndedHandler);
        player.addEventListener('error', player.__sofaErrorHandler);

        state.updatePlayerVolume(player);
        entry.players.add(player);

        if (player.__sofaChannelId) {
            state.channels.set(player.__sofaChannelId, player);
        }

        return player;
    };

    state.unlockHandler = function () {
        state.flushBlockedPlayers();
    };

    state.unlockEvents.forEach(function (eventName) {
        window.addEventListener(eventName, state.unlockHandler, { passive: true });
    });

    window.SofaSpudsWebAudio = state;
    return 1;
});

/*************************************************************************************
  \brief Tear down the shared browser-side audio state used by the web backend.

  Steps:
  - Stop every active browser audio player.
  - Unload each tracked sound and revoke its object URL.
  - Remove autoplay-unlock event listeners from the browser window.
  - Delete the shared state object so later initialization can start cleanly.
*************************************************************************************/
EM_JS(void, SofaWebAudio_Shutdown, (), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return;

    state.sounds.forEach(function (entry) {
        Array.from(entry.players).forEach(function (player) {
            state.stopPlayer(player);
        });
    });
    state.channels.clear();

    Array.from(state.sounds.keys()).forEach(function (name) {
        state.unloadSound(name);
    });

    if (state.unlockHandler) {
        state.unlockEvents.forEach(function (eventName) {
            window.removeEventListener(eventName, state.unlockHandler);
        });
    }

    delete window.SofaSpudsWebAudio;
});

/*************************************************************************************
  \brief Load one sound from Emscripten's virtual filesystem into browser playback state.

  \param namePtr  UTF-8 pointer to the logical sound id.
  \param pathPtr  UTF-8 pointer to the packaged filesystem path.
  \param loop     Non-zero when the sound's default loop state should be enabled.
  \return Non-zero on success, zero on failure.

  Steps:
  - Resolve the requested name/path from UTF-8 pointers.
  - Remove any older browser-side copy of the same sound id.
  - Read the packaged bytes from the Emscripten FS.
  - Wrap the bytes in a Blob and create an object URL for HTML audio playback.
  - Store the loaded sound metadata in the shared JS state map.
*************************************************************************************/
EM_JS(int, SofaWebAudio_LoadSound, (const char* namePtr, const char* pathPtr, int loop), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return 0;

    var name = UTF8ToString(namePtr);
    var path = UTF8ToString(pathPtr);

    try {
        state.unloadSound(name);

        var bytes = FS.readFile(path, { encoding: 'binary' });
        // Audio files live in Emscripten's virtual FS, so we wrap the raw bytes in a Blob
        // and hand the browser an object URL that HTMLAudioElement can stream from.
        var blob = new Blob([bytes], { type: state.guessMime(path) });
        var url = URL.createObjectURL(blob);

        state.sounds.set(name, {
            name: name,
            path: path,
            url: url,
            loopDefault: !!loop,
            players: new Set()
        });

        return 1;
    } catch (error) {
        console.error('[SofaSpuds][WebAudio] Failed to load sound', name, path, error);
        return 0;
    }
});

/*************************************************************************************
  \brief Unload one sound from the browser-side audio state.

  \param namePtr  UTF-8 pointer to the logical sound id.

  Stops active players for the sound, revokes its object URL, and removes the sound's
  metadata from the shared JS map.
*************************************************************************************/
EM_JS(void, SofaWebAudio_UnloadSound, (const char* namePtr), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return;
    state.unloadSound(UTF8ToString(namePtr));
});

/*************************************************************************************
  \brief Play one sound instance through the browser audio backend.

  \param namePtr    UTF-8 pointer to the logical sound id.
  \param channelId  Optional tracked ChannelID (0 for ordinary one-shot playback).
  \param volume     Requested playback volume.
  \param pitch      Requested playback rate.
  \param loop       Non-zero to loop the new player.
  \return Non-zero on success, zero on failure.

  Steps:
  - Find the loaded sound metadata in the shared JS state.
  - Create a new HTMLAudioElement backed by the sound's object URL.
  - Apply loop, volume, pitch, and bookkeeping metadata.
  - Attempt playback immediately, or mark it for retry if autoplay is blocked.
*************************************************************************************/
EM_JS(int, SofaWebAudio_PlaySound, (const char* namePtr, int channelId, float volume, float pitch, int loop), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return 0;

    var name = UTF8ToString(namePtr);
    var entry = state.sounds.get(name);
    if (!entry) return 0;

    try {
        var player = state.createPlayer(entry, name, channelId | 0, volume, pitch, !!loop);
        state.tryPlayPlayer(player);
        return 1;
    } catch (error) {
        console.error('[SofaSpuds][WebAudio] Failed to play sound', name, error);
        return 0;
    }
});

/*************************************************************************************
  \brief Stop every active player associated with one logical sound id.

  \param namePtr  UTF-8 pointer to the logical sound id.
*************************************************************************************/
EM_JS(void, SofaWebAudio_StopSound, (const char* namePtr), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return;
    state.stopSound(UTF8ToString(namePtr));
});

/*************************************************************************************
  \brief Stop all active browser audio players tracked by the web backend.
*************************************************************************************/
EM_JS(void, SofaWebAudio_StopAll, (), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return;
    state.sounds.forEach(function (entry) {
        Array.from(entry.players).forEach(function (player) {
            state.stopPlayer(player);
        });
    });
    state.channels.clear();
});

/*************************************************************************************
  \brief Pause or resume all players associated with one logical sound id.

  \param namePtr  UTF-8 pointer to the logical sound id.
  \param pause    Non-zero to pause, zero to resume.
*************************************************************************************/
EM_JS(void, SofaWebAudio_PauseSound, (const char* namePtr, int pause), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return;

    var name = UTF8ToString(namePtr);
    var entry = state.sounds.get(name);
    if (!entry) return;

    entry.players.forEach(function (player) {
        player.__sofaPausedBySound = !!pause;
        player.__sofaWaitingForGesture = false;

        if (pause) {
            try { player.pause(); } catch (error) {}
        } else if (!state.pausedAll) {
            state.tryPlayPlayer(player);
        }
    });
});

/*************************************************************************************
  \brief Pause or resume all active browser audio players.

  \param pause  Non-zero to pause all audio, zero to resume audio that is not
                explicitly paused per-sound.
*************************************************************************************/
EM_JS(void, SofaWebAudio_PauseAll, (int pause), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return;

    state.pausedAll = !!pause;
    state.sounds.forEach(function (entry) {
        entry.players.forEach(function (player) {
            if (pause) {
                player.__sofaWaitingForGesture = false;
                try { player.pause(); } catch (error) {}
            } else if (!player.__sofaPausedBySound) {
                state.tryPlayPlayer(player);
            }
        });
    });
});

/*************************************************************************************
  \brief Set the browser-side master volume for all active players.

  \param volume  Requested master gain in the range [0, 1].
*************************************************************************************/
EM_JS(void, SofaWebAudio_SetMasterVolume, (float volume), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return;

    state.masterVolume = state.clamp01(volume);
    state.sounds.forEach(function (entry) {
        entry.players.forEach(function (player) {
            state.updatePlayerVolume(player);
        });
    });
});

/*************************************************************************************
  \brief Set the logical volume for all active players of one sound id.

  \param namePtr  UTF-8 pointer to the logical sound id.
  \param volume   Requested per-sound gain in the range [0, 1].
*************************************************************************************/
EM_JS(void, SofaWebAudio_SetSoundVolume, (const char* namePtr, float volume), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return;

    var entry = state.sounds.get(UTF8ToString(namePtr));
    if (!entry) return;

    entry.players.forEach(function (player) {
        player.__sofaVolume = state.clamp01(volume);
        state.updatePlayerVolume(player);
    });
});

/*************************************************************************************
  \brief Set the playback rate for all active players of one sound id.

  \param namePtr  UTF-8 pointer to the logical sound id.
  \param pitch    Requested playback-rate multiplier.
*************************************************************************************/
EM_JS(void, SofaWebAudio_SetSoundPitch, (const char* namePtr, float pitch), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return;

    var entry = state.sounds.get(UTF8ToString(namePtr));
    if (!entry) return;

    entry.players.forEach(function (player) {
        player.playbackRate = pitch > 0 ? pitch : 1.0;
    });
});

/*************************************************************************************
  \brief Set the loop state for all active players of one sound id.

  \param namePtr  UTF-8 pointer to the logical sound id.
  \param loop     Non-zero to loop, zero for one-shot playback.
*************************************************************************************/
EM_JS(void, SofaWebAudio_SetSoundLoop, (const char* namePtr, int loop), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return;

    var entry = state.sounds.get(UTF8ToString(namePtr));
    if (!entry) return;

    entry.loopDefault = !!loop;
    entry.players.forEach(function (player) {
        player.loop = !!loop;
    });
});

/*************************************************************************************
  \brief Query whether a logical sound id is currently loaded in browser state.

  \param namePtr  UTF-8 pointer to the logical sound id.
  \return Non-zero when the sound exists in the shared JS map, otherwise zero.
*************************************************************************************/
EM_JS(int, SofaWebAudio_IsSoundLoaded, (const char* namePtr), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return 0;
    return state.sounds.has(UTF8ToString(namePtr)) ? 1 : 0;
});

/*************************************************************************************
  \brief Query whether any active player for a logical sound is currently playing.

  \param namePtr  UTF-8 pointer to the logical sound id.
  \return Non-zero when at least one player is active and not paused, otherwise zero.
*************************************************************************************/
EM_JS(int, SofaWebAudio_IsSoundPlaying, (const char* namePtr), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return 0;

    var entry = state.sounds.get(UTF8ToString(namePtr));
    if (!entry) return 0;

    var playing = false;
    entry.players.forEach(function (player) {
        if (!player.__sofaDestroyed && !player.paused && !player.ended) {
            playing = true;
        }
    });
    return playing ? 1 : 0;
});

/*************************************************************************************
  \brief Query whether a tracked browser-side channel is still playing.

  \param channelId  Logical ChannelID previously returned to C++.
  \return Non-zero when the channel's player is still active, otherwise zero.
*************************************************************************************/
EM_JS(int, SofaWebAudio_IsChannelPlaying, (int channelId), {
    var state = window.SofaSpudsWebAudio;
    if (!state) return 0;

    var player = state.channels.get(channelId | 0);
    if (!player || player.__sofaDestroyed) return 0;

    return (!player.paused && !player.ended) ? 1 : 0;
});

struct AudioManager::Impl
{
    struct SoundInfo
    {
        std::string path;
        bool loop{};
        bool is3D{};
        bool browserLoaded{};
    };

    struct FadeInfo
    {
        std::string name;
        float startVolume{};
        float endVolume{};
        float duration{};
        float elapsed{};
        bool stopOnComplete{};
    };

    std::unordered_map<std::string, SoundInfo> sounds;
    std::unordered_map<std::string, std::vector<AudioManager::ChannelID>> channelsBySound;
    std::unordered_map<AudioManager::ChannelID, std::string> channelToSound;
    // Web audio works on live browser players, so we track the latest requested
    // logical volume here and push it back out when fades or menu sliders change.
    std::unordered_map<std::string, float> soundVolumes;
    float masterVolume{ 1.0f };
    std::vector<FadeInfo> fades;
};

/*************************************************************************************
  \brief Ensure one registered sound has been materialized into browser playback state.

  \param impl  Web-audio implementation storage.
  \param name  Logical sound id to materialize on demand.
  \return True when the sound is ready for browser playback, false otherwise.

  Web builds previously converted every sound file into a Blob/object URL during
  startup. That made initial page-open audio hitch badly because the main thread had
  to copy and wrap the entire audio library up front. We now register sounds cheaply
  at startup and only do the expensive browser-side load the first time a sound is
  actually needed.
*************************************************************************************/
template <typename ImplT>
bool EnsureBrowserSoundLoaded(ImplT& impl, const std::string& name)
{
    auto it = impl.sounds.find(name);
    if (it == impl.sounds.end())
        return false;

    auto& info = it->second;
    if (info.browserLoaded)
        return true;

    if (SofaWebAudio_LoadSound(name.c_str(), info.path.c_str(), info.loop ? 1 : 0) == 0)
        return false;

    info.browserLoaded = true;
    return true;
}

AudioManager::AudioManager()
    : pImpl(new Impl())
{
}

/*************************************************************************************
  \brief Destroy the web audio manager and release its browser-side resources.

  Steps:
  - Call shutdown() so browser audio state is stopped and cleared.
  - Delete the private implementation storage.
*************************************************************************************/
AudioManager::~AudioManager()
{
    shutdown();
    delete pImpl;
    pImpl = nullptr;
}

/*************************************************************************************
  \brief Initialize the browser audio backend for the current web session.

  \return True if the shared JS audio state was created successfully, false otherwise.

  Steps:
  - Ask the EM_JS bridge to initialize the shared browser audio state.
  - Log the result for troubleshooting in the browser console / runtime log.
*************************************************************************************/
bool AudioManager::initialize()
{
    const bool initialized = SofaWebAudio_Init() != 0;
    if (initialized)
        std::cout << "[WebAudio] Browser audio initialized.\n";
    else
        std::cerr << "[WebAudio] Failed to initialize browser audio.\n";
    return initialized;
}

/*************************************************************************************
  \brief Shut down the browser audio backend and clear all cached runtime state.

  Steps:
  - Clear fade tracking and ChannelID bookkeeping kept on the C++ side.
  - Drop all remembered sound metadata and cached volume values.
  - Tell the browser bridge to stop players, unload object URLs, and remove listeners.
*************************************************************************************/
void AudioManager::shutdown()
{
    if (!pImpl)
        return;

    pImpl->fades.clear();
    pImpl->channelToSound.clear();
    pImpl->channelsBySound.clear();
    pImpl->soundVolumes.clear();
    pImpl->sounds.clear();
    SofaWebAudio_Shutdown();
}

/*************************************************************************************
  \brief Advance per-frame web audio maintenance.

  \param dt  Delta time in seconds.

  Steps:
  - Update active fade operations tracked on the C++ side.
  - Remove stopped channel ids from the web bookkeeping tables.
*************************************************************************************/
void AudioManager::update(float dt)
{
    updateFades(dt);
    pruneStoppedChannels();
}

/*************************************************************************************
  \brief Load one sound into the browser-backed audio manager.

  \param name      Logical id used by gameplay code to reference the sound later.
  \param filePath  Relative or absolute path to the packaged audio file.
  \param loop      Default loop flag for the loaded sound metadata.
  \param is3D      True when the sound was requested through the 3D load path.
  \return True on success, false on failure.

  Steps:
  - Resolve the incoming path against the project's packaged Assets folders.
  - Verify the file exists in the Emscripten-visible filesystem.
  - Cache sound metadata and default logical volume in the C++ side tables.
  - Defer the expensive browser-side Blob/object-URL creation until first playback.
*************************************************************************************/
bool AudioManager::loadSound(const std::string& name, const std::string& filePath, bool loop, bool is3D)
{
    if (!pImpl)
        return false;

    const std::string fullPath = getFullPath(filePath);
    if (fullPath.empty())
    {
        std::cerr << "[WebAudio] Failed to resolve sound path for '" << name << "'.\n";
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(std::filesystem::path(fullPath), ec))
    {
        std::cerr << "[WebAudio] Audio file not found: " << fullPath << '\n';
        return false;
    }

    const auto existing = pImpl->sounds.find(name);
    if (existing != pImpl->sounds.end())
    {
        if (existing->second.path == fullPath)
        {
            existing->second.loop = loop;
            existing->second.is3D = is3D;
            if (existing->second.browserLoaded)
                SofaWebAudio_SetSoundLoop(name.c_str(), loop ? 1 : 0);
            return true;
        }

        unloadSound(name);
    }

    pImpl->sounds[name] = { fullPath, loop, is3D, false };
    pImpl->soundVolumes.try_emplace(name, 1.0f);
    return true;
}

/*************************************************************************************
  \brief Accept listener-position updates from gameplay code.

  \param pos      Listener position pointer.
  \param forward  Listener forward vector pointer.
  \param up       Listener up vector pointer.

  The current browser backend is non-spatial, so these values are accepted to preserve
  API parity with desktop but intentionally ignored.
*************************************************************************************/
void AudioManager::setListenerPosition(const void* pos, const void* forward, const void* up)
{
    // The browser fallback currently provides non-spatial playback only.
    (void)pos;
    (void)forward;
    (void)up;
}

/*************************************************************************************
  \brief Accept per-sound position updates from gameplay code.

  \param name  Logical sound id.
  \param pos   Sound position pointer.
  \param vel   Sound velocity pointer.

  Spatial routing is reserved for future web work, so these inputs are currently no-ops.
*************************************************************************************/
void AudioManager::setSoundPosition(const std::string& name, const void* pos, const void* vel)
{
    // Positional updates are ignored for now; the web backend still routes through
    // the same API so game code does not need platform-specific branches.
    (void)name;
    (void)pos;
    (void)vel;
}

/*************************************************************************************
  \brief Unload one sound and clear all runtime state associated with it.

  \param name  Logical sound id to unload.

  Steps:
  - Stop all currently playing instances of the sound.
  - Tell the browser bridge to revoke and discard the loaded sound.
  - Remove cached sound metadata, fade entries, volume, and channel bookkeeping.
*************************************************************************************/
void AudioManager::unloadSound(const std::string& name)
{
    if (!pImpl)
        return;

    stopSound(name);
    SofaWebAudio_UnloadSound(name.c_str());

    pImpl->sounds.erase(name);
    pImpl->soundVolumes.erase(name);
    pImpl->channelsBySound.erase(name);
    pImpl->fades.erase(
        std::remove_if(pImpl->fades.begin(), pImpl->fades.end(),
            [&name](const Impl::FadeInfo& fade) { return fade.name == name; }),
        pImpl->fades.end());

    for (auto it = pImpl->channelToSound.begin(); it != pImpl->channelToSound.end();)
    {
        if (it->second == name)
            it = pImpl->channelToSound.erase(it);
        else
            ++it;
    }
}

/*************************************************************************************
  \brief Unload every currently loaded sound from the web backend.

  Steps:
  - Iterate all known sound ids and forward unload requests to the browser bridge.
  - Clear every C++ side cache used for sounds, channels, and fades.
*************************************************************************************/
void AudioManager::unloadAllSounds()
{
    if (!pImpl)
        return;

    const auto loaded = getLoadedSounds();
    for (const auto& name : loaded)
        SofaWebAudio_UnloadSound(name.c_str());

    pImpl->sounds.clear();
    pImpl->soundVolumes.clear();
    pImpl->channelsBySound.clear();
    pImpl->channelToSound.clear();
    pImpl->fades.clear();
}

/*************************************************************************************
  \brief Play a logical sound through the browser backend.

  \param name    Logical sound id to play.
  \param volume  Requested playback volume.
  \param pitch   Requested playback rate.
  \param loop    True to loop the new player.
  \return True on success, false otherwise.

  Steps:
  - Ensure the requested sound has been materialized into browser playback state.
  - Remove stale channel bookkeeping before starting new playback.
  - Forward the play request to the browser bridge.
  - Remember the latest logical per-sound volume for later fades/slider updates.
*************************************************************************************/
bool AudioManager::playSound(const std::string& name, float volume, float pitch, bool loop)
{
    if (!pImpl || pImpl->sounds.find(name) == pImpl->sounds.end())
        return false;

    if (!EnsureBrowserSoundLoaded(*pImpl, name))
        return false;

    pruneStoppedChannels();

    const float targetVolume = ClampVolume(volume);
    const float targetPitch = NormalizePitch(pitch);
    if (SofaWebAudio_PlaySound(name.c_str(), 0, targetVolume, targetPitch, loop ? 1 : 0) == 0)
        return false;

    pImpl->soundVolumes[name] = targetVolume;
    return true;
}

/*************************************************************************************
  \brief Stop all active players belonging to one logical sound id.

  \param name  Logical sound id to stop.

  Also removes any channel and fade bookkeeping tied to the sound.
*************************************************************************************/
void AudioManager::stopSound(const std::string& name)
{
    if (!pImpl)
        return;

    SofaWebAudio_StopSound(name.c_str());
    pImpl->channelsBySound.erase(name);
    pImpl->fades.erase(
        std::remove_if(pImpl->fades.begin(), pImpl->fades.end(),
            [&name](const Impl::FadeInfo& fade) { return fade.name == name; }),
        pImpl->fades.end());

    for (auto it = pImpl->channelToSound.begin(); it != pImpl->channelToSound.end();)
    {
        if (it->second == name)
            it = pImpl->channelToSound.erase(it);
        else
            ++it;
    }
}

/*************************************************************************************
  \brief Stop every active sound currently tracked by the web backend.

  Steps:
  - Forward a global stop request to the browser bridge.
  - Clear C++ side channel and fade bookkeeping.
*************************************************************************************/
void AudioManager::stopAllSounds()
{
    if (!pImpl)
        return;

    SofaWebAudio_StopAll();
    pImpl->channelsBySound.clear();
    pImpl->channelToSound.clear();
    pImpl->fades.clear();
}

/*************************************************************************************
  \brief Pause or resume one logical sound id.

  \param name   Logical sound id to update.
  \param pause  True to pause, false to resume.
*************************************************************************************/
void AudioManager::pauseSound(const std::string& name, bool pause)
{
    SofaWebAudio_PauseSound(name.c_str(), pause ? 1 : 0);
}

/*************************************************************************************
  \brief Pause or resume all active sounds managed by the web backend.

  \param pause  True to pause, false to resume.
*************************************************************************************/
void AudioManager::pauseAllSounds(bool pause)
{
    SofaWebAudio_PauseAll(pause ? 1 : 0);
}

/*************************************************************************************
  \brief Set the master volume applied to all browser-side players.

  \param volume  Requested master volume.
*************************************************************************************/
void AudioManager::setMasterVolume(float volume)
{
    if (!pImpl)
        return;

    pImpl->masterVolume = ClampVolume(volume);
    SofaWebAudio_SetMasterVolume(pImpl->masterVolume);
}

/*************************************************************************************
  \brief Set the logical volume for one sound id's active players.

  \param name    Logical sound id.
  \param volume  Requested per-sound volume.
*************************************************************************************/
void AudioManager::setSoundVolume(const std::string& name, float volume)
{
    if (!pImpl || pImpl->sounds.find(name) == pImpl->sounds.end())
        return;

    pImpl->soundVolumes[name] = ClampVolume(volume);
    if (pImpl->sounds[name].browserLoaded)
        SofaWebAudio_SetSoundVolume(name.c_str(), pImpl->soundVolumes[name]);
}

/*************************************************************************************
  \brief Set the playback rate for one sound id's active players.

  \param name   Logical sound id.
  \param pitch  Requested playback-rate multiplier.
*************************************************************************************/
void AudioManager::setSoundPitch(const std::string& name, float pitch)
{
    if (!pImpl || pImpl->sounds.find(name) == pImpl->sounds.end())
        return;

    if (pImpl->sounds[name].browserLoaded)
        SofaWebAudio_SetSoundPitch(name.c_str(), NormalizePitch(pitch));
}

/*************************************************************************************
  \brief Set the loop state for one loaded sound id.

  \param name  Logical sound id.
  \param loop  True to loop, false for one-shot behavior.
*************************************************************************************/
void AudioManager::setSoundLoop(const std::string& name, bool loop)
{
    if (!pImpl || pImpl->sounds.find(name) == pImpl->sounds.end())
        return;

    pImpl->sounds[name].loop = loop;
    if (pImpl->sounds[name].browserLoaded)
        SofaWebAudio_SetSoundLoop(name.c_str(), loop ? 1 : 0);
}

/*************************************************************************************
  \brief Report whether one logical sound id is currently loaded.

  \param name  Logical sound id.
  \return True when the sound has been registered with the web backend, otherwise false.
*************************************************************************************/
bool AudioManager::isSoundLoaded(const std::string& name) const
{
    if (!pImpl)
        return false;

    return pImpl->sounds.find(name) != pImpl->sounds.end();
}

/*************************************************************************************
  \brief Report whether one logical sound id is currently playing.

  \param name  Logical sound id.
  \return True when at least one active player is still playing, otherwise false.
*************************************************************************************/
bool AudioManager::isSoundPlaying(const std::string& name) const
{
    if (!pImpl)
        return false;

    return SofaWebAudio_IsSoundPlaying(name.c_str()) != 0;
}

/*************************************************************************************
  \brief Begin a fade-in for one logical sound id.

  \param name          Logical sound id.
  \param duration      Fade duration in seconds.
  \param targetVolume  Final target volume after the fade.

  Steps:
  - Validate that the sound is loaded.
  - If duration is zero, apply the final volume immediately.
  - Otherwise set the sound's current volume to zero.
  - Store a fade record so `updateFades()` can ramp the volume over time.
*************************************************************************************/
void AudioManager::fadeInSound(const std::string& name, float duration, float targetVolume)
{
    if (!pImpl || pImpl->sounds.find(name) == pImpl->sounds.end())
        return;

    const float clampedTarget = ClampVolume(targetVolume);
    if (duration <= 0.0f)
    {
        setSoundVolume(name, clampedTarget);
        return;
    }

    SofaWebAudio_SetSoundVolume(name.c_str(), 0.0f);
    pImpl->soundVolumes[name] = 0.0f;
    pImpl->fades.push_back({ name, 0.0f, clampedTarget, duration, 0.0f, false });
}

/*************************************************************************************
  \brief Begin a fade-out for one logical sound id.

  \param name      Logical sound id.
  \param duration  Fade duration in seconds.

  Steps:
  - Validate that the sound is loaded.
  - If duration is zero, stop the sound immediately.
  - Otherwise capture the current logical volume as the fade start value.
  - Store a fade record that will ramp to zero and stop the sound when complete.
*************************************************************************************/
void AudioManager::fadeOutSound(const std::string& name, float duration)
{
    if (!pImpl || pImpl->sounds.find(name) == pImpl->sounds.end())
        return;

    if (duration <= 0.0f)
    {
        stopSound(name);
        return;
    }

    const float startVolume =
        pImpl->soundVolumes.find(name) != pImpl->soundVolumes.end() ? pImpl->soundVolumes[name] : 1.0f;
    pImpl->fades.push_back({ name, ClampVolume(startVolume), 0.0f, duration, 0.0f, true });
}

/*************************************************************************************
  \brief Play a sound and return a tracked logical ChannelID.

  \param name    Logical sound id to play.
  \param volume  Requested playback volume.
  \param pitch   Requested playback rate.
  \param loop    True to loop the created player.
  \param pos     Optional sound position (currently ignored on web).
  \param vel     Optional sound velocity (currently ignored on web).
  \return A non-zero logical ChannelID on success, otherwise 0.

  Steps:
  - Ensure the requested sound is already loaded.
  - Prune stale tracked channels.
  - Allocate the next logical ChannelID.
  - Ask the browser bridge to play the sound while associating that channel id.
  - Cache the channel-to-sound relationship for later status queries and cleanup.
*************************************************************************************/
AudioManager::ChannelID AudioManager::playSoundChannel(
    const std::string& name,
    float volume,
    float pitch,
    bool loop,
    const FMOD_VECTOR* pos,
    const FMOD_VECTOR* vel)
{
    // We still hand back a ChannelID on web so enemy/gameplay code can keep using
    // the same bookkeeping path even though the underlying player is HTML audio.
    (void)pos;
    (void)vel;

    if (!pImpl || pImpl->sounds.find(name) == pImpl->sounds.end())
        return 0;

    if (!EnsureBrowserSoundLoaded(*pImpl, name))
        return 0;

    pruneStoppedChannels();

    const ChannelID id = m_nextChannelId++;
    const float targetVolume = ClampVolume(volume);
    const float targetPitch = NormalizePitch(pitch);
    if (SofaWebAudio_PlaySound(name.c_str(), static_cast<int>(id), targetVolume, targetPitch, loop ? 1 : 0) == 0)
        return 0;

    pImpl->channelsBySound[name].push_back(id);
    pImpl->channelToSound[id] = name;
    pImpl->soundVolumes[name] = targetVolume;
    return id;
}

/*************************************************************************************
  \brief Accept a 3D position update for a tracked logical channel.

  \param id   Logical ChannelID.
  \param pos  Optional position pointer.
  \param vel  Optional velocity pointer.

  Spatial channel movement is reserved for future web audio work, so this is currently
  a no-op that preserves API compatibility with the native backend.
*************************************************************************************/
void AudioManager::setChannel3DPosition(ChannelID id, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel)
{
    // Reserved for future spatial web audio work.
    (void)id;
    (void)pos;
    (void)vel;
}

/*************************************************************************************
  \brief Report whether a tracked logical ChannelID is still active.

  \param id  Logical ChannelID to query.
  \return True when the browser bridge reports the player as still playing.
*************************************************************************************/
bool AudioManager::isChannelPlaying(ChannelID id)
{
    if (!pImpl || id == 0)
        return false;

    return SofaWebAudio_IsChannelPlaying(static_cast<int>(id)) != 0;
}

/*************************************************************************************
  \brief Return the list of currently loaded logical sound ids.

  \return Vector containing every sound id tracked by the web backend.
*************************************************************************************/
std::vector<std::string> AudioManager::getLoadedSounds() const
{
    std::vector<std::string> sounds;
    if (!pImpl)
        return sounds;

    sounds.reserve(pImpl->sounds.size());
    for (const auto& [name, info] : pImpl->sounds)
    {
        (void)info;
        sounds.push_back(name);
    }
    return sounds;
}

/*************************************************************************************
  \brief Resolve an audio file path against the project's packaged web asset layout.

  \param fileName  Incoming relative or absolute file path.
  \return The best matching packaged path, normalized for browser-side use.

  Steps:
  - Accept the original path immediately if it already exists.
  - Probe the project's `Assets/Audio` and `Assets/Audio__OFF_WEB` locations.
  - Try both the full relative path and the filename-only form for compatibility with
    existing call sites.
  - Fall back to the original input when nothing more specific can be found.
*************************************************************************************/
std::string AudioManager::getFullPath(const std::string& fileName) const
{
    namespace fs = std::filesystem;

    auto normalize = [](const fs::path& value)
    {
        return value.generic_string();
    };

    const fs::path input(fileName);
    std::error_code ec;
    if (fs::exists(input, ec))
        return normalize(input);

    // Mirror the desktop lookup behavior, but also try the web-only fallback folder
    // so packaged builds still find assets when a project uses Audio__OFF_WEB.
    const std::vector<fs::path> candidates = {
        Framework::ResolveAssetPath(input),
        Framework::ResolveAssetPath(fs::path("Audio") / input),
        Framework::ResolveAssetPath(fs::path("Audio__OFF_WEB") / input),
        Framework::ResolveAssetPath(fs::path("Audio") / input.filename()),
        Framework::ResolveAssetPath(fs::path("Audio__OFF_WEB") / input.filename())
    };

    for (const auto& candidate : candidates)
    {
        std::error_code candidateEc;
        if (!candidate.empty() && fs::exists(candidate, candidateEc))
            return normalize(candidate);
    }

    return normalize(input);
}

/*************************************************************************************
  \brief Advance all active fade operations tracked by the web backend.

  \param deltaTime  Time elapsed since the previous frame, in seconds.

  Steps:
  - Remove any fade whose sound has already been unloaded.
  - Advance the fade timer and compute the interpolated logical volume.
  - Forward the new volume to the browser bridge.
  - Remove completed fades and stop their sound when finishing a fade-out.
*************************************************************************************/
void AudioManager::updateFades(float deltaTime)
{
    if (!pImpl)
        return;

    // Fades are tracked on the C++ side so menu/gameplay code can reuse the same calls
    // on desktop FMOD and browser audio without caring about the backend details.
    for (auto it = pImpl->fades.begin(); it != pImpl->fades.end();)
    {
        if (pImpl->sounds.find(it->name) == pImpl->sounds.end())
        {
            it = pImpl->fades.erase(it);
            continue;
        }

        it->elapsed += deltaTime;
        float t = it->duration > 0.0f ? (it->elapsed / it->duration) : 1.0f;
        t = std::clamp(t, 0.0f, 1.0f);

        const float volume = it->startVolume + t * (it->endVolume - it->startVolume);
        pImpl->soundVolumes[it->name] = ClampVolume(volume);
        SofaWebAudio_SetSoundVolume(it->name.c_str(), pImpl->soundVolumes[it->name]);

        if (t >= 1.0f)
        {
            const std::string finishedName = it->name;
            const bool stopOnComplete = it->stopOnComplete;
            it = pImpl->fades.erase(it);

            if (stopOnComplete)
            {
                SofaWebAudio_StopSound(finishedName.c_str());
                pImpl->channelsBySound.erase(finishedName);

                for (auto channelIt = pImpl->channelToSound.begin();
                     channelIt != pImpl->channelToSound.end();)
                {
                    if (channelIt->second == finishedName)
                        channelIt = pImpl->channelToSound.erase(channelIt);
                    else
                        ++channelIt;
                }
            }
        }
        else
        {
            ++it;
        }
    }
}

/*************************************************************************************
  \brief FMOD-style error helper retained for API parity with the native backend.

  \param result     Placeholder FMOD result value.
  \param operation  Description of the attempted operation.

  The browser backend does not use FMOD, so this is intentionally a no-op.
*************************************************************************************/
void AudioManager::checkFMODError(FMOD_RESULT result, const std::string& operation) const
{
    (void)result;
    (void)operation;
}

/*************************************************************************************
  \brief Remove tracked ChannelIDs whose browser-side players have already stopped.

  Steps:
  - Query each tracked channel through the browser bridge.
  - Remove stale channel ids from both the reverse lookup table and per-sound lists.
  - Drop empty per-sound channel lists after cleanup.
*************************************************************************************/
void AudioManager::pruneStoppedChannels()
{
    if (!pImpl)
        return;

    for (auto it = pImpl->channelToSound.begin(); it != pImpl->channelToSound.end();)
    {
        if (SofaWebAudio_IsChannelPlaying(static_cast<int>(it->first)) != 0)
        {
            ++it;
            continue;
        }

        const auto soundIt = pImpl->channelsBySound.find(it->second);
        if (soundIt != pImpl->channelsBySound.end())
        {
            auto& ids = soundIt->second;
            ids.erase(std::remove(ids.begin(), ids.end(), it->first), ids.end());
            if (ids.empty())
                pImpl->channelsBySound.erase(soundIt);
        }

        it = pImpl->channelToSound.erase(it);
    }
}
