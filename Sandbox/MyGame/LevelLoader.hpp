/*********************************************************************************************
 \file      LevelLoader.hpp
 \par       SofaSpuds
 \author    SofaSpuds Team
 \brief     Incremental level loader used by MyGame's loading transition state.
*********************************************************************************************/

#pragma once

#include "Composition/Composition.h"
#include "Serialization/JsonSerialization.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Framework {
    class LogicSystem;
}

namespace mygame {

    class LevelLoader {
    public:
        bool BeginLoad(Framework::LogicSystem& logic, const std::filesystem::path& levelPath);
        void TickLoadStep(std::size_t maxObjectsPerTick);
        void Reset();

        bool IsActive() const { return active; }
        bool IsDone() const { return started && done; }
        bool Succeeded() const { return success; }
        float Progress() const;

    private:
        void FinishLoad();

        Framework::LogicSystem* logic = nullptr;
        std::unique_ptr<Framework::JsonSerializer> stream;
        std::filesystem::path targetPath;
        std::string levelName;
        std::vector<Framework::GOC*> loadedObjects;
        std::size_t nextObjectIndex = 0;
        std::size_t totalObjects = 0;
        bool started = false;
        bool active = false;
        bool prepared = false;
        bool done = false;
        bool success = false;
    };

} // namespace mygame
