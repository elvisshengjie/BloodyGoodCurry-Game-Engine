/*********************************************************************************************
 \file      LevelLoader.cpp
 \par       SofaSpuds
 \author    SofaSpuds Team
 \brief     Incremental level loader used by MyGame's loading transition state.
*********************************************************************************************/

#include "LevelLoader.hpp"

#include "Factory/Factory.h"
#include "Systems/LogicSystem.h"

namespace mygame {

    bool LevelLoader::BeginLoad(Framework::LogicSystem& inLogic, const std::filesystem::path& levelPath)
    {
        if (active || levelPath.empty() || !inLogic.Factory())
            return false;

        Reset();

        targetPath = levelPath.is_relative()
            ? inLogic.ResolveDataPath(levelPath.generic_string())
            : levelPath;

        stream = std::make_unique<Framework::JsonSerializer>();
        if (!stream->Open(targetPath.string()) || !stream->IsGood())
        {
            Reset();
            return false;
        }

        if (!stream->EnterObject("Level"))
        {
            Reset();
            return false;
        }

        if (stream->HasKey("name"))
            stream->ReadString("name", levelName);

        if (!stream->EnterArray("GameObjects"))
        {
            Reset();
            return false;
        }

        logic = &inLogic;
        totalObjects = stream->ArraySize();
        loadedObjects.reserve(totalObjects);
        started = true;
        active = true;
        success = false;

        return true;
    }

    void LevelLoader::TickLoadStep(std::size_t maxObjectsPerTick)
    {
        if (!active || !logic || !stream)
            return;

        if (!prepared)
        {
            logic->PrepareForIncrementalLevelLoad();
            prepared = true;
        }

        std::size_t loadedThisTick = 0;
        while (loadedThisTick < maxObjectsPerTick && nextObjectIndex < totalObjects)
        {
            if (stream->EnterIndex(nextObjectIndex))
            {
                if (auto* obj = logic->Factory()->BuildFromCurrentJsonObject(*stream))
                {
                    obj->initialize();
                    loadedObjects.push_back(obj);
                }
                stream->ExitObject();
            }

            ++nextObjectIndex;
            ++loadedThisTick;
        }

        if (nextObjectIndex >= totalObjects)
            FinishLoad();
    }

    void LevelLoader::Reset()
    {
        logic = nullptr;
        stream.reset();
        targetPath.clear();
        levelName.clear();
        loadedObjects.clear();
        nextObjectIndex = 0;
        totalObjects = 0;
        started = false;
        active = false;
        prepared = false;
        done = false;
        success = false;
    }

    float LevelLoader::Progress() const
    {
        if (!started)
            return 0.0f;
        if (done || totalObjects == 0)
            return 1.0f;
        return static_cast<float>(nextObjectIndex) / static_cast<float>(totalObjects);
    }

    void LevelLoader::FinishLoad()
    {
        if (!logic)
            return;

        if (stream)
        {
            stream->ExitArray();
            stream->ExitObject();
        }

        logic->FinalizeIncrementalLevelLoad(targetPath, levelName, loadedObjects);
        active = false;
        done = true;
        success = true;
        stream.reset();
    }

} // namespace mygame
