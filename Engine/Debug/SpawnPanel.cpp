/*********************************************************************************************
 \file      SpawnPanel.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Implements the engine-owned editor spawn panel and game extension dispatch.
 \details   Keeps generic editor spawning in the engine while allowing game projects to
            contribute additional UI and apply custom component settings through callbacks.

 \copyright
            All content (c) 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#if SOFASPUDS_ENABLE_EDITOR

#include "SpawnPanel.h"

#include "Component/BehaviorTreeComponent.h"
#include "Component/CircleRenderComponent.h"
#include "Component/RenderComponent.h"
#include "Component/SpriteComponent.h"
#include "Component/TransformComponent.h"
#include "Composition/PrefabManager.h"
#include "Core/PathUtils.h"
#include "Debug/LayerPanel.h"
#include "Debug/Selection.h"
#include "Debug/UndoStack.h"
#include "Physics/Dynamics/RigidBodyComponent.h"
#include "Resource_Asset_Manager/Resource_Manager.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "Common/CRTDebug.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif

namespace Framework
{
    namespace
    {
        std::string sSpriteTexKey;
        unsigned sSpriteTextureId = 0;
        std::string sRectangleTexKey;
        unsigned sRectangleTextureId = 0;
        std::filesystem::path sAssetsRoot;

        bool gLevelFilesInitialized = false;
        std::vector<std::string> gLevelFiles;
        int gSelectedLevelIndex = 0;
        char gLevelNameBuffer[128] = "level";
        std::string gLevelStatusMessage;
        bool gLevelStatusIsError = false;

        EditorSimulationQueryCallback gIsEditorSimulationRunning;
        EditorLoadLevelCallback gLoadLevelFromEditor;
        std::vector<SpawnPanelExtension> gExtensions;

        std::string gSelectedPrefab = "Rect";
        std::string gSelectedPrefabToClear = "Rect";
        char gPrefabFilterBuffer[128] = "";
        SpawnSettings gSettings;
        bool gPendingPrefabSync = true;
        bool gPanelOpen = true;

        /*************************************************************************************
         \brief  Lowercase-copy helper used by filtering and sorting code.
         \param  value  Input string.
         \return Lowercased ASCII copy.
        *************************************************************************************/
        std::string ToLower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        }

        /*************************************************************************************
         \brief  Check whether a path uses a supported image extension.
         \param  path  Candidate filesystem path.
         \return True for `.png`, `.jpg`, or `.jpeg`.
        *************************************************************************************/
        bool IsTextureFile(const std::filesystem::path& path)
        {
            const auto lower = ToLower(path.extension().string());
            return lower == ".png" || lower == ".jpg" || lower == ".jpeg";
        }

        /*************************************************************************************
         \brief  Trim leading and trailing ASCII whitespace from a string copy.
         \param  value  Input string.
         \return Trimmed copy.
        *************************************************************************************/
        std::string TrimCopy(std::string value)
        {
            auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(),
                [&](unsigned char c) { return !isSpace(c); }));
            value.erase(std::find_if(value.rbegin(), value.rend(),
                [&](unsigned char c) { return !isSpace(c); }).base(), value.end());
            return value;
        }

        /*************************************************************************************
         \brief  Treat JSON files as levels when their filename contains `level`.
         \param  name  Candidate filename.
         \return True if the filename looks like a level file.
        *************************************************************************************/
        bool ContainsLevelKeyword(const std::string& name)
        {
            return ToLower(name).find("level") != std::string::npos;
        }

        /*************************************************************************************
         \brief  Resolve the active project's data directory used for level JSON files.
         \return Active project data root.
        *************************************************************************************/
        std::filesystem::path LevelDirectory()
        {
            return ResolveDataPath("");
        }

        /*************************************************************************************
         \brief  Rebuild the list of level JSON files available to the panel.
        *************************************************************************************/
        void RefreshLevelFileList()
        {
            gLevelFiles.clear();

            std::error_code ec;
            const auto levelDirectory = LevelDirectory();
            if (!std::filesystem::exists(levelDirectory, ec))
                return;

            for (const auto& entry : std::filesystem::directory_iterator(levelDirectory, ec))
            {
                if (ec)
                    break;
                if (!entry.is_regular_file())
                    continue;

                const auto& path = entry.path();
                if (path.extension() != ".json")
                    continue;

                const std::string filename = path.filename().string();
                if (!ContainsLevelKeyword(filename))
                    continue;

                gLevelFiles.push_back(filename);
            }

            std::sort(gLevelFiles.begin(), gLevelFiles.end());
            if (gSelectedLevelIndex >= static_cast<int>(gLevelFiles.size()))
                gSelectedLevelIndex = gLevelFiles.empty() ? 0 : static_cast<int>(gLevelFiles.size() - 1);
        }

        /*************************************************************************************
         \brief  Build an absolute level path from a level filename.
         \param  filename  Level filename relative to the project data directory.
         \return Absolute level path.
        *************************************************************************************/
        std::filesystem::path LevelFilePath(const std::string& filename)
        {
            return LevelDirectory() / filename;
        }

        /*************************************************************************************
         \brief  Determine whether an object is one of the prefab master copies.
         \param  obj  Candidate object pointer.
         \return True when the pointer belongs to the prefab master registry.
        *************************************************************************************/
        bool IsMasterObject(GOC* obj)
        {
            for (const auto& kv : master_copies)
            {
                if (kv.second.get() == obj)
                    return true;
            }
            return false;
        }

        /*************************************************************************************
         \brief  Collect all live scene objects excluding prefab masters.
         \return Vector of live non-master objects.
        *************************************************************************************/
        std::vector<GOC*> CollectNonMasterObjects()
        {
            std::vector<GOC*> result;
            if (!FACTORY)
                return result;

            result.reserve(FACTORY->Objects().size());
            for (auto& [id, objPtr] : FACTORY->Objects())
            {
                (void)id;
                auto* obj = objPtr.get();
                if (!obj || IsMasterObject(obj))
                    continue;
                result.push_back(obj);
            }
            return result;
        }

        /*************************************************************************************
         \brief  Destroy an object while recording the action for undo.
         \param  obj               Target object pointer.
         \param  flushImmediately  True to immediately process deferred deletes when safe.
        *************************************************************************************/
        void DestroyWithUndo(GOC* obj, bool flushImmediately = true)
        {
            if (!obj)
                return;

            mygame::editor::RecordObjectDeleted(*obj);
            if (FACTORY)
            {
                FACTORY->Destroy(obj);
                const bool isSimulationRunning = gIsEditorSimulationRunning ? gIsEditorSimulationRunning() : false;
                if (flushImmediately && !isSimulationRunning)
                    FACTORY->Update(0.0f);
            }
            else
            {
                obj->Destroy();
            }
        }

        /*************************************************************************************
         \brief  Build the panel context passed to game-side extension callbacks.
         \return Context referencing the current level list and level directory.
        *************************************************************************************/
        SpawnPanelContext MakeContext()
        {
            return SpawnPanelContext{ gLevelFiles, LevelDirectory() };
        }

        /*************************************************************************************
         \brief  Clear the sprite texture override preview and selection.
        *************************************************************************************/
        void ClearSpriteTexture()
        {
            sSpriteTexKey.clear();
            sSpriteTextureId = 0;
        }

        /*************************************************************************************
         \brief  Clear the rectangle texture override preview and selection.
        *************************************************************************************/
        void ClearRectangleTexture()
        {
            sRectangleTexKey.clear();
            sRectangleTextureId = 0;
        }

        /*************************************************************************************
         \brief  Apply generic engine spawn settings to a live object.
         \param  obj                     Target object.
         \param  settings                Generic panel settings.
         \param  index                   Batch index used for positional stepping.
         \param  applyTransformAndLayer  True when applying new-spawn transform offsets.
        *************************************************************************************/
        void ApplyGenericSettingsToObject(GOC& obj,
            const SpawnSettings& settings,
            int index,
            bool applyTransformAndLayer)
        {
            if (auto* tr = obj.GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent))
            {
                if (applyTransformAndLayer && settings.overridePrefabTransform)
                {
                    tr->x = settings.x;
                    tr->y = settings.y;
                    tr->rot = settings.rot;
                }

                if (applyTransformAndLayer)
                {
                    tr->x += settings.stepX * index;
                    tr->y += settings.stepY * index;
                }
            }

            auto* spriteComp = obj.GetComponentType<SpriteComponent>(ComponentTypeId::CT_SpriteComponent);

            if (auto* rc = obj.GetComponentType<RenderComponent>(ComponentTypeId::CT_RenderComponent))
            {
                if (settings.overridePrefabSize)
                {
                    rc->w = settings.w;
                    rc->h = settings.h;
                }

                rc->r = settings.rgba[0];
                rc->g = settings.rgba[1];
                rc->b = settings.rgba[2];
                rc->a = settings.rgba[3];

                if (settings.overridePrefabVisible)
                    rc->visible = settings.visible;
                if (settings.overridePrefabBlendMode)
                    rc->blendMode = settings.blendMode;

                if (!sRectangleTexKey.empty() && !spriteComp)
                {
                    rc->texture_key = sRectangleTexKey;
                    rc->texture_id = Resource_Manager::getTexture(sRectangleTexKey);
                }
            }

            if (auto* cc = obj.GetComponentType<CircleRenderComponent>(ComponentTypeId::CT_CircleRenderComponent))
            {
                if (settings.overridePrefabCircle)
                    cc->radius = settings.radius;
                cc->r = settings.rgba[0];
                cc->g = settings.rgba[1];
                cc->b = settings.rgba[2];
                cc->a = settings.rgba[3];
            }

            if (spriteComp && !sSpriteTexKey.empty())
            {
                spriteComp->texture_key = sSpriteTexKey;
                spriteComp->texture_id = Resource_Manager::getTexture(sSpriteTexKey);
            }

            if (auto* rb = obj.GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent))
            {
                if (settings.overridePrefabVelocity)
                {
                    rb->velX = settings.rbVelX;
                    rb->velY = settings.rbVelY;
                }
                if (settings.overridePrefabCollider)
                {
                    rb->width = settings.rbWidth;
                    rb->height = settings.rbHeight;
                }
            }
        }

        /*************************************************************************************
         \brief  Apply generic engine spawn settings to serialized level JSON components.
         \param  comps          `Components` JSON object for a level entry.
         \param  settings       Generic panel settings.
         \param  objectChanged  Set to true when any JSON field is updated.
        *************************************************************************************/
        void ApplyGenericSettingsToLevelJson(json& comps,
            const SpawnSettings& settings,
            bool& objectChanged)
        {
            auto setJsonFloat = [&objectChanged](json& obj, const char* key, float value)
            {
                auto it = obj.find(key);
                if (it == obj.end() || !it->is_number() || static_cast<float>(it->get<double>()) != value)
                {
                    obj[key] = value;
                    objectChanged = true;
                }
            };
            auto setJsonBool = [&objectChanged](json& obj, const char* key, bool value)
            {
                auto it = obj.find(key);
                if (it == obj.end() || !it->is_boolean() || it->get<bool>() != value)
                {
                    obj[key] = value;
                    objectChanged = true;
                }
            };
            auto setJsonString = [&objectChanged](json& obj, const char* key, const std::string& value)
            {
                auto it = obj.find(key);
                if (it == obj.end() || !it->is_string() || it->get<std::string>() != value)
                {
                    obj[key] = value;
                    objectChanged = true;
                }
            };

            const bool hasSprite = comps.contains("SpriteComponent");

            if (auto it = comps.find("RenderComponent"); it != comps.end() && it->is_object())
            {
                auto& rc = *it;
                if (settings.overridePrefabSize)
                {
                    setJsonFloat(rc, "w", settings.w);
                    setJsonFloat(rc, "h", settings.h);
                }
                setJsonFloat(rc, "r", settings.rgba[0]);
                setJsonFloat(rc, "g", settings.rgba[1]);
                setJsonFloat(rc, "b", settings.rgba[2]);
                setJsonFloat(rc, "a", settings.rgba[3]);
                if (settings.overridePrefabVisible)
                    setJsonBool(rc, "visible", settings.visible);
                if (settings.overridePrefabBlendMode)
                    setJsonString(rc, "blend_mode", BlendModeToString(settings.blendMode));
                if (!sRectangleTexKey.empty() && !hasSprite)
                    setJsonString(rc, "texture_key", sRectangleTexKey);
            }

            if (auto it = comps.find("CircleRenderComponent"); it != comps.end() && it->is_object())
            {
                auto& cc = *it;
                if (settings.overridePrefabCircle)
                    setJsonFloat(cc, "radius", settings.radius);
                setJsonFloat(cc, "r", settings.rgba[0]);
                setJsonFloat(cc, "g", settings.rgba[1]);
                setJsonFloat(cc, "b", settings.rgba[2]);
                setJsonFloat(cc, "a", settings.rgba[3]);
            }

            if (auto it = comps.find("SpriteComponent"); it != comps.end() && it->is_object())
            {
                auto& sp = *it;
                if (!sSpriteTexKey.empty())
                    setJsonString(sp, "texture_key", sSpriteTexKey);
            }

            if (auto it = comps.find("RigidBodyComponent"); it != comps.end() && it->is_object())
            {
                auto& rb = *it;
                if (settings.overridePrefabVelocity)
                {
                    setJsonFloat(rb, "velocity_x", settings.rbVelX);
                    setJsonFloat(rb, "velocity_y", settings.rbVelY);
                }
                if (settings.overridePrefabCollider)
                {
                    setJsonFloat(rb, "width", settings.rbWidth);
                    setJsonFloat(rb, "height", settings.rbHeight);
                }
            }
        }

        /*************************************************************************************
         \brief  Apply full spawn panel state to a live object.
         \param  obj                     Target object.
         \param  settings                Generic engine settings.
         \param  index                   Batch index used for stepping.
         \param  applyTransformAndLayer  True when applying new-spawn transform behavior.
        *************************************************************************************/
        void ApplySpawnSettingsToObject(GOC& obj,
            const SpawnSettings& settings,
            int index,
            bool applyTransformAndLayer)
        {
            ApplyGenericSettingsToObject(obj, settings, index, applyTransformAndLayer);
            const auto context = MakeContext();
            for (const auto& extension : gExtensions)
            {
                if (extension.applyToObject)
                    extension.applyToObject(obj, context, applyTransformAndLayer);
            }
        }

        /*************************************************************************************
         \brief  Apply current spawn settings to all existing instances of a prefab.
         \param  prefabName  Prefab/object name to update.
         \param  settings    Generic engine settings.
         \return Number of updated live objects.
        *************************************************************************************/
        size_t ApplySpawnSettingsToExistingInstances(const std::string& prefabName,
            const SpawnSettings& settings)
        {
            size_t updated = 0;
            auto all = CollectNonMasterObjects();
            for (auto* obj : all)
            {
                if (!obj || obj->GetObjectName() != prefabName)
                    continue;
                ApplySpawnSettingsToObject(*obj, settings, 0, false);
                ++updated;
            }
            return updated;
        }

        /*************************************************************************************
         \brief  Apply current spawn settings to matching prefab entries in a level file.
         \param  levelPath           Level JSON file to edit.
         \param  prefabName          Prefab/object name to match.
         \param  settings            Generic engine settings.
         \param  outObjectsUpdated   Number of modified objects written back.
         \param  outError            Error text on failure.
         \return True if the level was processed successfully.
        *************************************************************************************/
        bool ApplySpawnSettingsToLevelFile(const std::filesystem::path& levelPath,
            const std::string& prefabName,
            const SpawnSettings& settings,
            size_t& outObjectsUpdated,
            std::string& outError)
        {
            outObjectsUpdated = 0;
            outError.clear();

            std::ifstream in(levelPath);
            if (!in.is_open())
            {
                outError = "Failed to open " + levelPath.string();
                return false;
            }

            json root = json::parse(in, nullptr, false);
            if (root.is_discarded())
            {
                outError = "Failed to parse " + levelPath.string();
                return false;
            }

            auto levelIt = root.find("Level");
            if (levelIt == root.end() || !levelIt->is_object())
            {
                outError = "Missing Level object in " + levelPath.string();
                return false;
            }

            auto& levelObj = *levelIt;
            auto objectsIt = levelObj.find("GameObjects");
            if (objectsIt == levelObj.end() || !objectsIt->is_array())
            {
                outError = "Missing GameObjects array in " + levelPath.string();
                return false;
            }

            const auto context = MakeContext();
            bool changed = false;
            for (auto& obj : *objectsIt)
            {
                if (!obj.is_object())
                    continue;

                auto nameIt = obj.find("name");
                if (nameIt == obj.end() || !nameIt->is_string() || nameIt->get<std::string>() != prefabName)
                    continue;

                auto compsIt = obj.find("Components");
                if (compsIt == obj.end() || !compsIt->is_object())
                    continue;

                bool objectChanged = false;
                auto& comps = *compsIt;
                ApplyGenericSettingsToLevelJson(comps, settings, objectChanged);
                for (const auto& extension : gExtensions)
                {
                    if (extension.applyToLevelJson)
                        extension.applyToLevelJson(comps, context, objectChanged);
                }

                if (objectChanged)
                {
                    changed = true;
                    ++outObjectsUpdated;
                }
            }

            if (!changed)
                return true;

            std::ofstream out(levelPath);
            if (!out.is_open())
            {
                outError = "Failed to write " + levelPath.string();
                return false;
            }

            out << std::setw(2) << root;
            if (!out.good())
            {
                outError = "Failed to write " + levelPath.string();
                return false;
            }
            return true;
        }

        /*************************************************************************************
         \brief  Spawn one prefab instance and apply the current panel state.
         \param  prefab    Prefab key to clone.
         \param  settings  Generic engine settings.
         \param  index     Batch index used for transform stepping.
        *************************************************************************************/
        void SpawnOnePrefab(const char* prefab, const SpawnSettings& settings, int index)
        {
            GOC* obj = ClonePrefab(prefab);
            if (!obj)
                return;

            ApplySpawnSettingsToObject(*obj, settings, index, true);
            if (auto* btComp = obj->GetComponentType<BehaviorTreeComponent>(
                ComponentTypeId::CT_BehaviorTreeComponent))
            {
                btComp->BuildTree(obj);
            }

            obj->SetLayerName(mygame::ActiveLayerName());
            mygame::editor::RecordObjectCreated(*obj);
        }

        /*************************************************************************************
         \brief  Try to load and select a texture asset for sprite or rectangle overrides.
         \param  relativePath  Project-relative or absolute asset path.
         \param  outKey        Resolved resource key.
         \param  outHandle     Loaded texture handle.
         \return True if the texture was resolved and loaded successfully.
        *************************************************************************************/
        bool LoadTextureSelection(const std::filesystem::path& relativePath,
            std::string& outKey,
            unsigned& outHandle)
        {
            if (relativePath.empty() || sAssetsRoot.empty())
                return false;

            std::filesystem::path relative = relativePath;
            if (relative.is_absolute())
            {
                std::error_code ec;
                auto canonical = std::filesystem::weakly_canonical(relative, ec);
                if (ec)
                    return false;
                relative = canonical.lexically_relative(sAssetsRoot);
            }

            if (relative.empty())
                return false;

            std::error_code ec;
            auto absolute = std::filesystem::weakly_canonical(sAssetsRoot / relative, ec);
            if (ec)
                absolute = sAssetsRoot / relative;

            if (!std::filesystem::exists(absolute) ||
                !std::filesystem::is_regular_file(absolute) ||
                !IsTextureFile(absolute))
            {
                return false;
            }

            std::string key = relative.generic_string();
            if (!key.empty())
            {
                constexpr std::string_view kAssetsPrefix = "Assets/";
                constexpr std::string_view kLowerAssetsPrefix = "assets/";
                if (key.rfind(kAssetsPrefix, 0) != 0)
                {
                    if (key.rfind(kLowerAssetsPrefix, 0) == 0)
                        key = std::string(kAssetsPrefix) + key.substr(kLowerAssetsPrefix.size());
                    else
                        key = std::string(kAssetsPrefix) + key;
                }
            }

            if (key.empty())
                return false;

            if (Resource_Manager::resources_map.find(key) == Resource_Manager::resources_map.end())
                Resource_Manager::load(key, absolute.string());

            unsigned handle = Resource_Manager::getTexture(key);
            if (handle == 0)
                return false;

            outKey = std::move(key);
            outHandle = handle;
            return true;
        }

        /*************************************************************************************
         \brief  Check whether a prefab name matches the current search filter.
         \param  name    Prefab name.
         \param  filter  Active filter text.
         \return True when the prefab should be shown in the combo box.
        *************************************************************************************/
        bool PrefabMatchesFilter(const std::string& name, std::string_view filter)
        {
            if (filter.empty())
                return true;

            return std::search(name.begin(), name.end(), filter.begin(), filter.end(),
                [](unsigned char a, unsigned char b)
                {
                    return static_cast<unsigned char>(std::tolower(a)) ==
                        static_cast<unsigned char>(std::tolower(b));
                }) != name.end();
        }

        /*************************************************************************************
         \brief  Synchronize panel defaults from the selected prefab master object.
         \param  master  Newly selected prefab master.
        *************************************************************************************/
        void SyncSettingsFromMaster(const GOC& master)
        {
            if (auto* tr = master.GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent))
            {
                gSettings.x = tr->x;
                gSettings.y = tr->y;
                gSettings.rot = tr->rot;
            }

            if (auto* rc = master.GetComponentType<RenderComponent>(ComponentTypeId::CT_RenderComponent))
            {
                gSettings.w = rc->w;
                gSettings.h = rc->h;
                gSettings.visible = rc->visible;
                gSettings.overridePrefabVisible = false;
                gSettings.blendMode = rc->blendMode;
                gSettings.overridePrefabBlendMode = false;
            }

            if (auto* cc = master.GetComponentType<CircleRenderComponent>(ComponentTypeId::CT_CircleRenderComponent))
            {
                gSettings.radius = cc->radius;
            }

            if (auto* rb = master.GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent))
            {
                gSettings.rbWidth = rb->width;
                gSettings.rbHeight = rb->height;
                gSettings.rbVelX = rb->velX;
                gSettings.rbVelY = rb->velY;
            }

            const auto context = MakeContext();
            for (const auto& extension : gExtensions)
            {
                if (extension.onPrefabSelected)
                    extension.onPrefabSelected(master, context);
            }
        }

        /*************************************************************************************
         \brief  Draw the shared level save/load controls in the spawn panel.
        *************************************************************************************/
        void DrawLevelControls()
        {
            ImGui::SeparatorText("Levels");
            ImGui::InputText("Level Name", gLevelNameBuffer, IM_ARRAYSIZE(gLevelNameBuffer));

            ImGui::SameLine();
            if (ImGui::Button("Save Level"))
            {
                if (!FACTORY)
                {
                    gLevelStatusMessage = "Factory is not available.";
                    gLevelStatusIsError = true;
                }
                else
                {
                    std::string levelName = TrimCopy(gLevelNameBuffer);
                    if (levelName.empty())
                    {
                        gLevelStatusMessage = "Level name cannot be empty";
                        gLevelStatusIsError = true;
                    }
                    else
                    {
                        std::string filename = levelName;
                        if (filename.find('.') == std::string::npos)
                            filename += ".json";

                        const auto levelPath = LevelFilePath(filename);
                        const std::string levelLabel = std::filesystem::path(filename).stem().string();
                        if (FACTORY->SaveLevel(levelPath.string(), levelLabel))
                        {
                            gLevelStatusMessage = "Saved level to " + levelPath.string();
                            gLevelStatusIsError = false;
                            RefreshLevelFileList();
                        }
                        else
                        {
                            gLevelStatusMessage = "Failed to save level to " + levelPath.string();
                            gLevelStatusIsError = true;
                        }
                    }
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Refresh Level List"))
                RefreshLevelFileList();

            if (gLevelFiles.empty())
            {
                const auto levelDirectory = LevelDirectory();
                ImGui::TextDisabled("No level files found in %s", levelDirectory.string().c_str());
                return;
            }

            const char* preview = gLevelFiles[gSelectedLevelIndex].c_str();
            if (ImGui::BeginCombo("Available Levels", preview))
            {
                for (size_t i = 0; i < gLevelFiles.size(); ++i)
                {
                    const bool selected = (static_cast<int>(i) == gSelectedLevelIndex);
                    if (ImGui::Selectable(gLevelFiles[i].c_str(), selected))
                        gSelectedLevelIndex = static_cast<int>(i);
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (ImGui::Button("Load Selected Level"))
            {
                const std::string selected = gLevelFiles[gSelectedLevelIndex];
                const auto levelPath = LevelFilePath(selected);
                std::error_code ec;
                if (!std::filesystem::exists(levelPath, ec))
                {
                    gLevelStatusMessage = "Level file not found: " + levelPath.string();
                    gLevelStatusIsError = true;
                }
                else if (gLoadLevelFromEditor && gLoadLevelFromEditor(levelPath))
                {
                    const size_t count = FACTORY ? FACTORY->LastLevelObjects().size() : 0;
                    gLevelStatusMessage = "Loaded level from " + levelPath.string() +
                        " (" + std::to_string(count) + " objects)";
                    gLevelStatusIsError = false;
                }
                else
                {
                    gLevelStatusMessage = "Failed to load level from " + levelPath.string();
                    gLevelStatusIsError = true;
                }
            }
        }
    }

    /*************************************************************************************
     \brief  Draw the engine-owned spawn panel.
    *************************************************************************************/
    void DrawSpawnPanel()
    {
        ImGui::Begin("Spawn", &gPanelOpen);

        if (!gLevelFilesInitialized)
        {
            RefreshLevelFileList();
            gLevelFilesInitialized = true;
            if (FACTORY && !FACTORY->LastLevelName().empty())
            {
                std::snprintf(gLevelNameBuffer, IM_ARRAYSIZE(gLevelNameBuffer), "%s",
                    FACTORY->LastLevelName().c_str());
            }
        }

        if (master_copies.empty())
        {
            ImGui::TextDisabled("No prefabs are loaded.");
            ImGui::End();
            return;
        }

        if (master_copies.find(gSelectedPrefab) == master_copies.end())
        {
            gSelectedPrefab = master_copies.begin()->first;
            gPendingPrefabSync = true;
        }

        ImGui::InputTextWithHint("Prefab Search", "Type to filter...", gPrefabFilterBuffer,
            IM_ARRAYSIZE(gPrefabFilterBuffer));
        if (ImGui::BeginCombo("Prefab", gSelectedPrefab.c_str()))
        {
            std::string_view filter(gPrefabFilterBuffer);
            bool anyShown = false;
            for (const auto& kv : master_copies)
            {
                if (!PrefabMatchesFilter(kv.first, filter))
                    continue;

                anyShown = true;
                const bool selected = (kv.first == gSelectedPrefab);
                if (ImGui::Selectable(kv.first.c_str(), selected))
                {
                    if (gSelectedPrefab != kv.first)
                    {
                        gSelectedPrefab = kv.first;
                        gPendingPrefabSync = true;
                    }
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }

            if (!anyShown)
                ImGui::TextDisabled("No prefabs match \"%s\".", gPrefabFilterBuffer);

            ImGui::EndCombo();
        }

        GOC* master = nullptr;
        if (auto it = master_copies.find(gSelectedPrefab); it != master_copies.end())
            master = it->second.get();

        if (!master)
        {
            ImGui::TextDisabled("Missing master for '%s'", gSelectedPrefab.c_str());
            ImGui::End();
            return;
        }

        if (gPendingPrefabSync)
        {
            SyncSettingsFromMaster(*master);
            gPendingPrefabSync = false;
        }

        const bool hasTransform =
            (master->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent) != nullptr);
        auto* masterRender = master->GetComponentType<RenderComponent>(ComponentTypeId::CT_RenderComponent);
        const bool hasRender = (masterRender != nullptr);
        const bool hasSprite =
            (master->GetComponentType<SpriteComponent>(ComponentTypeId::CT_SpriteComponent) != nullptr);
        const bool hasCircle =
            (master->GetComponentType<CircleRenderComponent>(ComponentTypeId::CT_CircleRenderComponent) != nullptr);
        const bool hasRigidBody =
            (master->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponent) != nullptr);

        if (hasSprite)
        {
            ImGui::SeparatorText("Sprite");
            const char* previewLabel = sSpriteTexKey.empty() ? "<drop texture>" : sSpriteTexKey.c_str();
            ImGui::Text("Sprite Texture: %s", previewLabel);

            ImVec2 avail = ImGui::GetContentRegionAvail();
            const float previewEdge = std::min(128.0f, avail.x);
            const ImVec2 previewSize(previewEdge, previewEdge);

            ImGui::PushID("SpriteTexturePreview");
            if (!sSpriteTexKey.empty() && sSpriteTextureId != 0)
            {
                ImGui::Image((ImTextureID)(void*)(intptr_t)sSpriteTextureId, previewSize, ImVec2(0, 1), ImVec2(1, 0));
            }
            else
            {
                ImGui::Button("Drop Texture Here", previewSize);
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_BROWSER_PATH"))
                {
                    if (payload->Data && payload->DataSize > 0)
                    {
                        const std::string relative(static_cast<const char*>(payload->Data), payload->DataSize - 1);
                        (void)LoadTextureSelection(relative, sSpriteTexKey, sSpriteTextureId);
                    }
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::PopID();

            if (sSpriteTextureId != 0)
            {
                if (ImGui::Button("Clear Sprite Texture"))
                    ClearSpriteTexture();
            }
            else
            {
                ImGui::TextDisabled("Drag from the Content Browser or drop files into the editor window.");
            }
        }

        if (hasTransform)
        {
            ImGui::SeparatorText("Transform");
            ImGui::Checkbox("Override prefab transform", &gSettings.overridePrefabTransform);
            if (!gSettings.overridePrefabTransform)
                ImGui::BeginDisabled();
            ImGui::DragFloat("x", &gSettings.x, 0.005f, 0.0f, 1.0f);
            ImGui::DragFloat("y", &gSettings.y, 0.005f, 0.0f, 1.0f);
            ImGui::DragFloat("rot", &gSettings.rot, 0.01f, -3.14159f, 3.14159f);
            if (!gSettings.overridePrefabTransform)
                ImGui::EndDisabled();
        }

        if (hasRender)
        {
            ImGui::SeparatorText("Size");
            if (ImGui::Checkbox("Override prefab size", &gSettings.overridePrefabSize))
            {
                if (!gSettings.overridePrefabSize && masterRender)
                {
                    gSettings.w = masterRender->w;
                    gSettings.h = masterRender->h;
                }
            }

            if (!gSettings.overridePrefabSize)
                ImGui::BeginDisabled();
            ImGui::DragFloat("w", &gSettings.w, 0.005f, 0.01f, 1.0f);
            ImGui::DragFloat("h", &gSettings.h, 0.005f, 0.01f, 1.0f);
            if (!gSettings.overridePrefabSize)
                ImGui::EndDisabled();

            ImGui::SeparatorText("Visibility");
            if (ImGui::Checkbox("Override prefab visibility", &gSettings.overridePrefabVisible))
            {
                if (!gSettings.overridePrefabVisible && masterRender)
                    gSettings.visible = masterRender->visible;
            }
            if (!gSettings.overridePrefabVisible)
                ImGui::BeginDisabled();
            ImGui::Checkbox("Visible", &gSettings.visible);
            if (!gSettings.overridePrefabVisible)
                ImGui::EndDisabled();

            ImGui::SeparatorText("Blend Mode");
            if (ImGui::Checkbox("Override prefab blend mode", &gSettings.overridePrefabBlendMode))
            {
                if (!gSettings.overridePrefabBlendMode && masterRender)
                    gSettings.blendMode = masterRender->blendMode;
            }
            if (!gSettings.overridePrefabBlendMode)
                ImGui::BeginDisabled();
            int blendModeIndex = static_cast<int>(gSettings.blendMode);
            if (ImGui::Combo("Blend Mode", &blendModeIndex, kBlendModeLabels.data(),
                static_cast<int>(kBlendModeLabels.size())))
            {
                gSettings.blendMode = static_cast<BlendMode>(blendModeIndex);
            }
            if (!gSettings.overridePrefabBlendMode)
                ImGui::EndDisabled();

            if (!hasSprite)
            {
                ImGui::SeparatorText("Texture");
                const char* previewLabel = sRectangleTexKey.empty() ? "<drop texture>" : sRectangleTexKey.c_str();
                ImGui::Text("Texture: %s", previewLabel);

                ImVec2 avail = ImGui::GetContentRegionAvail();
                const float previewEdge = std::min(128.0f, avail.x);
                const ImVec2 previewSize(previewEdge, previewEdge);

                ImGui::PushID("RectangleTexturePreview");
                if (!sRectangleTexKey.empty() && sRectangleTextureId != 0)
                {
                    ImGui::Image((ImTextureID)(void*)(intptr_t)sRectangleTextureId, previewSize, ImVec2(0, 1), ImVec2(1, 0));
                }
                else
                {
                    ImGui::Button("Drop Texture Here", previewSize);
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_BROWSER_PATH"))
                    {
                        if (payload->Data && payload->DataSize > 0)
                        {
                            const std::string relative(static_cast<const char*>(payload->Data), payload->DataSize - 1);
                            (void)LoadTextureSelection(relative, sRectangleTexKey, sRectangleTextureId);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::PopID();

                if (sRectangleTextureId != 0)
                {
                    if (ImGui::Button("Clear Rectangle Texture"))
                        ClearRectangleTexture();
                }
                else
                {
                    ImGui::TextDisabled("Drag from the Content Browser or drop files into the editor window.");
                }
            }
        }

        if (hasCircle)
        {
            ImGui::SeparatorText("Circle");
            ImGui::Checkbox("Override prefab circle", &gSettings.overridePrefabCircle);
            if (!gSettings.overridePrefabCircle)
                ImGui::BeginDisabled();
            ImGui::DragFloat("radius", &gSettings.radius, 0.005f, 0.01f, 1.0f);
            if (!gSettings.overridePrefabCircle)
                ImGui::EndDisabled();
        }

        if (hasRigidBody)
        {
            ImGui::SeparatorText("RigidBody");
            if (ImGui::Checkbox("Override prefab collider", &gSettings.overridePrefabCollider))
            {
                if (!gSettings.overridePrefabCollider)
                {
                    if (auto* rb = master->GetComponentType<RigidBodyComponent>(
                        ComponentTypeId::CT_RigidBodyComponent))
                    {
                        gSettings.rbWidth = rb->width;
                        gSettings.rbHeight = rb->height;
                    }
                }
            }
            if (!gSettings.overridePrefabCollider)
                ImGui::BeginDisabled();
            ImGui::DragFloat("Collider Width", &gSettings.rbWidth, 0.005f, 0.01f, 2.0f);
            ImGui::DragFloat("Collider Height", &gSettings.rbHeight, 0.005f, 0.01f, 2.0f);
            if (!gSettings.overridePrefabCollider)
                ImGui::EndDisabled();

            ImGui::Separator();

            if (ImGui::Checkbox("Override prefab velocity", &gSettings.overridePrefabVelocity))
            {
                if (!gSettings.overridePrefabVelocity)
                {
                    if (auto* rb = master->GetComponentType<RigidBodyComponent>(
                        ComponentTypeId::CT_RigidBodyComponent))
                    {
                        gSettings.rbVelX = rb->velX;
                        gSettings.rbVelY = rb->velY;
                    }
                }
            }
            if (!gSettings.overridePrefabVelocity)
                ImGui::BeginDisabled();
            ImGui::DragFloat("Velocity X", &gSettings.rbVelX, 0.01f, -100.0f, 100.0f);
            ImGui::DragFloat("Velocity Y", &gSettings.rbVelY, 0.01f, -100.0f, 100.0f);
            if (!gSettings.overridePrefabVelocity)
                ImGui::EndDisabled();
        }

        const auto context = MakeContext();
        for (const auto& extension : gExtensions)
        {
            if (extension.drawUi)
                extension.drawUi(*master, context);
        }

        if (hasRender || hasCircle)
        {
            ImGui::SeparatorText("Color");
            ImGui::ColorEdit4("rgba", gSettings.rgba);
        }

        ImGui::SeparatorText("Batch");
        ImGui::DragInt("count", &gSettings.count, 1, 1, 500);
        ImGui::DragFloat("stepX", &gSettings.stepX, 0.005f);
        ImGui::DragFloat("stepY", &gSettings.stepY, 0.005f);

        DrawLevelControls();

        if (!gLevelStatusMessage.empty())
        {
            const ImVec4 color = gLevelStatusIsError
                ? ImVec4(0.9f, 0.3f, 0.3f, 1.0f)
                : ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
            ImGui::TextColored(color, "%s", gLevelStatusMessage.c_str());
        }

        if (master_copies.find(gSelectedPrefabToClear) == master_copies.end())
            gSelectedPrefabToClear = master_copies.begin()->first;

        if (ImGui::BeginCombo("Clear Prefab", gSelectedPrefabToClear.c_str()))
        {
            for (const auto& kv : master_copies)
            {
                const bool selected = (kv.first == gSelectedPrefabToClear);
                if (ImGui::Selectable(kv.first.c_str(), selected))
                    gSelectedPrefabToClear = kv.first;
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Spawn"))
        {
            for (int i = 0; i < gSettings.count; ++i)
                SpawnOnePrefab(gSelectedPrefab.c_str(), gSettings, i);
        }

        ImGui::SameLine();
        if (ImGui::Button("Apply to Existing"))
            (void)ApplySpawnSettingsToExistingInstances(gSelectedPrefab, gSettings);

        ImGui::SameLine();
        if (ImGui::Button("Apply to All Levels"))
        {
            const size_t sceneUpdates = ApplySpawnSettingsToExistingInstances(gSelectedPrefab, gSettings);
            size_t fileUpdates = 0;
            size_t objectUpdates = 0;
            size_t fileFailures = 0;
            std::string lastFailure;

            RefreshLevelFileList();
            for (const auto& filename : gLevelFiles)
            {
                const auto levelPath = LevelFilePath(filename);
                size_t fileObjects = 0;
                std::string error;
                if (!ApplySpawnSettingsToLevelFile(levelPath, gSelectedPrefab, gSettings, fileObjects, error))
                {
                    ++fileFailures;
                    lastFailure = error;
                    continue;
                }

                if (fileObjects > 0)
                {
                    ++fileUpdates;
                    objectUpdates += fileObjects;
                }
            }

            gLevelStatusIsError = (fileFailures > 0);
            if (gLevelStatusIsError)
            {
                gLevelStatusMessage = "Applied spawn settings to " + std::to_string(objectUpdates) +
                    " instances across " + std::to_string(fileUpdates) +
                    " level files (" + std::to_string(sceneUpdates) +
                    " in current scene). Failed to update " + std::to_string(fileFailures) +
                    " files. Last error: " + lastFailure;
            }
            else
            {
                gLevelStatusMessage = "Applied spawn settings to " + std::to_string(objectUpdates) +
                    " instances across " + std::to_string(fileUpdates) +
                    " level files (" + std::to_string(sceneUpdates) + " in current scene).";
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear Selected Prefab") && !gSelectedPrefabToClear.empty())
        {
            auto toKill = CollectNonMasterObjects();
            toKill.erase(std::remove_if(toKill.begin(), toKill.end(),
                [](GOC* obj) { return obj->GetObjectName() != gSelectedPrefabToClear; }),
                toKill.end());

            mygame::ClearSelection();
            mygame::SetHoverObjectId(0);
            for (auto* obj : toKill)
                DestroyWithUndo(obj, false);
            if (FACTORY)
                FACTORY->Update(0.0f);
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear All (keep masters)"))
        {
            const auto toKill = CollectNonMasterObjects();
            mygame::ClearSelection();
            mygame::SetHoverObjectId(0);
            for (auto* obj : toKill)
                DestroyWithUndo(obj, false);
            if (FACTORY)
                FACTORY->Update(0.0f);
        }

        ImGui::SeparatorText("Counts");
        const size_t totalObjs = FACTORY ? FACTORY->Objects().size() : 0;
        ImGui::Text("Total objects:   %zu", totalObjs);

        ImGui::End();
    }

    /*************************************************************************************
     \brief  Update the assets root used for drag-drop texture rebasing.
     \param  root  Active project assets directory.
    *************************************************************************************/
    void SetSpawnPanelAssetsRoot(const std::filesystem::path& root)
    {
        if (root.empty())
        {
            sAssetsRoot.clear();
            ClearSpriteTexture();
            ClearRectangleTexture();
            gLevelFilesInitialized = false;
            gLevelFiles.clear();
            gSelectedLevelIndex = 0;
            return;
        }

        std::error_code ec;
        auto canonical = std::filesystem::weakly_canonical(root, ec);
        sAssetsRoot = ec ? root : canonical;
        ClearSpriteTexture();
        ClearRectangleTexture();
        gLevelFilesInitialized = false;
        gLevelFiles.clear();
        gSelectedLevelIndex = 0;
    }

    /*************************************************************************************
     \brief  Install editor callbacks used by the panel for simulation and level loading.
     \param  isSimulationRunning  Query for current editor simulation state.
     \param  loadLevelFromEditor  Callback used when the panel requests level loading.
    *************************************************************************************/
    void SetSpawnPanelEditorCallbacks(
        EditorSimulationQueryCallback isSimulationRunning,
        EditorLoadLevelCallback loadLevelFromEditor)
    {
        gIsEditorSimulationRunning = std::move(isSimulationRunning);
        gLoadLevelFromEditor = std::move(loadLevelFromEditor);
    }

    /*************************************************************************************
     \brief  Register a game-side extension callback bundle with the spawn panel.
     \param  extension  Extension definition to append.
    *************************************************************************************/
    void RegisterSpawnPanelExtension(SpawnPanelExtension extension)
    {
        if (extension.id.empty())
            extension.id = "UnnamedSpawnExtension";
        gExtensions.push_back(std::move(extension));
        gPendingPrefabSync = true;
    }

    /*************************************************************************************
     \brief  Remove all currently registered game-side spawn panel extensions.
    *************************************************************************************/
    void ClearSpawnPanelExtensions()
    {
        gExtensions.clear();
        gPendingPrefabSync = true;
    }
}

#endif
