/*********************************************************************************************
 \file      ProjectContext.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%
 \brief     Declares the active game project context and project-scoped path accessors.
 \details   Provides a small engine-level API for tracking the currently selected game
            project root and resolving its asset, data, and save directories.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "ProjectContext.h"
#include "PathUtils.h"

#include <fstream>
#include <string>
#include <system_error>
#include "Common/CRTDebug.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif

namespace Framework
{
    namespace
    {
        constexpr const char* kProjectRootMarker = "sofaspuds_project_root.txt";
        std::filesystem::path gProjectRoot;

        /*************************************************************************************
         \brief  Returns a weakly canonical version of the given path when possible.
         \param  path  The input filesystem path to normalize.
         \return A normalized path, or the original path if canonicalization fails.
        *************************************************************************************/
        std::filesystem::path CanonicalIfPossible(const std::filesystem::path& path)
        {
            if (path.empty())
                return {};

            std::error_code ec;
            const auto canonical = std::filesystem::weakly_canonical(path, ec);
            return ec ? path : canonical;
        }

        /*************************************************************************************
         \brief  Checks whether a named child directory exists under a root path.
         \param  root   The parent directory to inspect.
         \param  child  The child directory name to test for.
         \return True if the child exists and is a directory.
        *************************************************************************************/
        bool HasDirectory(const std::filesystem::path& root, const char* child)
        {
            std::error_code ec;
            const auto candidate = root / child;
            return std::filesystem::exists(candidate, ec) &&
                std::filesystem::is_directory(candidate, ec);
        }

        /*************************************************************************************
         \brief  Chooses the preferred project subdirectory, with support for legacy names.
         \param  root       The project root directory.
         \param  preferred  The modern subdirectory name.
         \param  legacy     The legacy fallback subdirectory name.
         \return The resolved subdirectory path, or an empty path if root is empty.
        *************************************************************************************/
        std::filesystem::path PickProjectSubdir(const std::filesystem::path& root,
            const char* preferred,
            const char* legacy)
        {
            if (root.empty())
                return {};

            if (HasDirectory(root, preferred))
                return CanonicalIfPossible(root / preferred);

            if (HasDirectory(root, legacy))
                return CanonicalIfPossible(root / legacy);

            return CanonicalIfPossible(root / preferred);
        }

        /*************************************************************************************
         \brief  Validates and adopts a candidate path as the current project root.
         \param  candidate  The path to test as a project root.
         \return True if the path matches either the modern or legacy project layout.
        *************************************************************************************/
        bool TryUseProjectRoot(const std::filesystem::path& candidate)
        {
            if (candidate.empty())
                return false;

            // Generated projects can legitimately start with Data only and an empty Assets
            // folder. In web builds, Emscripten may not materialize an empty /Assets mount,
            // so accept either side of the recognized layout instead of requiring both.
            const bool modernLayout = HasDirectory(candidate, "Assets") || HasDirectory(candidate, "Data");
            const bool legacyLayout = HasDirectory(candidate, "assets") || HasDirectory(candidate, "Data_Files");
            if (!modernLayout && !legacyLayout)
                return false;

            gProjectRoot = CanonicalIfPossible(candidate);
            return !gProjectRoot.empty();
        }

        /*************************************************************************************
         \brief  Trims leading and trailing ASCII whitespace from a string.
         \param  value  The string to trim in place.
        *************************************************************************************/
        void TrimAsciiWhitespace(std::string& value)
        {
            const auto first = value.find_first_not_of(" \t\r\n");
            if (first == std::string::npos)
            {
                value.clear();
                return;
            }

            const auto last = value.find_last_not_of(" \t\r\n");
            value = value.substr(first, last - first + 1);
        }

        /*************************************************************************************
         \brief  Searches upward for a project marker file and adopts its referenced root.
         \param  start  The directory to begin probing from.
         \return True if a valid project root marker is found and accepted.
        *************************************************************************************/
        bool TryUseProjectMarker(const std::filesystem::path& start)
        {
            if (start.empty())
                return false;

            auto probe = start;
            for (int up = 0; up < 4 && !probe.empty(); ++up)
            {
                std::error_code ec;
                const auto marker = probe / kProjectRootMarker;
                if (std::filesystem::exists(marker, ec) &&
                    std::filesystem::is_regular_file(marker, ec))
                {
                    std::ifstream in(marker);
                    std::string rootLine;
                    if (std::getline(in, rootLine))
                    {
                        TrimAsciiWhitespace(rootLine);
                        if (!rootLine.empty())
                        {
                            std::filesystem::path candidate(rootLine);
                            if (candidate.is_relative())
                                candidate = probe / candidate;

                            if (TryUseProjectRoot(candidate))
                                return true;
                        }
                    }
                }

                if (probe == probe.root_path())
                    break;

                probe = probe.parent_path();
            }

            return false;
        }
    }

    /*************************************************************************************
     \brief  Sets the active project root explicitly.
     \param  root  The project root to store, or an empty path to clear the current project.
    *************************************************************************************/
    void SetCurrentProjectRoot(const std::filesystem::path& root)
    {
        if (root.empty())
        {
            gProjectRoot.clear();
            return;
        }

        gProjectRoot = CanonicalIfPossible(root);
    }

    /*************************************************************************************
     \brief  Returns the active project root, initializing it from runtime layout if needed.
     \return A reference to the current project root path.
    *************************************************************************************/
    const std::filesystem::path& GetCurrentProjectRoot()
    {
        if (gProjectRoot.empty())
            InitializeProjectFromExecutableLayout();

        return gProjectRoot;
    }

    /*************************************************************************************
     \brief  Reports whether a project root is currently active.
     \return True if a current project has been resolved or assigned.
    *************************************************************************************/
    bool HasCurrentProject()
    {
        return !gProjectRoot.empty();
    }

    /*************************************************************************************
     \brief  Attempts to discover the active project root from the executable layout.
     \details Checks for a project marker near the current working directory and executable,
              then walks upward looking for either the modern or legacy project folder layout.
     \return True if a project root is found and stored.
    *************************************************************************************/
    bool InitializeProjectFromExecutableLayout()
    {
        if (!gProjectRoot.empty())
            return true;

        std::error_code ec;
        const std::filesystem::path roots[] = {
            std::filesystem::current_path(ec),
            GetExecutableDir()
        };

        for (const auto& start : roots)
        {
            if (start.empty())
                continue;

            if (TryUseProjectMarker(start))
                return true;

            auto probe = start;
            for (int up = 0; up < 8 && !probe.empty(); ++up)
            {
                if (TryUseProjectRoot(probe))
                    return true;

                if (probe == probe.root_path())
                    break;

                probe = probe.parent_path();
            }
        }

        return false;
    }

    /*************************************************************************************
     \brief  Returns the active project's asset directory.
     \return The modern `Assets` directory, or the legacy `assets` directory if present.
    *************************************************************************************/
    std::filesystem::path GetCurrentAssetsRoot()
    {
        return PickProjectSubdir(GetCurrentProjectRoot(), "Assets", "assets");
    }

    /*************************************************************************************
     \brief  Returns the active project's data directory.
     \return The modern `Data` directory, or the legacy `Data_Files` directory if present.
    *************************************************************************************/
    std::filesystem::path GetCurrentDataRoot()
    {
        return PickProjectSubdir(GetCurrentProjectRoot(), "Data", "Data_Files");
    }

    /*************************************************************************************
     \brief  Returns the active project's save directory.
     \return The `Saves` directory under the active project root, or an empty path.
    *************************************************************************************/
    std::filesystem::path GetCurrentSavesRoot()
    {
        if (const auto root = GetCurrentProjectRoot(); !root.empty())
            return CanonicalIfPossible(root / "Saves");

        return {};
    }
}
