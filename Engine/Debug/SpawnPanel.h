/*********************************************************************************************
 \file      SpawnPanel.h
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Declares the engine-owned editor spawn panel and its game extension API.
 \details   The engine owns the generic spawn workflow: prefab selection, engine-component
            overrides, batch spawn, and level save/load utilities. Game projects extend
            the panel by registering callbacks that draw extra UI and apply project-specific
            component settings during object spawning and level JSON edits.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/
#pragma once

#if SOFASPUDS_ENABLE_EDITOR

#include "Component/RenderComponent.h"
#include "Factory/Factory.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace Framework
{
    struct SpawnSettings
    {
        float x{ 0.5f };
        float y{ 0.5f };
        float rot{ 0.0f };
        float w{ 0.5f };
        float h{ 0.5f };
        float radius{ 0.08f };
        bool overridePrefabTransform{ false };
        bool overridePrefabSize{ false };
        bool overridePrefabCircle{ false };
        bool overridePrefabCollider{ false };
        bool overridePrefabVelocity{ false };
        bool overridePrefabVisible{ false };
        bool overridePrefabBlendMode{ false };
        float rgba[4]{ 1.f, 1.f, 1.f, 1.f };
        float rbWidth{ 0.5f };
        float rbHeight{ 0.5f };
        float rbVelX{ 0.f };
        float rbVelY{ 0.f };
        int count{ 1 };
        float stepX{ 0.05f };
        float stepY{ 0.0f };
        bool visible{ true };
        BlendMode blendMode{ BlendMode::Alpha };
    };

    struct SpawnPanelContext
    {
        const std::vector<std::string>& levelFiles;
        const std::filesystem::path& levelDirectory;
    };

    struct SpawnPanelExtension
    {
        std::string id;
        std::function<void(const GOC&, const SpawnPanelContext&)> onPrefabSelected;
        std::function<void(const GOC&, const SpawnPanelContext&)> drawUi;
        std::function<void(GOC&, const SpawnPanelContext&, bool applyTransformAndLayer)> applyToObject;
        std::function<void(json&, const SpawnPanelContext&, bool& objectChanged)> applyToLevelJson;
    };

    using EditorSimulationQueryCallback = std::function<bool()>;
    using EditorLoadLevelCallback = std::function<bool(const std::filesystem::path&)>;

    /*************************************************************************************
     \brief  Draw the engine-owned spawn panel.
    *************************************************************************************/
    void DrawSpawnPanel();

    /*************************************************************************************
     \brief  Update the assets root used for drag-drop texture rebasing.
     \param  root  Active project assets directory.
    *************************************************************************************/
    void SetSpawnPanelAssetsRoot(const std::filesystem::path& root);

    /*************************************************************************************
     \brief  Install editor callbacks used by the panel for simulation and level loading.
     \param  isSimulationRunning  Query for current editor simulation state.
     \param  loadLevelFromEditor  Callback used when the panel requests level loading.
    *************************************************************************************/
    void SetSpawnPanelEditorCallbacks(
        EditorSimulationQueryCallback isSimulationRunning,
        EditorLoadLevelCallback loadLevelFromEditor);

    /*************************************************************************************
     \brief  Register a game-side extension callback bundle with the spawn panel.
     \param  extension  Extension definition to append.
    *************************************************************************************/
    void RegisterSpawnPanelExtension(SpawnPanelExtension extension);

    /*************************************************************************************
     \brief  Remove all currently registered game-side spawn panel extensions.
    *************************************************************************************/
    void ClearSpawnPanelExtensions();
}

#endif
