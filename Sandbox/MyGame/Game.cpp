/*********************************************************************************************
 \file      Game.cpp
 \par       SofaSpuds
 \author    All TEAM MEMBERS
 \brief     Game lifecycle management + Main Menu transition (GUISystem-backed)
*********************************************************************************************/

#include "Graphics/Window.hpp"
#include "Systems/SystemManager.h"
#include "Systems/InputSystem.h"
#include "Systems/LogicSystem.h"
#include "Systems/HitBoxSystem.h"
#include "Systems/PhysicSystem.h"
#include "Systems/RenderSystem.h"
#include "Factory/Factory.h"
#include "Systems/audioSystem.h"
#include "Systems/AiSystem.h"
#include "Systems/ParticleSystem.h"
#include "Runtime/EnemySystem.h"
#include "Runtime/HealthSystem.h"
#include "Runtime/ZoomTriggerSystem.h"
#include "Audio/SoundManager.h"
#include "Debug/CrashLogger.hpp"
#include "Graphics/Graphics.hpp"
#include "Core/PathUtils.h"
#include "Debug/Perf.h"
#include "Memory/GameObjectPool.h"
#include "Memory/ObjectAllocator.h"
#include "Component/FlashComponent.h"
#include "Component/RenderComponent.h"
#include "Physics/Dynamics/RigidBodyComponent.h"
#include "Video/VideoPlayer.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <GLFW/glfw3.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <MainMenuPage.hpp>
#include <PauseMenuPage.hpp>
#include <DefeatScreenPage.hpp>
#include "EngineCall.hpp"
#include "HealthPresentation.hpp"
#include "LevelLoader.hpp"
#include "Common/GameComponentIDs.h"
#include "Components/EnemyComponent.h"
#include "Components/PlayerComponent.h"
#include "VfxPresets.hpp"

#include "Common/CRTDebug.h"   

#ifdef _DEBUG
#define new DBG_NEW       
#endif

namespace mygame {

    namespace {
        //Audio booleans
        //Main Menu Sounds
        bool mainMenuBGMPlaying = false;
        const char* MAIN_MENU_BGM = "MenuMusic";
        const char* START_BUTTTON = "MenuGameStart";
        const char* EXIT_BUTTTON = "Quit";
        const char* CUTSCENE_AUDIO = "CutsceneAudio";
        const char* WIN_VIDEO_AUDIO = "WinVideoAudio";
        const char* FINAL_LEVEL_CUTSCENE_AUDIO = "FinalLevelCutsceneAudio";

        // BGM Sounds
        bool gameplayBGMPlaying = false;
        const char* GAMEPLAY_BGM = "BGM";

        //Defeat Sounds
        const char* DEFEAT = "Defeat";
        const char* BOILING = "Boiling";
        bool defeatBGMPlaying = false;
        bool defeatSoundStarted = false;
        bool boilingStarted = false;
        //Boss music change
        const char* LEVEL3_BOSS_BGM = "MiniBoss";
        bool miniBossMusicFading = false;
        const char* LEVEL4_BOSS_BGM = "FinalBoss";
        bool levelMusicInitialized = false;

        std::string currentLevelMusic = "";

        //Timer
        float bgmFadeTimer = 0.0f;
        constexpr float kBGMFadeDuration = 1.5f;

        using clock = std::chrono::high_resolution_clock;
        const std::array<const char*, 4> kBgmSoundIds = {
            "MenuMusic",
            "BGM",
            "MiniBoss",
            "FinalBoss"
        };

        /*************************************************************************************
         \brief  Checks whether a loaded sound id belongs to background music.
         \param  name  The sound id to test.
         \return True if the id is one of the registered BGM tracks.
        *************************************************************************************/
        bool IsBgmSoundId(const std::string& name)
        {
            return std::any_of(kBgmSoundIds.begin(), kBgmSoundIds.end(),
                [&name](const char* id) { return name == id; });
        }

        /*************************************************************************************
         \brief  Applies the current music volume to all registered BGM sounds.
         \param  volume  Target music volume scalar.
        *************************************************************************************/
        void ApplyBgmVolume(float volume)
        {
            SoundManager& sm = SoundManager::getInstance();
            for (const char* id : kBgmSoundIds) {
                if (sm.isSoundLoaded(id)) {
                    sm.setSoundVolume(id, volume);
                }
            }
        }

        /*************************************************************************************
         \brief  Applies the current SFX volume to all non-BGM loaded sounds.
         \param  volume  Target sound-effects volume scalar.
        *************************************************************************************/
        void ApplySfxVolume(float volume)
        {
            SoundManager& sm = SoundManager::getInstance();
            const auto sounds = sm.getLoadedSounds();
            for (const auto& name : sounds) {
                if (!IsBgmSoundId(name)) {
                    sm.setSoundVolume(name, volume);
                }
            }
        }

       


        void PlayMainMenuMusic(float targetVolume = 0.3f)
        {
            SoundManager& sm = SoundManager::getInstance();
            if (!sm.isSoundLoaded(MAIN_MENU_BGM)) {
                mainMenuBGMPlaying = false;
                return;
            }

            if (!sm.isSoundPlaying(MAIN_MENU_BGM)) {
                sm.playSound(MAIN_MENU_BGM, 1.0f, 1.0f, true);
                sm.setSoundVolume(MAIN_MENU_BGM, 0.0f);
                sm.fadeInMusic(MAIN_MENU_BGM, kBGMFadeDuration, targetVolume);
            }

            mainMenuBGMPlaying = true;
            gameplayBGMPlaying = false;
        }

        void FadeOutMainMenuMusic()
        {
            SoundManager& sm = SoundManager::getInstance();
            if (sm.isSoundLoaded(MAIN_MENU_BGM) && sm.isSoundPlaying(MAIN_MENU_BGM)) {
                sm.fadeOutMusic(MAIN_MENU_BGM, kBGMFadeDuration);
            }
            mainMenuBGMPlaying = false;
        }

        std::string ResolveFirstExistingAsset(std::initializer_list<const char*> candidates)
        {
            for (const char* rel : candidates)
            {
                if (!rel || rel[0] == '\0')
                    continue;
                const auto resolved = Framework::ResolveAssetPath(rel);
                if (std::filesystem::exists(resolved))
                    return resolved.string();
            }
            return {};
        }

        bool EnsureSoundLoadedFromAssets(const char* soundId,
            std::initializer_list<const char*> candidates,
            bool loop = false)
        {
            if (!soundId || soundId[0] == '\0')
                return false;

            SoundManager& sm = SoundManager::getInstance();
            if (sm.isSoundLoaded(soundId))
                return true;

            const std::string resolvedPath = ResolveFirstExistingAsset(candidates);
            if (resolvedPath.empty())
                return false;

            return sm.loadSound(soundId, resolvedPath, loop);
        }

        void EnsureBossMusicLoaded()
        {
            EnsureSoundLoadedFromAssets(LEVEL3_BOSS_BGM, {
                "Audio/MiniBoss_Loop.wav",
                "Audio__OFF_WEB/MiniBoss_Loop.wav"
                }, true);
            EnsureSoundLoadedFromAssets(LEVEL4_BOSS_BGM, {
                "Audio/FinalBoss_Loop.wav",
                "Audio__OFF_WEB/FinalBoss_Loop.wav"
                }, true);
        }

        Framework::SystemManager gSystems;
        Framework::InputSystem* gInputSystem = nullptr;
        Framework::LogicSystem* gLogicSystem = nullptr;
        Framework::PhysicSystem* gPhysicsSystem = nullptr;
        Framework::AudioSystem* gAudioSystem = nullptr;
        Framework::RenderSystem* gRenderSystem = nullptr;
        Framework::EnemySystem* gEnemySystem = nullptr;
        Framework::AiSystem* gAiSystem = nullptr;
        Framework::HealthSystem* gHealthSystem = nullptr;
        Framework::ParticleSystem* gParticleSystem = nullptr;
        Framework::ZoomTriggerSystem* gZoomTriggerSystem = nullptr;

        enum class GameState { MAIN_MENU, CUTSCENE, FINAL_LEVEL_CUTSCENE, TRANSITIONING, LOADING_TRANSITION, PLAYING, PAUSED, DEFEAT, WIN_VIDEO, EXIT };
        GameState currentState = GameState::MAIN_MENU;
        bool editorSimulationRunning = false;
       

        MainMenuPage mainMenu;
        PauseMenuPage pauseMenu;
        DefeatScreenPage defeatScreen;
        Framework::VideoPlayer cutscenePlayer;
        Framework::VideoPlayer finalLevelCutscenePlayer;
        Framework::VideoPlayer transitionPlayer;
        Framework::VideoPlayer winVideoPlayer;
        LevelLoader levelLoader;
        bool cutsceneReady = false;
        bool cutsceneAudioPlaying = false;
        bool finalLevelCutsceneReady = false;
        bool finalLevelCutsceneAudioPlaying = false;
        bool winVideoAudioPlaying = false;
        bool transitionReady = false;
        bool winVideoReady = false;
        bool loadingTransitionVideoEnabled = true;
        bool loadingTransitionSkipRequested = false;
        bool loadingTransitionRequiresLevelLoad = true;
        bool loadingTransitionHideGameplayUntilDelay = false;
        bool loadingTransitionResumeSimulation = true;
        GameState loadingTransitionNextState = GameState::PLAYING;
        constexpr std::size_t kLoadingObjectsPerTick = 8;
        constexpr float kLoadingTransitionMinimumVideoTime = 1.0f;
        constexpr float kTransitionColorKeyLow = 0.02f;
        constexpr float kTransitionColorKeyHigh = 0.18f;
        const std::filesystem::path kTransitionVideoRelativePath =
            "Textures/UI/Transition screen/Transition screen.mpg";
        std::filesystem::path gCutsceneVideoPath;
        std::filesystem::path gFinalLevelCutsceneVideoPath;
        std::filesystem::path gTransitionVideoPath;
        std::filesystem::path gWinVideoPath;
        std::filesystem::path gPendingFinalLevelLoadPath;

        void StopAllGameplayAudioForFinalLevelCutscene()
        {
            SoundManager::getInstance().stopAllSounds();
            mainMenuBGMPlaying = false;
            gameplayBGMPlaying = false;
            cutsceneAudioPlaying = false;
            winVideoAudioPlaying = false;
            finalLevelCutsceneAudioPlaying = false;
            defeatSoundStarted = false;
            boilingStarted = false;
        }

        constexpr int START_KEY = GLFW_KEY_ENTER; // Keyboard stand-in for a controller Start button.
        constexpr int PAUSE_KEY = GLFW_KEY_ESCAPE;
        float stateAdvanceInputBlockTimer = 0.0f;
        float loadingTransitionElapsedTimer = 0.0f;
        constexpr float kStateAdvanceInputBlockDuration = 0.2f;

       
        void BlockStateAdvanceInput(float duration = kStateAdvanceInputBlockDuration)
        {
            stateAdvanceInputBlockTimer = std::max(stateAdvanceInputBlockTimer, duration);
            if (gInputSystem) {
                gInputSystem->Manager().ClearState();
            }
        }

        bool IsStateAdvanceInputPressed()
        {
            return stateAdvanceInputBlockTimer <= 0.0f &&
                gInputSystem &&
                (gInputSystem->IsKeyPressed(START_KEY) || gInputSystem->IsKeyPressed(PAUSE_KEY));
        }

        bool IsLoadingTransitionVideoPlaying()
        {
            return loadingTransitionVideoEnabled &&
                !loadingTransitionSkipRequested &&
                !transitionPlayer.IsFinished();
        }

        // Secret instructor cheats: type these words on the keyboard during gameplay.
        // `gort`, `gorl1`, `gorl2`, `gorl3`, and `gorll` jump to specific campaign levels.
        // `gonext` advances to the next campaign level, `goend` jumps to HeiBang's final level,
        // and `godcoming` toggles god mode on/off.
        bool godModeEnabled = false;
        std::string cheatInputBuffer;
        constexpr float kGodModePlayerDamage = 100.0f;
        constexpr std::size_t kMaxCheatBufferLength = 16;

        std::string ToLowerAscii(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        }

        bool ResolveLevelEntryCutsceneAssets(const std::filesystem::path& levelPath,
            std::string& outVideoPath,
            std::string& outAudioPath)
        {
            const std::string levelName = ToLowerAscii(levelPath.filename().string());
            if (levelName == "reallevel3.json")
            {
                outVideoPath = ResolveFirstExistingAsset({
                    "Video__OFF_WEB/Cutscene 02.mpg",
                    "Video/Cutscene 02.mpg"
                    });
                outAudioPath = ResolveFirstExistingAsset({
                    "Video__OFF_WEB/Cutscene 02.mp3",
                    "Video/Cutscene 02.mp3"
                    });
                return true;
            }

            if (levelName == "reallastlevel.json" || levelName == "reallastlevl.json")
            {
                outVideoPath = ResolveFirstExistingAsset({
                    "Video__OFF_WEB/Cutscene 03.mpg",
                    "Video/Cutscene 03.mpg"
                    });
                outAudioPath = ResolveFirstExistingAsset({
                    "Video__OFF_WEB/Cutscene 03.mp3",
                    "Video/Cutscene 03.mp3"
                    });
                return true;
            }

            outVideoPath.clear();
            outAudioPath.clear();
            return false;
        }

        void LogBossMusicDebug(const char* event,
            const char* selectedTrack = nullptr,
            bool forcePlay = false,
            bool attemptedPlay = false,
            bool playResult = false)
        {
            if (!g_crashLogger)
                return;

            SoundManager& sm = SoundManager::getInstance();
            const std::filesystem::path audioRoot = Framework::ResolveAssetPath("Audio__OFF_WEB");
            const std::filesystem::path miniPath = audioRoot / "MiniBoss_Loop.wav";
            const std::filesystem::path finalPath = audioRoot / "FinalBoss_Loop.wav";

            std::string levelName;
            if (gLogicSystem && gLogicSystem->Factory())
                levelName = gLogicSystem->Factory()->LastLevelPath().filename().string();

            std::error_code ec;
            std::string extra = "event=";
            extra += event ? event : "";
            extra += "|level=" + levelName;
            extra += "|selected=";
            extra += selectedTrack ? selectedTrack : "";
            extra += "|current=" + currentLevelMusic;
            extra += "|force=" + std::string(forcePlay ? "1" : "0");
            extra += "|attempted=" + std::string(attemptedPlay ? "1" : "0");
            extra += "|play_result=" + std::string(playResult ? "1" : "0");
            extra += "|mini_loaded=" + std::string(sm.isSoundLoaded(LEVEL3_BOSS_BGM) ? "1" : "0");
            extra += "|final_loaded=" + std::string(sm.isSoundLoaded(LEVEL4_BOSS_BGM) ? "1" : "0");
            extra += "|mini_playing=" + std::string(sm.isSoundPlaying(LEVEL3_BOSS_BGM) ? "1" : "0");
            extra += "|final_playing=" + std::string(sm.isSoundPlaying(LEVEL4_BOSS_BGM) ? "1" : "0");
            extra += "|audio_root=" + audioRoot.string();
            extra += "|audio_root_exists=" + std::string(std::filesystem::exists(audioRoot, ec) ? "1" : "0");
            ec.clear();
            extra += "|mini_file_exists=" + std::string(std::filesystem::exists(miniPath, ec) ? "1" : "0");
            ec.clear();
            extra += "|final_file_exists=" + std::string(std::filesystem::exists(finalPath, ec) ? "1" : "0");
            ec.clear();
            extra += "|cwd=" + std::filesystem::current_path(ec).string();

            g_crashLogger->Write("boss_music_debug", extra);
        }

        const char* GameStateName(GameState state)
        {
            switch (state)
            {
            case GameState::MAIN_MENU: return "MAIN_MENU";
            case GameState::CUTSCENE: return "CUTSCENE";
            case GameState::FINAL_LEVEL_CUTSCENE: return "FINAL_LEVEL_CUTSCENE";
            case GameState::TRANSITIONING: return "TRANSITIONING";
            case GameState::LOADING_TRANSITION: return "LOADING_TRANSITION";
            case GameState::PLAYING: return "PLAYING";
            case GameState::PAUSED: return "PAUSED";
            case GameState::DEFEAT: return "DEFEAT";
            case GameState::WIN_VIDEO: return "WIN_VIDEO";
            case GameState::EXIT: return "EXIT";
            default: return "UNKNOWN";
            }
        }

        void LogVideoDebug(const char* event,
            const std::filesystem::path& videoPath = {},
            bool exists = false,
            bool ready = false,
            const char* player = nullptr,
            const char* note = nullptr)
        {
            if (!g_crashLogger)
                return;

            std::error_code ec;
            std::string extra = "event=";
            extra += event ? event : "";
            extra += "|player=";
            extra += player ? player : "";
            extra += "|state=";
            extra += GameStateName(currentState);
            extra += "|path=" + videoPath.string();
            extra += "|exists=" + std::string(exists ? "1" : "0");
            extra += "|ready=" + std::string(ready ? "1" : "0");
            extra += "|transition_enabled=" + std::string(loadingTransitionVideoEnabled ? "1" : "0");
            extra += "|transition_skip=" + std::string(loadingTransitionSkipRequested ? "1" : "0");
            extra += "|note=";
            extra += note ? note : "";
            extra += "|cwd=" + std::filesystem::current_path(ec).string();

            g_crashLogger->Write("video_debug", extra);
        }

        /*********************************************************************************************
       \brief Plays the appropriate level BGM when a level is loaded.
       \details Only starts the music if it's not already playing. Fades out the previous track if needed.
      *********************************************************************************************/
        void OnLevelLoadedPlayMusic(bool forcePlay = false)
        {
            if (!gLogicSystem || !gLogicSystem->Factory())
                return;

            SoundManager& sm = SoundManager::getInstance();
            std::string levelName = ToLowerAscii(
                gLogicSystem->Factory()->LastLevelPath().filename().string());

            const char* nextTrack = GAMEPLAY_BGM;
            if (levelName == "reallevel3.json")
            {
                EnsureBossMusicLoaded();
                nextTrack = LEVEL3_BOSS_BGM;
            }
            else if (levelName == "reallastlevel.json" ||
                levelName == "reallastlevl.json")
            {
                EnsureBossMusicLoaded();
                nextTrack = LEVEL4_BOSS_BGM;
            }

            bool attemptedPlay = false;
            bool playResult = false;

            // Skip restarting if already playing the correct track
            if (!forcePlay && gameplayBGMPlaying && sm.isSoundPlaying(nextTrack))
            {
                LogBossMusicDebug("already_playing", nextTrack, forcePlay);
                return;
            }

            // Fade out old track if different
            if (!currentLevelMusic.empty() && sm.isSoundPlaying(currentLevelMusic.c_str()) && currentLevelMusic != nextTrack)
                sm.fadeOutMusic(currentLevelMusic.c_str(), kBGMFadeDuration);

            if (sm.isSoundLoaded(nextTrack) && !sm.isSoundPlaying(nextTrack))
            {
                attemptedPlay = true;
                playResult = sm.playSound(nextTrack, 1.0f, 1.0f, true);
                sm.setSoundVolume(nextTrack, 0.0f);
                sm.fadeInMusic(nextTrack, kBGMFadeDuration, 0.5f);
            }

            currentLevelMusic = nextTrack;
            gameplayBGMPlaying = true;
            LogBossMusicDebug("level_loaded_play_music", nextTrack, forcePlay, attemptedPlay, playResult);
        }

        bool BufferEndsWith(std::string_view suffix)
        {
            return cheatInputBuffer.size() >= suffix.size() &&
                std::equal(suffix.rbegin(), suffix.rend(), cheatInputBuffer.rbegin());
        }

        bool HasRemainingEnemiesForFlash()
        {
            if (!gLogicSystem || !gLogicSystem->Factory())
                return false;

            for (auto const& [id, ptr] : gLogicSystem->Factory()->Objects())
            {
                (void)id;
                auto* obj = ptr.get();
                if (!obj)
                    continue;

                auto* enemy = obj->GetComponentType<Framework::EnemyComponent>(
                    Framework::ComponentTypeId::CT_EnemyComponent);
                if (!enemy)
                    continue;

                auto* health = obj->GetComponentType<Framework::EnemyHealthComponent>(
                    Framework::ComponentTypeId::CT_EnemyHealthComponent);
                if (!health || health->enemyHealth > 0)
                    return true;
            }

            return false;
        }

        void RestoreFlashRenderState(Framework::RenderComponent& render, const Framework::FlashComponent& flash)
        {
            if (!flash.hasCachedRenderState)
                return;

            render.r = flash.cachedR;
            render.g = flash.cachedG;
            render.b = flash.cachedB;
            render.a = flash.cachedA;
            render.visible = flash.cachedVisible;
            render.blendMode = flash.cachedBlendMode;
        }

        void CacheFlashRenderState(Framework::FlashComponent& flash, const Framework::RenderComponent& render)
        {
            flash.cachedR = render.r;
            flash.cachedG = render.g;
            flash.cachedB = render.b;
            flash.cachedA = render.a;
            flash.cachedVisible = render.visible;
            flash.cachedBlendMode = render.blendMode;
            flash.hasCachedRenderState = true;
        }

        void UpdateEnemyClearFlashComponents(float dt)
        {
            if (!gLogicSystem || !gLogicSystem->Factory())
                return;

            const bool enemyCleared = !HasRemainingEnemiesForFlash();

            for (auto const& [id, ptr] : gLogicSystem->Factory()->Objects())
            {
                (void)id;
                auto* obj = ptr.get();
                if (!obj)
                    continue;

                auto* flash = obj->GetComponentType<Framework::FlashComponent>(
                    Framework::ComponentTypeId::CT_FlashComponent);
                auto* render = obj->GetComponentType<Framework::RenderComponent>(
                    Framework::ComponentTypeId::CT_RenderComponent);
                if (!flash || !render)
                    continue;

                const bool shouldFlash = flash->activate_on_enemy_clear && enemyCleared;
                if (!shouldFlash)
                {
                    if (!flash->hasCachedRenderState)
                        CacheFlashRenderState(*flash, *render);

                    RestoreFlashRenderState(*render, *flash);
                    flash->timer = 0.0f;
                    flash->visible = flash->cachedVisible;
                    flash->flashing = false;
                    flash->completed = false;
                    if (flash->hide_until_activated && flash->activate_on_enemy_clear)
                    {
                        flash->visible = false;
                        render->visible = false;
                    }
                    continue;
                }

                if (flash->completed)
                {
                    RestoreFlashRenderState(*render, *flash);
                    flash->visible = flash->start_visible;
                    continue;
                }

                if (!flash->flashing)
                {
                    flash->timer = 0.0f;
                    flash->flashing = true;
                    flash->completed = false;
                    if (!flash->hasCachedRenderState)
                        CacheFlashRenderState(*flash, *render);
                }

                flash->timer += std::max(0.0f, dt);
                if (flash->duration > 0.0f && flash->timer >= flash->duration)
                {
                    flash->completed = true;
                    flash->flashing = false;
                    flash->visible = flash->start_visible;
                    RestoreFlashRenderState(*render, *flash);
                    continue;
                }

                bool flashWhite =
                    std::fmod(flash->timer * std::max(0.0f, flash->frequency), 1.0f) < 0.5f;
                if (!flash->start_visible)
                    flashWhite = !flashWhite;

                flash->visible = flashWhite;
                if (flashWhite)
                {
                    render->visible = flash->hide_until_activated
                        ? true
                        : flash->cachedVisible;
                    render->r = 1.0f;
                    render->g = 1.0f;
                    render->b = 1.0f;
                    render->a = flash->cachedA;
                    render->blendMode = Framework::BlendMode::Add;
                }
                else
                {
                    render->visible = false;
                }
            }
        }

        void PushCheatCharacter(char c)
        {
            cheatInputBuffer.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            if (cheatInputBuffer.size() > kMaxCheatBufferLength)
            {
                cheatInputBuffer.erase(0, cheatInputBuffer.size() - kMaxCheatBufferLength);
            }
        }

        std::filesystem::path ResolveFirstExistingData(std::initializer_list<const char*> candidates)
        {
            if (!gLogicSystem)
                return {};

            std::error_code ec;
            for (const char* rel : candidates)
            {
                if (!rel || *rel == '\0')
                    continue;

                const auto resolved = gLogicSystem->ResolveDataPath(rel);
                if (std::filesystem::exists(resolved, ec) && std::filesystem::is_regular_file(resolved, ec))
                    return resolved;

                ec.clear();
            }

            return {};
        }

        std::filesystem::path ResolveNextCheatLevelPath()
        {
            if (!gLogicSystem || !gLogicSystem->Factory())
                return {};

            const std::string currentLevelName = ToLowerAscii(
                gLogicSystem->Factory()->LastLevelPath().filename().string());

            if (currentLevelName == "level_realtutorial.json" ||
                currentLevelName == "level_realtutorial2.json" ||
                currentLevelName == "level_realtutorial4.json")
            {
                return ResolveFirstExistingData({ "RealLevel1.json" });
            }
            if (currentLevelName == "reallevel1.json" ||
                currentLevelName == "reallevel1noenemy.json")
            {
                return ResolveFirstExistingData({ "RealLevel2.json" });
            }
            if (currentLevelName == "reallevel2.json")
                return ResolveFirstExistingData({ "RealLevel3.json" });
            if (currentLevelName == "reallevel3.json")
                return ResolveFirstExistingData({ "RealLastLevel.json", "RealLastLevl.json" });

            return {};
        }

        bool StartCheatLevelLoad(std::string_view cheatName, const std::filesystem::path& levelPath)
        {
            if (levelPath.empty())
                return false;

            if (!RequestLoadLevel(levelPath))
                return false;

            ResetPlayerDefeat();
            ResetHeiBangDefeat();
            ResetPlayerKeyCount();
            cheatInputBuffer.clear();
            std::cout << "[Cheat] " << cheatName << " -> loading "
                << levelPath.filename().string() << "\n";
            BlockStateAdvanceInput(0.1f);
            return true;
        }

        void CollectCheatCharacters()
        {
            if (!gInputSystem)
                return;

            for (int key = GLFW_KEY_A; key <= GLFW_KEY_Z; ++key)
            {
                if (gInputSystem->IsKeyPressed(key))
                    PushCheatCharacter(static_cast<char>('a' + (key - GLFW_KEY_A)));
            }

            for (int key = GLFW_KEY_0; key <= GLFW_KEY_9; ++key)
            {
                if (gInputSystem->IsKeyPressed(key))
                    PushCheatCharacter(static_cast<char>('0' + (key - GLFW_KEY_0)));
            }

            if (gInputSystem->IsKeyPressed(GLFW_KEY_BACKSPACE) && !cheatInputBuffer.empty())
                cheatInputBuffer.pop_back();
        }

        bool HandleCheatCodeInput()
        {
            if (!gInputSystem || Framework::RenderSystem::IsEditorVisible())
            {
                cheatInputBuffer.clear();
                return false;
            }

            const bool allowCheats =
                currentState == GameState::PLAYING ||
                currentState == GameState::PAUSED ||
                currentState == GameState::DEFEAT;
            if (!allowCheats)
            {
                cheatInputBuffer.clear();
                return false;
            }

            CollectCheatCharacters();
            if (cheatInputBuffer.empty())
                return false;

            if (BufferEndsWith("godcoming"))
            {
                godModeEnabled = !godModeEnabled;
                cheatInputBuffer.clear();
                std::cout << "[Cheat] godcoming -> god mode "
                    << (godModeEnabled ? "ENABLED" : "DISABLED") << "\n";
                BlockStateAdvanceInput(0.1f);
                return false;
            }

            if (BufferEndsWith("gort"))
            {
                if (StartCheatLevelLoad("gort",
                    ResolveFirstExistingData({ "level_RealTutorial.json" })))
                {
                    return true;
                }

                cheatInputBuffer.clear();
                std::cout << "[Cheat] gort -> tutorial level file not found\n";
                return false;
            }

            if (BufferEndsWith("gorl1"))
            {
                if (StartCheatLevelLoad("gorl1",
                    ResolveFirstExistingData({ "RealLevel1.json" })))
                {
                    return true;
                }

                cheatInputBuffer.clear();
                std::cout << "[Cheat] gorl1 -> RealLevel1 file not found\n";
                return false;
            }

            if (BufferEndsWith("gorl2"))
            {
                if (StartCheatLevelLoad("gorl2",
                    ResolveFirstExistingData({ "RealLevel2.json" })))
                {
                    return true;
                }

                cheatInputBuffer.clear();
                std::cout << "[Cheat] gorl2 -> RealLevel2 file not found\n";
                return false;
            }

            if (BufferEndsWith("gorl3"))
            {
                if (StartCheatLevelLoad("gorl3",
                    ResolveFirstExistingData({ "RealLevel3.json" })))
                {
                    return true;
                }

                cheatInputBuffer.clear();
                std::cout << "[Cheat] gorl3 -> RealLevel3 file not found\n";
                return false;
            }

            if (BufferEndsWith("gorll"))
            {
                if (StartCheatLevelLoad("gorll",
                    ResolveFirstExistingData({ "RealLastLevl.json", "RealLastLevel.json" })))
                {
                    return true;
                }

                cheatInputBuffer.clear();
                std::cout << "[Cheat] gorll -> final boss level file not found\n";
                return false;
            }

            if (BufferEndsWith("goend"))
            {
                if (StartCheatLevelLoad("goend",
                    ResolveFirstExistingData({ "RealLastLevel.json", "RealLastLevl.json" })))
                {
                    return true;
                }

                cheatInputBuffer.clear();
                std::cout << "[Cheat] goend -> final level file not found\n";
                return false;
            }

            if (BufferEndsWith("gonext"))
            {
                const auto nextLevel = ResolveNextCheatLevelPath();
                if (StartCheatLevelLoad("gonext", nextLevel))
                    return true;

                cheatInputBuffer.clear();
                std::cout << "[Cheat] gonext -> no next campaign level mapped from this stage\n";
            }

            return false;
        }

        /*************************************************************************************
         \brief  Receives allocator leak records during shutdown diagnostics.
         \param  block       Address of the still-live allocation.
         \param  blockIndex  Allocator block index for the leaked allocation.
        *************************************************************************************/
        void allocatorDumpCallback(const void* block, unsigned int blockIndex)
        {
            std::cout << "[Allocator] Leak: block #" << blockIndex << " at " << block << "\n";
        }

        /*************************************************************************************
         \brief  Freezes gameplay actors that must not advance during the loading transition.
         \details
                 - Enemy velocities and knockback state are cleared every frame so AI/pathing
                   cannot produce visible movement while the loader is active.
                 - Player movement velocity is also zeroed, but the player object still goes
                   through logic/camera updates so idle animation and follow-camera behavior remain live.
        *************************************************************************************/
        void FreezeLoadingTransitionActors()
        {
            if (!Framework::FACTORY)
                return;

            for (const auto& [id, obj] : Framework::FACTORY->Objects())
            {
                (void)id;
                if (!obj)
                    continue;

                auto* body = obj->GetComponentType<Framework::RigidBodyComponent>(
                    Framework::ComponentTypeId::CT_RigidBodyComponent);
                if (!body)
                    continue;

                if (obj->GetComponentType<Framework::EnemyComponent>(mygame::CT_EnemyComponent()))
                {
                    body->velX = 0.0f;
                    body->velY = 0.0f;
                    body->knockVelX = 0.0f;
                    body->knockVelY = 0.0f;
                    body->knockbackTime = 0.0f;
                }
                else if (obj->GetComponentType<Framework::PlayerComponent>(mygame::CT_PlayerComponent()))
                {
                    body->velX = 0.0f;
                    body->velY = 0.0f;
                }
            }
        }

        bool StartLevelEntryCutscene(const std::filesystem::path& levelPath)
        {
            if (levelPath.empty())
                return false;

            std::string cutsceneVideoPath;
            std::string cutsceneAudioPath;
            if (!ResolveLevelEntryCutsceneAssets(levelPath, cutsceneVideoPath, cutsceneAudioPath))
                return false;

            gFinalLevelCutsceneVideoPath = cutsceneVideoPath;
            finalLevelCutsceneReady = !cutsceneVideoPath.empty() &&
                finalLevelCutscenePlayer.Load(cutsceneVideoPath);
            if (!finalLevelCutsceneReady)
            {
                std::cerr << "[Cutscene] Warning: Could not load level-entry cutscene video for "
                    << levelPath.filename().string() << "\n";
                return false;
            }

            SoundManager& sm = SoundManager::getInstance();
            if (sm.isSoundLoaded(FINAL_LEVEL_CUTSCENE_AUDIO))
            {
                sm.stopSound(FINAL_LEVEL_CUTSCENE_AUDIO);
                sm.unloadSound(FINAL_LEVEL_CUTSCENE_AUDIO);
            }

            const bool levelEntryCutsceneAudioReady =
                !cutsceneAudioPath.empty() &&
                sm.loadSound(FINAL_LEVEL_CUTSCENE_AUDIO, cutsceneAudioPath);
            if (!levelEntryCutsceneAudioReady)
            {
                std::cerr << "[Cutscene] Warning: Could not load level-entry cutscene audio for "
                    << levelPath.filename().string() << "\n";
            }

            gPendingFinalLevelLoadPath = levelPath;
            StopAllGameplayAudioForFinalLevelCutscene();
            if (gLogicSystem) {
                gLogicSystem->PrepareForIncrementalLevelLoad();
            }
            finalLevelCutscenePlayer.Start();
            LogVideoDebug("final_level_cutscene_start",
                gFinalLevelCutsceneVideoPath,
                !gFinalLevelCutsceneVideoPath.empty() && std::filesystem::exists(gFinalLevelCutsceneVideoPath),
                finalLevelCutsceneReady,
                "final_level_cutscene",
                levelPath.filename().string().c_str());
            if (levelEntryCutsceneAudioReady && sm.isSoundLoaded(FINAL_LEVEL_CUTSCENE_AUDIO)) {
                sm.stopSound(FINAL_LEVEL_CUTSCENE_AUDIO);
                sm.playSound(FINAL_LEVEL_CUTSCENE_AUDIO, 1.0f, 1.0f, false);
                finalLevelCutsceneAudioPlaying = true;
            }
            else {
                finalLevelCutsceneAudioPlaying = false;
            }

            editorSimulationRunning = false;
            currentState = GameState::FINAL_LEVEL_CUTSCENE;
            BlockStateAdvanceInput();
            return true;
        }

        /*************************************************************************************
         \brief  Enters the staged level-loading state machine and optionally starts the transition video.
         \param  levelPath                 Level file to load.
         \param  resumeSimulationAfterLoad Whether gameplay simulation should resume when loading finishes.
         \param  nextStateAfterLoad        State to switch to once both loading and the video are done.
         \param  playVideo                 True to show the transition video; false for editor-triggered loads.
         \return True if the staged load was successfully started.
        *************************************************************************************/
        bool StartGameplayLoadTransition(const std::filesystem::path& levelPath,
            bool resumeSimulationAfterLoad,
            GameState nextStateAfterLoad = GameState::PLAYING,
            bool playVideo = true,
            bool hideGameplayUntilDelay = false)
        {
            
            if (!gLogicSystem || levelLoader.IsActive())
                return false;

            loadingTransitionVideoEnabled = playVideo && transitionReady;
            loadingTransitionRequiresLevelLoad =
                !(nextStateAfterLoad == GameState::MAIN_MENU && loadingTransitionVideoEnabled);
            if (loadingTransitionVideoEnabled) {
                transitionPlayer.Start();
                LogVideoDebug("transition_start",
                    gTransitionVideoPath,
                    !gTransitionVideoPath.empty() && std::filesystem::exists(gTransitionVideoPath),
                    transitionReady,
                    "transition",
                    "StartGameplayLoadTransition");
            }
            else
            {
                LogVideoDebug("transition_skipped",
                    gTransitionVideoPath,
                    !gTransitionVideoPath.empty() && std::filesystem::exists(gTransitionVideoPath),
                    transitionReady,
                    "transition",
                    playVideo ? "transition_not_ready" : "playVideo_false");
            }

            if (loadingTransitionRequiresLevelLoad)
            {
                if (levelPath.empty() || !levelLoader.BeginLoad(*gLogicSystem, levelPath))
                {
                    std::cerr << "[LoadingTransition] Failed to begin load: " << levelPath << "\n";
                    return false;
                }
            }
            else
            {
                levelLoader.Reset();
            }

            loadingTransitionSkipRequested = !loadingTransitionVideoEnabled;
            loadingTransitionResumeSimulation = resumeSimulationAfterLoad;
            loadingTransitionNextState = nextStateAfterLoad;
            loadingTransitionHideGameplayUntilDelay = hideGameplayUntilDelay;
            loadingTransitionElapsedTimer = 0.0f;
            ResetHeiBangDefeat();
            editorSimulationRunning = false;
            levelMusicInitialized = false;
            currentState = GameState::LOADING_TRANSITION;
            BlockStateAdvanceInput();
            return true;
        }

    }

    // Transition timing helpers (file-local).
    static float transitionTimer = 0.0f;
    static constexpr float kStartTransitionDuration = 1.0f;

    /*************************************************************************************
     \brief  Initializes engine systems and game-side bindings for the current session.
     \param  win  The main application window used by window-dependent systems.
     \details
             - Creates and initializes the shared engine systems through SystemManager.
             - Creates the game-owned HitBoxSystem after LogicSystem is initialized so
               combat behavior remains on the game side.
             - Binds BloodyGoodCurry scripts, combat audio, combat VFX, and UI flow.
    *************************************************************************************/
    void init(gfx::Window& win)
    {
        gInputSystem = gSystems.RegisterSystem<Framework::InputSystem>(win);
        gLogicSystem = gSystems.RegisterSystem<Framework::LogicSystem>(win, *gInputSystem);
        ConfigureGameBootstrap(*gLogicSystem);
        gPhysicsSystem = gSystems.RegisterSystem<Framework::PhysicSystem>();
        gAiSystem = gSystems.RegisterSystem<Framework::AiSystem>(win);
        gAudioSystem = gSystems.RegisterSystem<Framework::AudioSystem>(win);
        gRenderSystem = gSystems.RegisterSystem<Framework::RenderSystem>(win, *gLogicSystem);
        ConfigureRenderBootstrap(*gRenderSystem);
        gHealthSystem = gSystems.RegisterSystem<Framework::HealthSystem>(win);
        gParticleSystem = gSystems.RegisterSystem<Framework::ParticleSystem>();
        gZoomTriggerSystem = gSystems.RegisterSystem<Framework::ZoomTriggerSystem>();

        //(void)gPhysicsSystem;
        //(void)gAudioSystem;
        //(void)gRenderSystem;

        gSystems.IntializeAll();
        if (gAudioSystem)
        {
            gAudioSystem->SetListenerQueryCallback([]() -> Framework::GOC*
            {
                return gLogicSystem ? gLogicSystem->FindAnyAlivePlayer() : nullptr;
            });
        }
        if (gLogicSystem && !gLogicSystem->hitBoxSystem)
        {
            // HitBoxSystem remains a shared runtime service, but this game now owns its lifetime.
            gLogicSystem->hitBoxSystem = new Framework::HitBoxSystem(*gLogicSystem);
            gLogicSystem->hitBoxSystem->Initialize();
        }
        if (gLogicSystem)
        {
            // Keep gameplay-owned runtime updates synchronized with the end of LogicSystem::Update().
            gLogicSystem->SetPostUpdateCallback([](float dt)
            {
                if (gLogicSystem && gLogicSystem->hitBoxSystem)
                    gLogicSystem->hitBoxSystem->Update(dt);
                UpdateEnemyClearFlashComponents(dt);
            });
        }
        RegisterMyGameScripts(*gLogicSystem);
        BindCombatAudio(*gLogicSystem, *gHealthSystem);
        BindCombatVfx(*gLogicSystem);
        BindAiCombat(*gAiSystem, *gLogicSystem);
        BindHealthPresentation(*gHealthSystem);
        EnsureBossMusicLoaded();
        LogBossMusicDebug("init_after_audio");
        mainMenu.Init(gRenderSystem->ScreenWidth(), gRenderSystem->ScreenHeight());
        pauseMenu.Init(gRenderSystem->ScreenWidth(), gRenderSystem->ScreenHeight());
        defeatScreen.Init(gRenderSystem->ScreenWidth(), gRenderSystem->ScreenHeight());
        const std::string cutscenePath = ResolveFirstExistingAsset({
            "Video__OFF_WEB/Cutscene 01.mpg",
            "Video/Cutscene 01.mpg"
            });
        gCutsceneVideoPath = cutscenePath;
        cutsceneReady = !cutscenePath.empty() && cutscenePlayer.Load(cutscenePath);
        LogVideoDebug("cutscene_resolve",
            gCutsceneVideoPath,
            !gCutsceneVideoPath.empty() && std::filesystem::exists(gCutsceneVideoPath),
            cutsceneReady,
            "cutscene");
        if (!cutsceneReady) {
            std::cerr << "[Cutscene] Warning: Could not load Cutscene 01.mpg from Video/ or Video__OFF_WEB/.\n";
        }
        gTransitionVideoPath = Framework::ResolveAssetPath(kTransitionVideoRelativePath);
        transitionReady = std::filesystem::exists(gTransitionVideoPath) &&
            transitionPlayer.Load(gTransitionVideoPath.string());
        LogVideoDebug("transition_resolve",
            gTransitionVideoPath,
            std::filesystem::exists(gTransitionVideoPath),
            transitionReady,
            "transition");
        if (transitionReady) {
            transitionPlayer.SetColorKeyEnabled(true, kTransitionColorKeyLow, kTransitionColorKeyHigh);
        }
        else {
            std::cerr << "[LoadingTransition] Warning: Could not load "
                << gTransitionVideoPath.string() << "\n";
        }
        const std::string winVideoPath = ResolveFirstExistingAsset({
            "Video__OFF_WEB/Cutscene 04.mpg",
            "Textures/UI/GameWinVideo/WinGame_plmpeg.mpg"
            });
        gWinVideoPath = winVideoPath;
        winVideoReady = !winVideoPath.empty() &&
            winVideoPlayer.Load(winVideoPath);
        LogVideoDebug("win_video_resolve",
            gWinVideoPath,
            std::filesystem::exists(gWinVideoPath),
            winVideoReady,
            "win");
        if (!winVideoReady) {
            std::cerr << "[WinVideo] Warning: Could not load "
                << gWinVideoPath.string() << "\n";
        }
        if (!SoundManager::getInstance().isSoundLoaded(WIN_VIDEO_AUDIO)) {
            const std::string winVideoAudioPath = ResolveFirstExistingAsset({
                "Video__OFF_WEB/Cutscene04.mp3",
                "Textures/UI/GameWinVideo/Cutscene04.mp3"
                });
            if (winVideoAudioPath.empty() ||
                !SoundManager::getInstance().loadSound(WIN_VIDEO_AUDIO, winVideoAudioPath)) {
                std::cerr << "[WinVideo] Warning: Could not load "
                    << (winVideoAudioPath.empty() ? "Cutscene04.mp3 from Video__OFF_WEB/ or Textures/UI/GameWinVideo/" : winVideoAudioPath)
                    << "\n";
            }
        }
        if (!SoundManager::getInstance().isSoundLoaded(CUTSCENE_AUDIO)) {
            const std::string cutsceneAudioPath = ResolveFirstExistingAsset({
                "Video__OFF_WEB/Cutscene 01.mp3",
                "Video/Cutscene 01.mp3"
                });
            if (cutsceneAudioPath.empty() ||
                !SoundManager::getInstance().loadSound(CUTSCENE_AUDIO, cutsceneAudioPath)) {
                std::cerr << "[Cutscene] Warning: Could not load Cutscene 01.mp3 from Video/ or Video__OFF_WEB/.\n";
            }
        }
        currentState = GameState::MAIN_MENU;

        editorSimulationRunning = false;
    }

    /*************************************************************************************
     \brief  Advances the active game state for one simulation tick.
     \param  dt  Fixed-step delta time in seconds.
     \details Handles state transitions, menu flow, cutscene playback, editor simulation,
              global audio volume updates, and system updates during gameplay.
    *************************************************************************************/
    void update(float dt)
    {
        TryGuard::Run([&] {
            stateAdvanceInputBlockTimer = std::max(0.0f, stateAdvanceInputBlockTimer - dt);
            SoundManager::getInstance().update(dt);
            const bool editorMode = Framework::RenderSystem::IsEditorVisible();
            const bool systemsUpdating = (currentState == GameState::PLAYING && editorSimulationRunning);
            if (!systemsUpdating && gInputSystem) {
                gInputSystem->Update(dt);
            }
            float bgmVolume = pauseMenu.GetBgmVolume();
            float sfxVolume = pauseMenu.GetSfxVolume();
            const auto& pauseOptions = pauseMenu.GetOptionsValues();
            const auto& mainOptions = mainMenu.GetOptionsValues();
            if (currentState == GameState::MAIN_MENU ||
                currentState == GameState::TRANSITIONING ||
                currentState == GameState::CUTSCENE ||
                (currentState == GameState::LOADING_TRANSITION &&
                    loadingTransitionNextState == GameState::MAIN_MENU)) {
                bgmVolume = mainMenu.GetBgmVolume();
                sfxVolume = mainMenu.GetSfxVolume();
                pauseMenu.SetOptionsValues(mainOptions);
            }
            else {
                mainMenu.SetOptionsValues(pauseOptions);
            }
            ApplyBgmVolume(bgmVolume);
            ApplySfxVolume(sfxVolume);
            auto handlePerfToggle = []() {
                static bool prevTogglePerf = false;
                const bool togglePerf = gInputSystem && gInputSystem->IsKeyPressed(GLFW_KEY_F1);
                if (togglePerf && !prevTogglePerf) {
                    Framework::ToggleVisible();
                }
                prevTogglePerf = togglePerf;
                };
            switch (currentState)
            {
            case GameState::MAIN_MENU:
                mainMenu.Update(gInputSystem);
                handlePerfToggle();
                if ((!mainMenuBGMPlaying || !SoundManager::getInstance().isSoundPlaying(MAIN_MENU_BGM)) &&
                    SoundManager::getInstance().isSoundLoaded(MAIN_MENU_BGM)) {
                    PlayMainMenuMusic();
                }
                if (mainMenu.ConsumeStart())
                {
                    if (SoundManager::getInstance().isSoundLoaded(START_BUTTTON))
                        SoundManager::getInstance().playSound(START_BUTTTON);
                    FadeOutMainMenuMusic();
                    BlockStateAdvanceInput();
                    if (cutsceneReady) {
                        cutscenePlayer.Start();
                        LogVideoDebug("cutscene_start",
                            gCutsceneVideoPath,
                            !gCutsceneVideoPath.empty() && std::filesystem::exists(gCutsceneVideoPath),
                            cutsceneReady,
                            "cutscene",
                            "main_menu_start");
                        if (SoundManager::getInstance().isSoundLoaded(CUTSCENE_AUDIO)) {
                            SoundManager::getInstance().stopSound(CUTSCENE_AUDIO);
                            SoundManager::getInstance().playSound(CUTSCENE_AUDIO, 1.0f, 1.0f, false);
                            cutsceneAudioPlaying = true;
                        }
                        currentState = GameState::CUTSCENE;
                    }
                    else {
                        LogVideoDebug("cutscene_unavailable_fallback",
                            gCutsceneVideoPath,
                            !gCutsceneVideoPath.empty() && std::filesystem::exists(gCutsceneVideoPath),
                            cutsceneReady,
                            "cutscene",
                            "falling_back_to_transition");
                        if (!RequestReloadLevel(true))
                        {
                            currentState = GameState::TRANSITIONING;
                            transitionTimer = kStartTransitionDuration;
                        }
                    }
                    editorSimulationRunning = false;
                    pauseMenu.ResetLatches();
                    ResetPlayerDefeat();
                    ResetHeiBangDefeat();
                    ResetPlayerKeyCount();
                }
                if (mainMenu.ConsumeExit())
                {
                    currentState = GameState::EXIT;
                }
                break;

            case GameState::TRANSITIONING:
                transitionTimer -= dt;
                if (transitionTimer <= 0.0f)
                {
                    currentState = GameState::PLAYING;
                    editorSimulationRunning = true;
                    BlockStateAdvanceInput();
                }
                break;

            case GameState::CUTSCENE: {
                cutscenePlayer.Update(dt);
                handlePerfToggle();
                const bool skipCutscene = IsStateAdvanceInputPressed();
                if (skipCutscene || cutscenePlayer.IsFinished())
                {
                    LogVideoDebug(skipCutscene ? "cutscene_skip" : "cutscene_finish",
                        gCutsceneVideoPath,
                        !gCutsceneVideoPath.empty() && std::filesystem::exists(gCutsceneVideoPath),
                        cutsceneReady,
                        "cutscene");
                    if (cutsceneAudioPlaying && SoundManager::getInstance().isSoundLoaded(CUTSCENE_AUDIO)) {
                        SoundManager::getInstance().stopSound(CUTSCENE_AUDIO);
                    }
                    cutsceneAudioPlaying = false;
                    if (!RequestReloadLevel(true))
                    {
                        currentState = GameState::PLAYING;
                        editorSimulationRunning = true;
                        BlockStateAdvanceInput();
                    }
                }
                break;
            }

            case GameState::FINAL_LEVEL_CUTSCENE:
            {
                finalLevelCutscenePlayer.Update(dt);
                handlePerfToggle();
                const bool skipCutscene = IsStateAdvanceInputPressed();
                if (skipCutscene || finalLevelCutscenePlayer.IsFinished())
                {
                    LogVideoDebug(skipCutscene ? "final_level_cutscene_skip" : "final_level_cutscene_finish",
                        gFinalLevelCutsceneVideoPath,
                        !gFinalLevelCutsceneVideoPath.empty() && std::filesystem::exists(gFinalLevelCutsceneVideoPath),
                        finalLevelCutsceneReady,
                        "final_level_cutscene",
                        gPendingFinalLevelLoadPath.filename().string().c_str());
                    if (finalLevelCutsceneAudioPlaying &&
                        SoundManager::getInstance().isSoundLoaded(FINAL_LEVEL_CUTSCENE_AUDIO)) {
                        SoundManager::getInstance().stopSound(FINAL_LEVEL_CUTSCENE_AUDIO);
                    }
                    finalLevelCutsceneAudioPlaying = false;
                    const std::filesystem::path pendingLevelPath = gPendingFinalLevelLoadPath;
                    gPendingFinalLevelLoadPath.clear();
                    if (!pendingLevelPath.empty() &&
                        StartGameplayLoadTransition(pendingLevelPath, true, GameState::PLAYING, true, false))
                    {
                        BlockStateAdvanceInput();
                    }
                    else
                    {
                        currentState = GameState::PLAYING;
                        editorSimulationRunning = true;
                        BlockStateAdvanceInput();
                    }
                }
                break;
            }

            case GameState::LOADING_TRANSITION:
            {
                const bool transitionVideoPlaying = IsLoadingTransitionVideoPlaying();
                if (transitionVideoPlaying)
                {
                    // Keep the transition screen fully non-interactive while its video is on-screen.
                    BlockStateAdvanceInput();
                }
                else
                {
                    handlePerfToggle();
                }

                if (loadingTransitionVideoEnabled)
                    loadingTransitionElapsedTimer += dt;

                const bool minimumVideoDelayElapsed =
                    !loadingTransitionVideoEnabled ||
                    loadingTransitionElapsedTimer >= kLoadingTransitionMinimumVideoTime;

                if (loadingTransitionRequiresLevelLoad && minimumVideoDelayElapsed)
                {
                    levelLoader.TickLoadStep(kLoadingObjectsPerTick);
                }

                const bool showMainMenuDuringTransition =
                    loadingTransitionNextState == GameState::MAIN_MENU &&
                    minimumVideoDelayElapsed;

                if (showMainMenuDuringTransition)
                {
                    mainMenu.UpdatePassive();
                }
                else if (!loadingTransitionHideGameplayUntilDelay || minimumVideoDelayElapsed)
                {
                    UpdateHealthPresentationDelta(dt);
                    if (gLogicSystem)
                        gLogicSystem->Update(dt);
                    FreezeLoadingTransitionActors();
                    if (gPhysicsSystem)
                        gPhysicsSystem->Update(dt);
                    if (gHealthSystem)
                        gHealthSystem->Update(dt);
                    if (gParticleSystem)
                        gParticleSystem->Update(dt);
                    if (gZoomTriggerSystem)
                        gZoomTriggerSystem->Update(dt);
                }

                if (loadingTransitionVideoEnabled &&
                    !loadingTransitionSkipRequested &&
                    !transitionPlayer.IsFinished()) {
                    transitionPlayer.Update(dt);
                }

                const bool loadingFinished =
                    !loadingTransitionRequiresLevelLoad ||
                    (levelLoader.IsDone() && levelLoader.Succeeded());
                const bool transitionFinished =
                    !loadingTransitionVideoEnabled ||
                    transitionPlayer.IsFinished();

                // Start the next level's music as soon as the level data is ready instead of
                // waiting for the transition video to finish. This avoids a noticeable delay
                // before BGM starts on heavier loads such as level_RealTutorial.json.
                if (minimumVideoDelayElapsed &&
                    loadingFinished &&
                    !levelMusicInitialized &&
                    loadingTransitionNextState != GameState::MAIN_MENU)
                {
                    OnLevelLoadedPlayMusic();
                    levelMusicInitialized = true;
                }

                if (minimumVideoDelayElapsed && loadingFinished && transitionFinished)
                {
                    currentState = loadingTransitionNextState;
                    editorSimulationRunning = loadingTransitionResumeSimulation;
                    BlockStateAdvanceInput();
                }
                break;
            }

            case GameState::PLAYING:
                if (editorSimulationRunning)
                {
                    UpdateHealthPresentationDelta(dt);
                    gSystems.UpdateAll(dt);
                    if (currentState != GameState::PLAYING)
                        break;
                }
                // When simulation is not running we already refreshed input above.
                handlePerfToggle();
                if (HandleCheatCodeInput())
                    break;

                if (!SoundManager::getInstance().isSoundPlaying(GAMEPLAY_BGM) &&
                    !levelMusicInitialized &&
                    SoundManager::getInstance().isSoundLoaded(GAMEPLAY_BGM))
                {
                    SoundManager::getInstance().playSound(GAMEPLAY_BGM, 1.0f, 1.0f, true);
                    SoundManager::getInstance().setSoundVolume(GAMEPLAY_BGM, 0.0f);
                    SoundManager::getInstance().fadeInMusic(GAMEPLAY_BGM, kBGMFadeDuration, 0.4f);
                    gameplayBGMPlaying = true;
                }
     
                if (!editorMode && IsPlayerDefeated())
                {
                    defeatScreen.ResetLatches();
                    if (gRenderSystem)
                        defeatScreen.SyncLayout(gRenderSystem->ScreenWidth(), gRenderSystem->ScreenHeight());
                    editorSimulationRunning = false;
                    currentState = GameState::DEFEAT;
                    break;
                }
                if (!editorMode && IsNancieDefeated())
                {
                    if (!miniBossMusicFading &&
                        SoundManager::getInstance().isSoundLoaded(LEVEL3_BOSS_BGM) &&
                        SoundManager::getInstance().isSoundPlaying(LEVEL3_BOSS_BGM))
                    {
                        SoundManager::getInstance().fadeOutMusic(LEVEL3_BOSS_BGM, kBGMFadeDuration);

                        // Fade in regular gameplay BGM only if not already playing
                        if (!SoundManager::getInstance().isSoundPlaying(GAMEPLAY_BGM) &&
                            SoundManager::getInstance().isSoundLoaded(GAMEPLAY_BGM))
                        {
                            SoundManager::getInstance().playSound(GAMEPLAY_BGM, 1.0f, 1.0f, true);
                            SoundManager::getInstance().setSoundVolume(GAMEPLAY_BGM, 0.0f);
                            SoundManager::getInstance().fadeInMusic(GAMEPLAY_BGM, kBGMFadeDuration, 0.4f);
                        }

                        currentLevelMusic = GAMEPLAY_BGM;
                        miniBossMusicFading = true;
                    }
                }
                if (!editorMode && IsHeiBangDefeated())
                {
                    // Fade out final boss music
                    if (SoundManager::getInstance().isSoundLoaded(LEVEL4_BOSS_BGM) &&
                        SoundManager::getInstance().isSoundPlaying(LEVEL4_BOSS_BGM))
                    {
                        SoundManager::getInstance().fadeOutMusic(LEVEL4_BOSS_BGM, kBGMFadeDuration);
                    }
                    if (gameplayBGMPlaying && SoundManager::getInstance().isSoundLoaded(GAMEPLAY_BGM))
                    {
                        SoundManager::getInstance().fadeOutMusic(GAMEPLAY_BGM, kBGMFadeDuration);
                        gameplayBGMPlaying = false;
                    }
                    editorSimulationRunning = false;
                    if (winVideoReady)
                    {
                        winVideoPlayer.Start();
                        if (SoundManager::getInstance().isSoundLoaded(WIN_VIDEO_AUDIO)) {
                            SoundManager::getInstance().stopSound(WIN_VIDEO_AUDIO);
                            SoundManager::getInstance().playSound(WIN_VIDEO_AUDIO, 1.0f, 1.0f, false);
                            winVideoAudioPlaying = true;
                        }
                        currentState = GameState::WIN_VIDEO;
                        BlockStateAdvanceInput();
                        break;
                    }

                    if (gLogicSystem &&
                        StartGameplayLoadTransition(gLogicSystem->Factory()->LastLevelPath().empty()
                            ? gLogicSystem->ResolveDataPath("level.json")
                            : gLogicSystem->Factory()->LastLevelPath(),
                            false,
                            GameState::MAIN_MENU,
                            false))
                    {
                        ResetPlayerDefeat();
                        ResetPlayerKeyCount();
                        break;
                    }

                    ResetPlayerDefeat();
                    ResetHeiBangDefeat();
                    ResetPlayerKeyCount();
                    currentState = GameState::MAIN_MENU;
                    BlockStateAdvanceInput();
                    break;
                }
                if (!editorMode && IsStateAdvanceInputPressed())
                {
                    pauseMenu.ResetLatches();
                    currentState = GameState::PAUSED;
                    BlockStateAdvanceInput();
                }
                break;

            case GameState::PAUSED:
                if (editorMode)
                {
                    currentState = GameState::PLAYING;
                    break;
                }
                pauseMenu.Update(gInputSystem);
                handlePerfToggle();
                if (HandleCheatCodeInput())
                    break;
                if (pauseMenu.ConsumeResume() ||
                    IsStateAdvanceInputPressed())
                {
                    // [UPDATED LOGIC] Resume based on previous state if possible, 
                    // or default to PLAYING. If we came from DEFEAT, going back to PLAYING
                    // might be weird if the player is still dead, but typically "Resume" means "Back to Game".
                    // If the player is dead, the next frame's check in PLAYING will send them back to DEFEAT screen.
                    currentState = GameState::PLAYING;
                    BlockStateAdvanceInput();
                    break;
                }

                if (pauseMenu.ConsumeMainMenu())
                {
                    const std::array<const char*, 3> allTracks = {
                        GAMEPLAY_BGM,
                        LEVEL3_BOSS_BGM,
                        LEVEL4_BOSS_BGM
                    };
                    for (const char* track : allTracks)
                    {
                        if (SoundManager::getInstance().isSoundLoaded(track) &&
                            SoundManager::getInstance().isSoundPlaying(track))
                        {
                            SoundManager::getInstance().fadeOutMusic(track, kBGMFadeDuration);
                        }
                    }
                    gameplayBGMPlaying = false;
                    levelMusicInitialized = false;
                    PlayMainMenuMusic(0.4f);
                    if (gLogicSystem &&
                        StartGameplayLoadTransition(gLogicSystem->Factory()->LastLevelPath().empty()
                            ? gLogicSystem->ResolveDataPath("level.json")
                            : gLogicSystem->Factory()->LastLevelPath(),
                            false,
                            GameState::MAIN_MENU))
                    {
                        ResetPlayerDefeat();
                        ResetHeiBangDefeat();
                        miniBossMusicFading = false;
                        ResetNancieDefeat();
                        ResetPlayerKeyCount();
                        break;
                    }
                    ResetPlayerDefeat();
                    ResetHeiBangDefeat();
                    miniBossMusicFading = false;
                    ResetNancieDefeat();
                    ResetPlayerKeyCount();
                    editorSimulationRunning = false;
                    currentState = GameState::MAIN_MENU;
                    break;
                }

                if (pauseMenu.ConsumeExitConfirmed())
                {
                    currentState = GameState::EXIT;
                    break;
                }

                if (pauseMenu.ConsumeQuitRequest())
                {
                    pauseMenu.ShowExitPopup();
                }
                break;

            case GameState::DEFEAT:
                defeatScreen.Update(gInputSystem);
                handlePerfToggle();
                if (HandleCheatCodeInput())
                    break;

                if (!defeatSoundStarted && SoundManager::getInstance().isSoundLoaded(DEFEAT))
                {
                    SoundManager::getInstance().playSound(DEFEAT, false); // one-shot
                    SoundManager::getInstance().setSoundVolume(DEFEAT, 0.5f);
                    defeatSoundStarted = true;
                }
                if (!boilingStarted && SoundManager::getInstance().isSoundLoaded(BOILING))
                {
                    SoundManager::getInstance().playSound(BOILING, false, 1.0f); // start silent
                    SoundManager::getInstance().fadeInMusic(BOILING, 0.7f, 1.0f); // fade in to 0.5 volume over 2 seconds
                    boilingStarted = true;
                }
                if (gameplayBGMPlaying && SoundManager::getInstance().isSoundLoaded(GAMEPLAY_BGM))
                {
                    SoundManager::getInstance().fadeOutMusic(GAMEPLAY_BGM, kBGMFadeDuration);
                    gameplayBGMPlaying = false;
                }

                // [ADDED] Check for Pause input to go to Pause Menu
                if (!editorMode && IsStateAdvanceInputPressed())
                {
                    pauseMenu.ResetLatches();
                    currentState = GameState::PAUSED;
                    BlockStateAdvanceInput();
                    break;
                }

                if (defeatScreen.ConsumeTryAgain())
                {
                    
                    if (SoundManager::getInstance().isSoundLoaded(DEFEAT))
                        SoundManager::getInstance().stopSound(DEFEAT);
                    if (SoundManager::getInstance().isSoundLoaded(BOILING))
                        SoundManager::getInstance().stopSound(BOILING);

                    
                    if (!currentLevelMusic.empty() &&
                        SoundManager::getInstance().isSoundLoaded(currentLevelMusic) &&
                        !SoundManager::getInstance().isSoundPlaying(currentLevelMusic))
                    {
                        SoundManager::getInstance().playSound(currentLevelMusic, 1.0f, 1.0f, true);
                        SoundManager::getInstance().setSoundVolume(currentLevelMusic, 0.0f);
                        SoundManager::getInstance().fadeInMusic(currentLevelMusic, kBGMFadeDuration, 0.4f);
                        gameplayBGMPlaying = true;
                    }

                    defeatSoundStarted = false;

                    
                    if (RequestReloadLevel(false))
                    {
                        ResetPlayerDefeat();
                    }
                }
                break;

            case GameState::WIN_VIDEO:
                handlePerfToggle();
                winVideoPlayer.Update(dt);

                if (winVideoPlayer.IsFinished())
                {
                    if (winVideoAudioPlaying && SoundManager::getInstance().isSoundLoaded(WIN_VIDEO_AUDIO)) {
                        SoundManager::getInstance().stopSound(WIN_VIDEO_AUDIO);
                    }
                    winVideoAudioPlaying = false;
                    if (gLogicSystem &&
                        StartGameplayLoadTransition(gLogicSystem->Factory()->LastLevelPath().empty()
                            ? gLogicSystem->ResolveDataPath("level.json")
                            : gLogicSystem->Factory()->LastLevelPath(),
                            false,
                            GameState::MAIN_MENU,
                            false))
                    {
                        ResetPlayerDefeat();
                        ResetPlayerKeyCount();
                        break;
                    }

                    ResetPlayerDefeat();
                    ResetHeiBangDefeat();
                    ResetPlayerKeyCount();
                    currentState = GameState::MAIN_MENU;
                    BlockStateAdvanceInput();
                }
                break;


                case GameState::EXIT:
                if (gInputSystem) {
                    if (auto* w = gInputSystem->Window()) w->close();
                }
                break;
            }

            Framework::setUpdate(0.0);
            }, "mygame::update");
    }

    /*************************************************************************************
     \brief  Draws the current game state and any state-specific UI overlays.
     \details Renders menus, gameplay systems, health presentation, cutscenes, and
              brightness overlays according to the active top-level game state.
    *************************************************************************************/
    void draw()
    {
        TryGuard::Run([&] {
            switch (currentState)
            {
            case GameState::MAIN_MENU:
                if (gRenderSystem) {
                    gRenderSystem->HandleMenuShortcuts();
                    gRenderSystem->BeginMenuFrame();
                    mainMenu.Draw(gRenderSystem);   // bg + GUI buttons
                    gRenderSystem->EndMenuFrame();
                    gRenderSystem->RenderBrightnessOverlay();
                }
                break;

            case GameState::PLAYING:
                gSystems.DrawAll();
                if (gRenderSystem) {
                    DrawHealthPresentation(*gRenderSystem);
                    gRenderSystem->RenderBrightnessOverlay();
                }
                break;

            case GameState::TRANSITIONING:
                if (gRenderSystem)
                {
                    gRenderSystem->BeginMenuFrame();
                    mainMenu.Draw(gRenderSystem);
                    const float normalized = (kStartTransitionDuration > 0.0f)
                        ? std::clamp(1.0f - (transitionTimer / kStartTransitionDuration), 0.0f, 1.0f)
                        : 1.0f;
                    gfx::Graphics::renderRectangleUI(0.0f, 0.0f,
                        static_cast<float>(gRenderSystem->ScreenWidth()),
                        static_cast<float>(gRenderSystem->ScreenHeight()),
                        0.0f, 0.0f, 0.0f, normalized,
                        gRenderSystem->ScreenWidth(), gRenderSystem->ScreenHeight());
                    gRenderSystem->EndMenuFrame();
                    gRenderSystem->RenderBrightnessOverlay();
                }
                break;

            case GameState::LOADING_TRANSITION:
                if (gRenderSystem) {
                    const bool showMainMenuDuringTransition =
                        loadingTransitionNextState == GameState::MAIN_MENU &&
                        (!loadingTransitionVideoEnabled ||
                            loadingTransitionElapsedTimer >= kLoadingTransitionMinimumVideoTime);
                    const bool showGameplayDuringTransition =
                        !showMainMenuDuringTransition &&
                        (!loadingTransitionHideGameplayUntilDelay ||
                            !loadingTransitionVideoEnabled ||
                            loadingTransitionElapsedTimer >= kLoadingTransitionMinimumVideoTime);

                    if (showGameplayDuringTransition)
                    {
                        gSystems.DrawAll();
                    }

                    if (showGameplayDuringTransition)
                    {
                        DrawHealthPresentation(*gRenderSystem);
                    }
                    gRenderSystem->BeginMenuFrame();
                    if (!showMainMenuDuringTransition &&
                        !showGameplayDuringTransition &&
                        loadingTransitionHideGameplayUntilDelay)
                    {
                        gfx::Graphics::renderRectangleUI(
                            0.0f, 0.0f,
                            static_cast<float>(gRenderSystem->ScreenWidth()),
                            static_cast<float>(gRenderSystem->ScreenHeight()),
                            0.0f, 0.0f, 0.0f, 1.0f,
                            gRenderSystem->ScreenWidth(), gRenderSystem->ScreenHeight());
                    }
                    if (showMainMenuDuringTransition)
                    {
                        mainMenu.Draw(gRenderSystem);
                    }
                    if (loadingTransitionVideoEnabled &&
                        !loadingTransitionSkipRequested &&
                        !transitionPlayer.IsFinished()) {
                        transitionPlayer.Draw();
                    }
                    gRenderSystem->EndMenuFrame();
                    gRenderSystem->RenderBrightnessOverlay();
                }
                break;

            case GameState::CUTSCENE:
                if (gRenderSystem)
                {
                    gRenderSystem->BeginMenuFrame();
                    cutscenePlayer.Draw();
                    gRenderSystem->EndMenuFrame();
                    gRenderSystem->RenderBrightnessOverlay();
                }
                break;

            case GameState::FINAL_LEVEL_CUTSCENE:
                if (gRenderSystem)
                {
                    gRenderSystem->BeginMenuFrame();
                    finalLevelCutscenePlayer.Draw();
                    gRenderSystem->EndMenuFrame();
                    gRenderSystem->RenderBrightnessOverlay();
                }
                break;

            case GameState::PAUSED:
                gSystems.DrawAll();
                if (gRenderSystem) {
                    DrawHealthPresentation(*gRenderSystem);
                    gRenderSystem->BeginMenuFrame();
                    pauseMenu.Draw(gRenderSystem);
                    gRenderSystem->EndMenuFrame();
                    gRenderSystem->RenderBrightnessOverlay();
                }
                break;

            case GameState::DEFEAT:
                gSystems.DrawAll();
                if (gRenderSystem)
                {
                    DrawHealthPresentation(*gRenderSystem);
                    gRenderSystem->BeginMenuFrame();
                    defeatScreen.Draw(gRenderSystem);
                    gRenderSystem->EndMenuFrame();
                    gRenderSystem->RenderBrightnessOverlay();
                }
                break;

            case GameState::WIN_VIDEO:
                if (gRenderSystem)
                {
                    gRenderSystem->BeginMenuFrame();
                    winVideoPlayer.Draw();
                    gRenderSystem->EndMenuFrame();
                    gRenderSystem->RenderBrightnessOverlay();
                }
                break;


            case GameState::EXIT:
                break;
            }
            }, "mygame::draw");
    }

    /*************************************************************************************
     \brief  Handles application focus changes reported by the Core.
     \param  suspended  True when the app loses focus or is minimized, false on resume.
    *************************************************************************************/
    void onAppFocusChanged(bool suspended)
    {
        if (suspended)
        {
            if (currentState == GameState::PLAYING &&
                !Framework::RenderSystem::IsEditorVisible())
            {
                pauseMenu.ResetLatches();
                currentState = GameState::PAUSED;
                BlockStateAdvanceInput();
            }
        }

        // Halt/resume audio cleanly and flush transient input so keys do not stick.
        SoundManager::getInstance().pauseAllSounds(suspended);
        if (gInputSystem)
        {
            gInputSystem->Manager().ClearState();
        }
    }

    /*************************************************************************************
     \brief  Shuts down game systems and prints allocator leak diagnostics.
     \details
             - Destroys the game-owned HitBoxSystem before engine systems are released.
             - Shuts down all registered systems through SystemManager.
             - Prints allocator leak information after shutdown for debugging.
    *************************************************************************************/
    void shutdown()
    {
        std::cout << "[Game] Shutting down systems...\n";

        levelLoader.Reset();
        transitionPlayer.Stop();
        cutscenePlayer.Stop();
        finalLevelCutscenePlayer.Stop();
        winVideoPlayer.Stop();
        if (SoundManager::getInstance().isSoundLoaded(FINAL_LEVEL_CUTSCENE_AUDIO)) {
            SoundManager::getInstance().stopSound(FINAL_LEVEL_CUTSCENE_AUDIO);
        }
        if (SoundManager::getInstance().isSoundLoaded(WIN_VIDEO_AUDIO)) {
            SoundManager::getInstance().stopSound(WIN_VIDEO_AUDIO);
        }

        if (gLogicSystem && gLogicSystem->hitBoxSystem)
        {
            gLogicSystem->SetPostUpdateCallback({});
            gLogicSystem->hitBoxSystem->Shutdown();
            delete gLogicSystem->hitBoxSystem;
            gLogicSystem->hitBoxSystem = nullptr;
        }

        // Only call ShutdownAll(), do NOT manually delete gEnemySystem etc.
        gSystems.ShutdownAll();

        const unsigned leaks = Framework::GameObjectPool::Storage().Allocator().DumpMemoryInUse(&allocatorDumpCallback);
        std::cout << "[Allocator] DumpMemoryInUse found " << leaks << " live blocks at shutdown.\n";
        // Null out global pointers so you donÃ¢â‚¬â„¢t accidentally access them later
        gEnemySystem = nullptr;
        gAiSystem = nullptr;
        gRenderSystem = nullptr;
        gAudioSystem = nullptr;
        gPhysicsSystem = nullptr;
        gLogicSystem = nullptr;
        gInputSystem = nullptr;
        gHealthSystem = nullptr;
        gParticleSystem = nullptr;
        gZoomTriggerSystem = nullptr;

        std::cout << "[Game] Shutdown complete.\n";
    }

    /*************************************************************************************
     \brief  Reports whether editor-driven simulation is currently running.
     \return True when the editor is allowing gameplay systems to update.
    *************************************************************************************/
    bool IsEditorSimulationRunning()
    {
        return editorSimulationRunning;
    }

    /*************************************************************************************
     \brief  Starts simulation from the editor bridge.
    *************************************************************************************/
    void EditorPlaySimulation()
    {
        editorSimulationRunning = true;
        if (currentState != GameState::PLAYING)
            currentState = GameState::PLAYING;
        if (Framework::FACTORY)
            Framework::FACTORY->Layers().LogVisibilitySummary("EditorPlaySimulation");
    }

    /*************************************************************************************
     \brief  Stops simulation from the editor bridge.
    *************************************************************************************/
    void EditorStopSimulation()
    {

        editorSimulationRunning = false;
        if (Framework::FACTORY)
            Framework::FACTORY->Layers().LogVisibilitySummary("EditorStopSimulation");
    }

    /*************************************************************************************
     \brief  Requests a staged reload of the current gameplay level using the transition video.
     \return True if the loading transition was started.
    *************************************************************************************/
    bool RequestReloadLevel(bool hideGameplayUntilDelay)
    {
        if (!gLogicSystem || !gLogicSystem->Factory())
            return false;

        std::filesystem::path levelPath = gLogicSystem->Factory()->LastLevelPath();
        if (levelPath.empty())
            levelPath = gLogicSystem->ResolveDataPath("level.json");

        return StartGameplayLoadTransition(levelPath, true, GameState::PLAYING, true, hideGameplayUntilDelay);
    }

    /*************************************************************************************
     \brief  Requests a staged load of a specific gameplay level using the transition video.
     \param  levelPath  Target level path to load.
     \return True if the loading transition was started.
    *************************************************************************************/
    bool RequestLoadLevel(const std::filesystem::path& levelPath)
    {
        if (!gLogicSystem || levelPath.empty())
            return false;

        if (currentState != GameState::FINAL_LEVEL_CUTSCENE &&
            StartLevelEntryCutscene(levelPath))
        {
            return true;
        }

        return StartGameplayLoadTransition(levelPath, true, GameState::PLAYING, true, false);
    }

    /*************************************************************************************
     \brief  Reports whether session-wide god mode is active.
     \return True when incoming player damage should be ignored and outgoing damage boosted.
    *************************************************************************************/
    bool IsGodModeEnabled()
    {
        return godModeEnabled;
    }

    /*************************************************************************************
     \brief  Returns the flat outgoing player damage applied while god mode is enabled.
     \return 100.0f while god mode is enabled, otherwise 1.0f.
    *************************************************************************************/
    float GetPlayerGodModeDamage()
    {
        return godModeEnabled ? kGodModePlayerDamage : 1.0f;
    }

    /*************************************************************************************
     \brief  Loads a level selected through the editor bridge.
     \param  levelPath  The level file to load.
     \return True if the request is valid and the level is passed to LogicSystem.
    *************************************************************************************/
    bool LoadLevelFromEditor(const std::filesystem::path& levelPath)
    {
        if (!gLogicSystem || levelPath.empty())
            return false;

        const bool beganLoad = StartGameplayLoadTransition(
            levelPath, editorSimulationRunning, GameState::PLAYING, false);
        if (beganLoad)
        {
            ResetPlayerDefeat();
            ResetPlayerKeyCount();
        }
        return beganLoad;
    }

    /*************************************************************************************
     \brief  Requests that the active gameplay state enter the pause menu.
     \return True when the request changed the state to paused.
    *************************************************************************************/
    bool RequestPauseMenu()
    {
        if (currentState != GameState::PLAYING)
            return false;

        if (Framework::RenderSystem::IsEditorVisible())
            return false;

        pauseMenu.ResetLatches();
        currentState = GameState::PAUSED;
        BlockStateAdvanceInput();
        return true;
    }

    /*************************************************************************************
     \brief  Reports whether gameplay scripts should ignore player-driven controls this frame.
     \details Used by game-side behaviours so camera/animation can continue updating while
              combat and movement input stay disabled during the loading transition.
    *************************************************************************************/
    bool IsGameplayInputBlocked()
    {
        return currentState == GameState::LOADING_TRANSITION;
    }



} // namespace mygame
