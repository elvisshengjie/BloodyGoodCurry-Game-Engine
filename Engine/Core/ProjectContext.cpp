/*********************************************************************************************
 \file      ProjectContext.cpp
 \par       SofaSpuds
 \brief     Stores the active game project root and resolves project-scoped folders.
*********************************************************************************************/

#include "ProjectContext.h"
#include "PathUtils.h"

#include <system_error>
#include "Common/CRTDebug.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif

namespace Framework
{
    namespace
    {
        std::filesystem::path gProjectRoot;

        std::filesystem::path CanonicalIfPossible(const std::filesystem::path& path)
        {
            if (path.empty())
                return {};

            std::error_code ec;
            const auto canonical = std::filesystem::weakly_canonical(path, ec);
            return ec ? path : canonical;
        }

        bool HasDirectory(const std::filesystem::path& root, const char* child)
        {
            std::error_code ec;
            const auto candidate = root / child;
            return std::filesystem::exists(candidate, ec) &&
                std::filesystem::is_directory(candidate, ec);
        }

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

        bool TryUseProjectRoot(const std::filesystem::path& candidate)
        {
            if (candidate.empty())
                return false;

            const bool modernLayout = HasDirectory(candidate, "Assets") && HasDirectory(candidate, "Data");
            const bool legacyLayout = HasDirectory(candidate, "assets") && HasDirectory(candidate, "Data_Files");
            if (!modernLayout && !legacyLayout)
                return false;

            gProjectRoot = CanonicalIfPossible(candidate);
            return !gProjectRoot.empty();
        }
    }

    void SetCurrentProjectRoot(const std::filesystem::path& root)
    {
        if (root.empty())
        {
            gProjectRoot.clear();
            return;
        }

        gProjectRoot = CanonicalIfPossible(root);
    }

    const std::filesystem::path& GetCurrentProjectRoot()
    {
        if (gProjectRoot.empty())
            InitializeProjectFromExecutableLayout();

        return gProjectRoot;
    }

    bool HasCurrentProject()
    {
        return !gProjectRoot.empty();
    }

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

    std::filesystem::path GetCurrentAssetsRoot()
    {
        return PickProjectSubdir(GetCurrentProjectRoot(), "Assets", "assets");
    }

    std::filesystem::path GetCurrentDataRoot()
    {
        return PickProjectSubdir(GetCurrentProjectRoot(), "Data", "Data_Files");
    }

    std::filesystem::path GetCurrentSavesRoot()
    {
        if (const auto root = GetCurrentProjectRoot(); !root.empty())
            return CanonicalIfPossible(root / "Saves");

        return {};
    }
}
