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
#include "Runtime/NavigationSystem.h"
#include "Runtime/HealthSystem.h"
#include "Runtime/ZoomTriggerSystem.h"
#include "Audio/SoundManager.h"
#include "Debug/CrashLogger.hpp"
#include "Graphics/Graphics.hpp"
#include "Core/PathUtils.h"
#include "Debug/Perf.h"
#include "Memory/GameObjectPool.h"
#include "Memory/ObjectAllocator.h"
#include "Physics/Dynamics/RigidBodyComponent.h"
#include "Video/VideoPlayer.hpp"
#include <algorithm>
#include <array>
#include <GLFW/glfw3.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
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
        // BGM Sounds
        bool gameplayBGMPlaying = false;
        const char* GAMEPLAY_BGM = "BGM";

        //Defeat Sounds
        const char* DEFEAT = "Defeat";
        const char* BOILING = "Boiling";
        bool defeatBGMPlaying = false;
        bool defeatSoundStarted = false;
        bool boilingStarted = false;
        //Timer
        float bgmFadeTimer = 0.0f;
        constexpr float kBGMFadeDuration = 1.5f;

        using clock = std::chrono::high_resolution_clock;
        const std::array<const char*, 2> kBgmSoundIds = {
            "MenuMusic",
            "BGM"
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

        Framework::SystemManager gSystems;
        Framework::InputSystem* gInputSystem = nullptr;
        Framework::LogicSystem* gLogicSystem = nullptr;
        Framework::PhysicSystem* gPhysicsSystem = nullptr;
        Framework::AudioSystem* gAudioSystem = nullptr;
        Framework::RenderSystem* gRenderSystem = nullptr;
        Framework::EnemySystem* gEnemySystem = nullptr;
        Framework::NavSystem* gNavSystem = nullptr;
        Framework::AiSystem* gAiSystem = nullptr;
        Framework::HealthSystem* gHealthSystem = nullptr;
        Framework::ParticleSystem* gParticleSystem = nullptr;
        Framework::ZoomTriggerSystem* gZoomTriggerSystem = nullptr;

        enum class GameState { MAIN_MENU, CUTSCENE, TRANSITIONING, LOADING_TRANSITION, PLAYING, PAUSED, DEFEAT, EXIT };
        GameState currentState = GameState::MAIN_MENU;
        bool editorSimulationRunning = false;
       

        MainMenuPage mainMenu;
        PauseMenuPage pauseMenu;
        DefeatScreenPage defeatScreen;
        Framework::VideoPlayer cutscenePlayer;
        Framework::VideoPlayer transitionPlayer;
        LevelLoader levelLoader;
        bool cutsceneReady = false;
        bool cutsceneAudioPlaying = false;
        bool transitionReady = false;
        bool loadingTransitionVideoEnabled = true;
        bool loadingTransitionSkipRequested = false;
        bool loadingTransitionResumeSimulation = true;
        GameState loadingTransitionNextState = GameState::PLAYING;
        constexpr std::size_t kLoadingObjectsPerTick = 8;
        constexpr float kTransitionColorKeyLow = 0.02f;
        constexpr float kTransitionColorKeyHigh = 0.18f;
        const std::filesystem::path kTransitionVideoRelativePath =
            "Textures/UI/Transition screen/Transition screen.mpg";

        constexpr int START_KEY = GLFW_KEY_ENTER; // Keyboard stand-in for a controller Start button.
        constexpr int PAUSE_KEY = GLFW_KEY_ESCAPE;

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
            bool playVideo = true)
        {
            if (!gLogicSystem || levelPath.empty() || levelLoader.IsActive())
                return false;

            loadingTransitionVideoEnabled = playVideo && transitionReady;
            if (loadingTransitionVideoEnabled) {
                transitionPlayer.Start();
            }

            if (!levelLoader.BeginLoad(*gLogicSystem, levelPath))
            {
                std::cerr << "[LoadingTransition] Failed to begin load: " << levelPath << "\n";
                return false;
            }

            loadingTransitionSkipRequested = !loadingTransitionVideoEnabled;
            loadingTransitionResumeSimulation = resumeSimulationAfterLoad;
            loadingTransitionNextState = nextStateAfterLoad;
            editorSimulationRunning = false;
            currentState = GameState::LOADING_TRANSITION;
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
        gNavSystem = gSystems.RegisterSystem<Framework::NavSystem>(win);
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
            // Keep hitbox timing aligned with the old engine behavior, but route the update
            // through a generic game callback instead of a hardcoded LogicSystem dependency.
            gLogicSystem->SetPostUpdateCallback([](float dt)
            {
                if (gLogicSystem && gLogicSystem->hitBoxSystem)
                    gLogicSystem->hitBoxSystem->Update(dt);
            });
        }
        RegisterMyGameScripts(*gLogicSystem);
        BindCombatAudio(*gLogicSystem, *gHealthSystem);
        BindCombatVfx(*gLogicSystem);
        BindAiCombat(*gAiSystem, *gLogicSystem);
        BindHealthPresentation(*gHealthSystem);
        mainMenu.Init(gRenderSystem->ScreenWidth(), gRenderSystem->ScreenHeight());
        pauseMenu.Init(gRenderSystem->ScreenWidth(), gRenderSystem->ScreenHeight());
        defeatScreen.Init(gRenderSystem->ScreenWidth(), gRenderSystem->ScreenHeight());
        const std::string cutscenePath = ResolveFirstExistingAsset({
            "Video/output.mpg",
            "Video__OFF_WEB/output.mpg"
            });
        cutsceneReady = !cutscenePath.empty() && cutscenePlayer.Load(cutscenePath);
        if (!cutsceneReady) {
            std::cerr << "[Cutscene] Warning: Could not load output.mpg from Video/ or Video__OFF_WEB/.\n";
        }
        const auto transitionVideoPath = Framework::ResolveAssetPath(kTransitionVideoRelativePath);
        transitionReady = std::filesystem::exists(transitionVideoPath) &&
            transitionPlayer.Load(transitionVideoPath.string());
        if (transitionReady) {
            transitionPlayer.SetColorKeyEnabled(true, kTransitionColorKeyLow, kTransitionColorKeyHigh);
        }
        else {
            std::cerr << "[LoadingTransition] Warning: Could not load "
                << transitionVideoPath.string() << "\n";
        }
        if (!SoundManager::getInstance().isSoundLoaded(CUTSCENE_AUDIO)) {
            const std::string cutsceneAudioPath = ResolveFirstExistingAsset({
                "Video/audio.mp3",
                "Video__OFF_WEB/audio.mp3"
                });
            if (cutsceneAudioPath.empty() ||
                !SoundManager::getInstance().loadSound(CUTSCENE_AUDIO, cutsceneAudioPath)) {
                std::cerr << "[Cutscene] Warning: Could not load audio.mp3 from Video/ or Video__OFF_WEB/.\n";
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
            if (currentState == GameState::MAIN_MENU || currentState == GameState::TRANSITIONING || currentState == GameState::CUTSCENE) {
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
                if (!mainMenuBGMPlaying && SoundManager::getInstance().isSoundLoaded(MAIN_MENU_BGM)) {
                    SoundManager::getInstance().playSound(MAIN_MENU_BGM, 1.0f, 1.0f, true);
                    SoundManager::getInstance().setSoundVolume(MAIN_MENU_BGM, 0.0f);
                    SoundManager::getInstance().fadeInMusic(MAIN_MENU_BGM, kBGMFadeDuration, 0.3f);
                    mainMenuBGMPlaying = true;
                    gameplayBGMPlaying = false;
                }
                if (mainMenu.ConsumeStart())
                {
                    if (SoundManager::getInstance().isSoundLoaded(START_BUTTTON))
                        SoundManager::getInstance().playSound(START_BUTTTON);
                    SoundManager::getInstance().isSoundLoaded(MAIN_MENU_BGM);
                    SoundManager::getInstance().fadeOutMusic(MAIN_MENU_BGM, kBGMFadeDuration);
                    if (cutsceneReady) {
                        cutscenePlayer.Start();
                        if (SoundManager::getInstance().isSoundLoaded(CUTSCENE_AUDIO)) {
                            SoundManager::getInstance().stopSound(CUTSCENE_AUDIO);
                            SoundManager::getInstance().playSound(CUTSCENE_AUDIO, 1.0f, 1.0f, false);
                            cutsceneAudioPlaying = true;
                        }
                        currentState = GameState::CUTSCENE;
                    }
                    else {
                        if (!RequestReloadLevel())
                        {
                            currentState = GameState::TRANSITIONING;
                            transitionTimer = kStartTransitionDuration;
                        }
                    }
                    editorSimulationRunning = false;
                    pauseMenu.ResetLatches();
                    ResetPlayerDefeat();
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
                }
                break;

            case GameState::CUTSCENE: {
                cutscenePlayer.Update(dt);
                handlePerfToggle();
                const bool skipCutscene = gInputSystem &&
                    (gInputSystem->IsKeyPressed(START_KEY) || gInputSystem->IsKeyPressed(PAUSE_KEY));
                if (skipCutscene || cutscenePlayer.IsFinished())
                {
                    if (cutsceneAudioPlaying && SoundManager::getInstance().isSoundLoaded(CUTSCENE_AUDIO)) {
                        SoundManager::getInstance().stopSound(CUTSCENE_AUDIO);
                    }
                    cutsceneAudioPlaying = false;
                    if (!RequestReloadLevel())
                    {
                        currentState = GameState::PLAYING;
                        editorSimulationRunning = true;
                    }
                }
                break;
            }

            case GameState::LOADING_TRANSITION:
            {
                handlePerfToggle();
                levelLoader.TickLoadStep(kLoadingObjectsPerTick);
                const bool showMainMenuDuringTransition =
                    loadingTransitionNextState == GameState::MAIN_MENU;

                if (!showMainMenuDuringTransition)
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

                const bool skipTransition = gInputSystem &&
                    (gInputSystem->IsKeyPressed(START_KEY) || gInputSystem->IsKeyPressed(PAUSE_KEY));
                if (skipTransition && loadingTransitionVideoEnabled) {
                    loadingTransitionSkipRequested = true;
                }

                const bool loadingFinished = levelLoader.IsDone() && levelLoader.Succeeded();
                const bool transitionFinished =
                    !transitionReady || loadingTransitionSkipRequested || transitionPlayer.IsFinished();

                if (loadingFinished && transitionFinished)
                {
                    currentState = loadingTransitionNextState;
                    editorSimulationRunning = loadingTransitionResumeSimulation;
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

                if (!gameplayBGMPlaying && SoundManager::getInstance().isSoundLoaded(GAMEPLAY_BGM))
                {
                    SoundManager::getInstance().playSound(GAMEPLAY_BGM, 1.0f, 1.0f, true);
                    SoundManager::getInstance().setSoundVolume(GAMEPLAY_BGM, 0.0f); // start silent
                    SoundManager::getInstance().fadeInMusic(GAMEPLAY_BGM, kBGMFadeDuration, 0.4f); // fade to 0.4
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
                if (gInputSystem && !editorMode &&
                    (gInputSystem->IsKeyPressed(PAUSE_KEY) || gInputSystem->IsKeyPressed(START_KEY)))
                {
                    pauseMenu.ResetLatches();
                    currentState = GameState::PAUSED;
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
                if (pauseMenu.ConsumeResume() ||
                    (gInputSystem && (gInputSystem->IsKeyPressed(PAUSE_KEY) || gInputSystem->IsKeyPressed(START_KEY))))
                {
                    // [UPDATED LOGIC] Resume based on previous state if possible, 
                    // or default to PLAYING. If we came from DEFEAT, going back to PLAYING
                    // might be weird if the player is still dead, but typically "Resume" means "Back to Game".
                    // If the player is dead, the next frame's check in PLAYING will send them back to DEFEAT screen.
                    currentState = GameState::PLAYING;
                    break;
                }

                if (pauseMenu.ConsumeMainMenu())
                {
                    if (SoundManager::getInstance().isSoundLoaded(GAMEPLAY_BGM))
                    {
                        SoundManager::getInstance().fadeOutMusic(GAMEPLAY_BGM, kBGMFadeDuration);
                        gameplayBGMPlaying = false;
                    }
                    if (SoundManager::getInstance().isSoundLoaded(MAIN_MENU_BGM))
                    {
                        SoundManager::getInstance().playSound(MAIN_MENU_BGM, true); // loop
                        SoundManager::getInstance().setSoundVolume(MAIN_MENU_BGM, 0.0f);
                        SoundManager::getInstance().fadeInMusic(MAIN_MENU_BGM, kBGMFadeDuration, 0.4f);
                        mainMenuBGMPlaying = true;
                    }
                    if (gLogicSystem &&
                        StartGameplayLoadTransition(gLogicSystem->Factory()->LastLevelPath().empty()
                            ? gLogicSystem->ResolveDataPath("level.json")
                            : gLogicSystem->Factory()->LastLevelPath(),
                            false,
                            GameState::MAIN_MENU))
                    {
                        ResetPlayerDefeat();
                        ResetPlayerKeyCount();
                        break;
                    }
                    ResetPlayerDefeat();
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
                if (gInputSystem && !editorMode &&
                    (gInputSystem->IsKeyPressed(PAUSE_KEY) || gInputSystem->IsKeyPressed(START_KEY)))
                {
                    pauseMenu.ResetLatches();
                    currentState = GameState::PAUSED;
                    break;
                }

                if (defeatScreen.ConsumeTryAgain())
                {
                    if (SoundManager::getInstance().isSoundLoaded(DEFEAT)&& SoundManager::getInstance().isSoundLoaded(BOILING))
                    {
                        SoundManager::getInstance().stopSound(DEFEAT);
                        SoundManager::getInstance().stopSound(BOILING);
                    }
                    if (SoundManager::getInstance().isSoundLoaded(GAMEPLAY_BGM))
                    {
                        SoundManager::getInstance().playSound(GAMEPLAY_BGM, true);
                        SoundManager::getInstance().setSoundVolume(GAMEPLAY_BGM, 0.0f);
                        SoundManager::getInstance().fadeInMusic(GAMEPLAY_BGM, kBGMFadeDuration, 0.4f);
                        gameplayBGMPlaying = true;
                    }

                    defeatSoundStarted = false;
                    if (RequestReloadLevel())
                    {
                        ResetPlayerDefeat();
                        ResetPlayerKeyCount();
                    }
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
                if (loadingTransitionNextState != GameState::MAIN_MENU)
                {
                    gSystems.DrawAll();
                }
                if (gRenderSystem) {
                    if (loadingTransitionNextState != GameState::MAIN_MENU)
                    {
                        DrawHealthPresentation(*gRenderSystem);
                    }
                    gRenderSystem->BeginMenuFrame();
                    if (loadingTransitionNextState == GameState::MAIN_MENU)
                        mainMenu.Draw(gRenderSystem);
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
        gNavSystem = nullptr;
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
    bool RequestReloadLevel()
    {
        if (!gLogicSystem || !gLogicSystem->Factory())
            return false;

        std::filesystem::path levelPath = gLogicSystem->Factory()->LastLevelPath();
        if (levelPath.empty())
            levelPath = gLogicSystem->ResolveDataPath("level.json");

        return StartGameplayLoadTransition(levelPath, true, GameState::PLAYING, true);
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

        return StartGameplayLoadTransition(levelPath, true, GameState::PLAYING, true);
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
