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

    /*****************************************************************************************
      \class LevelLoader
      \brief Incrementally instantiates a level over multiple frames during a loading transition.

      \details
      Owns a JsonSerializer positioned on the level's GameObjects array and builds a bounded
      number of objects each TickLoadStep() call. This keeps loading on the main thread while
      still allowing rendering, camera updates, and transition video playback to continue.
    *****************************************************************************************/
    class LevelLoader {
    public:
        /*************************************************************************************
          \brief Opens the target level file and prepares staged loading state.
          \param logic     Active LogicSystem that owns the factory/world being rebuilt.
          \param levelPath Target level path, relative or absolute.
          \return True if the level file was opened and the loader is ready to tick.
        *************************************************************************************/
        bool BeginLoad(Framework::LogicSystem& logic, const std::filesystem::path& levelPath);

        /*************************************************************************************
          \brief Builds up to maxObjectsPerTick objects from the pending level file.
          \param maxObjectsPerTick Upper bound on object instantiations for this frame.
          \details
            - The first tick clears the old level via LogicSystem::PrepareForIncrementalLevelLoad().
            - Subsequent ticks instantiate objects directly into the live factory.
            - Once all objects are built, FinishLoad() finalizes callbacks and cached metadata.
        *************************************************************************************/
        void TickLoadStep(std::size_t maxObjectsPerTick);

        /*************************************************************************************
          \brief Clears all staged load state without finalizing a level.
          \note Safe to call when idle or after a completed load.
        *************************************************************************************/
        void Reset();

        bool IsActive() const { return active; }
        bool IsDone() const { return started && done; }
        bool Succeeded() const { return success; }

        /*************************************************************************************
          \brief Returns normalized staged-load progress in the range [0, 1].
        *************************************************************************************/
        float Progress() const;

    private:
        /*************************************************************************************
          \brief Completes the incremental load and hands the new object list back to LogicSystem.
        *************************************************************************************/
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
