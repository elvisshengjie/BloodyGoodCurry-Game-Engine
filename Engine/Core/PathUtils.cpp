/*********************************************************************************************
 \file      PathUtils.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim ( elvisshengjie.lim@digipen.edu ) - Primary Author, 100%

 \brief     Implements cross-platform path utilities for locating the executable directory
            and nearby content roots (assets/ and Data_Files/). Provides robust resolution
            of relative asset/data paths for both IDE runs and packaged builds.

 \details   Why this exists:
            - When running inside Visual Studio, the working directory is typically the
              build folder, so relative paths like "assets/..." may work.
            - When running a packaged EXE by double-clicking, the working directory can be
              different; fragile paths like "../../assets" break and lead to missing-file
              crashes.
            - This module searches from both the current working directory and the EXE
              directory, walking upward to find the nearest "assets" or "Data_Files"
              folders, and then resolves file paths against those roots with validation.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "PathUtils.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN          ///< Reduce Windows header bloat.
#endif
#ifndef NOMINMAX
#  define NOMINMAX                     ///< Prevent Windows from defining min/max macros.
#endif
#include <Windows.h>
#else
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

#include <array>
#include <iostream>
#include <vector>

namespace Framework
{
    namespace
    {
        /*************************************************************************************
          \brief Attempts to canonicalize a path without throwing exceptions.
          \param p  Input path.
          \return   A weakly-canonical (normalized) version of p if possible, otherwise p.

          \details  weakly_canonical resolves ".." and "." segments and tries to normalize
                    symlinks. We pass an error_code to avoid exceptions (important for
                    packaged builds where some probes may not exist).
        *************************************************************************************/
        std::filesystem::path CanonicalIfPossible(const std::filesystem::path& p)
        {
            std::error_code ec;
            auto canonical = std::filesystem::weakly_canonical(p, ec);
            return ec ? p : canonical;
        }

        /*************************************************************************************
          \brief Finds the nearest directory named \p dirname by searching upward.
          \param dirname  Directory name to locate (e.g., "assets", "Data_Files").
          \return         Canonical path to the nearest matching directory, or empty if not found.

          \details  Search strategy:
                    1) Start from two roots:
                       - current working directory (fs::current_path)
                       - executable directory (GetExecutableDir)
                    2) For each root, walk upward up to 7 parents.
                    3) At each level, check for a child folder named dirname.
                    4) Return the first match found.
        *************************************************************************************/
        std::filesystem::path FindNearestDirectoryImpl(std::string_view dirname)
        {
            namespace fs = std::filesystem;

            // Roots to probe from: where we were launched + where the EXE lives.
            std::vector<fs::path> roots{ fs::current_path(), GetExecutableDir() };

            for (const auto& root : roots)
            {
                if (root.empty())
                    continue;

                auto probe = root;
                for (int up = 0; up < 7 && !probe.empty(); ++up)
                {
                    fs::path candidate = probe / dirname;
                    std::error_code ec;

                    // If a directory called dirname exists here, we found our root.
                    if (fs::exists(candidate, ec) && fs::is_directory(candidate, ec))
                        return CanonicalIfPossible(candidate);

                    // Otherwise, move one folder up and try again.
                    probe = probe.parent_path();
                }
            }

            // Not found.
            return {};
        }

        /*************************************************************************************
          \brief Resolves a relative path against a discovered root and legacy layout hints.
          \param root     Discovered content root (e.g., path to assets/).
          \param rel      Relative file path inside that root.
          \param dirname  Name of the root folder ("assets" or "Data_Files").
          \return         A usable full path to the file.

          \details  Candidate order:
                    1) If root is known, try root/rel first (packaged layout).
                    2) Try legacy layout hints:
                       - dirname/rel
                       - ../dirname/rel
                       - ../../dirname/rel
                       - ../../../dirname/rel
                    3) Return the first existing candidate.
                    4) If nothing exists, return best guess:
                       - root/rel if root exists, else rel.
        *************************************************************************************/
        std::filesystem::path ResolveAgainstRoot(const std::filesystem::path& root,
            const std::filesystem::path& rel,
            std::string_view dirname)
        {
            namespace fs = std::filesystem;
            std::vector<fs::path> candidates;

            // Preferred packaged layout: content sits beside the EXE.
            if (!root.empty())
            {
                candidates.emplace_back(root / rel);
            }

            // Hints for older build-folder layouts where content may be higher up.
            std::array<const char*, 4> relHints{
                "", "..", "../..", "../../.."
            };

            // Build candidates like:
            //   assets/rel
            //   ../assets/rel
            //   ../../assets/rel
            //   ../../../assets/rel
            for (auto hint : relHints)
            {
                fs::path candidate = fs::path(hint) / dirname / rel;
                candidates.emplace_back(candidate);
            }

            // Choose the first candidate that actually exists.
            for (const auto& candidate : candidates)
            {
                std::error_code ec;
                if (fs::exists(candidate, ec))
                    return CanonicalIfPossible(candidate);
            }

            // If no candidate exists, still return a sensible fallback path.
            return root.empty() ? rel : (root / rel);
        }
    } // anonymous namespace

    /*************************************************************************************
      \brief Returns the directory containing the currently running executable.
      \return Path to the EXE folder.

      \details  Platform-specific implementation:
                - Windows: GetModuleFileNameA
                - macOS: _NSGetExecutablePath
                - Linux: readlink("/proc/self/exe")
                Falls back to current working directory if detection fails.
    *************************************************************************************/
    std::filesystem::path GetExecutableDir()
    {
#if defined(_WIN32)
        // Windows: query full path to EXE, then remove filename.
        char buf[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, buf, MAX_PATH);
        return std::filesystem::path(buf).parent_path();
#elif defined(__APPLE__)
        // macOS: _NSGetExecutablePath writes into buffer (may require resize).
        char buf[2048];
        uint32_t sz = sizeof(buf);
        if (_NSGetExecutablePath(buf, &sz) == 0)
            return std::filesystem::path(buf).parent_path();

        std::string s; s.resize(sz);
        if (_NSGetExecutablePath(s.data(), &sz) == 0)
            return std::filesystem::path(s).parent_path();

        return std::filesystem::current_path();
#else
        // Linux: EXE path is exposed via /proc/self/exe symlink.
        char buf[4096] = {};
        ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (n > 0)
        {
            buf[n] = 0;
            return std::filesystem::path(buf).parent_path();
        }
        return std::filesystem::current_path();
#endif
    }

    /*************************************************************************************
      \brief Finds the nearest directory with the given name starting from common roots.
      \param dirname  Directory name to search for.
      \return         Full canonical path if found; empty otherwise.
    *************************************************************************************/
    std::filesystem::path FindNearestDirectory(std::string_view dirname)
    {
        return FindNearestDirectoryImpl(dirname);
    }

    /*************************************************************************************
      \brief Finds the root directory for assets.
      \return Full canonical path to "assets" folder, or empty if not found.
    *************************************************************************************/
    std::filesystem::path FindAssetsRoot()
    {
        return FindNearestDirectoryImpl("assets");
    }

    /*************************************************************************************
      \brief Finds the root directory for Data_Files.
      \return Full canonical path to "Data_Files" folder, or empty if not found.

      \details  Uses FindNearestDirectoryImpl first; if not found, tries a few common
                relative positions to support legacy layouts.
    *************************************************************************************/
    std::filesystem::path FindDataFilesRoot()
    {
        namespace fs = std::filesystem;

        // Prefer the nearest-directory search (works for packaged builds).
        if (auto nearest = FindNearestDirectoryImpl("Data_Files"); !nearest.empty())
            return nearest;

        // Legacy fallback search relative to current dir.
        static const char* rels[] = {
            "Data_Files",
            "../Data_Files",
            "../../Data_Files",
            "../../../Data_Files"
        };

        for (auto rel : rels)
        {
            fs::path candidate = rel;
            std::error_code ec;
            if (fs::exists(candidate, ec) && fs::is_directory(candidate, ec))
                return CanonicalIfPossible(candidate);
        }

        // Not found.
        return {};
    }

    /*************************************************************************************
      \brief Resolves a relative asset path to a full usable path.
      \param relative  Relative path inside assets (e.g., "Textures/player.png").
      \return          Best resolved full path to that asset.

      \details  Uses FindAssetsRoot + ResolveAgainstRoot to handle both packaged and
                IDE build layouts.
    *************************************************************************************/
    std::filesystem::path ResolveAssetPath(const std::filesystem::path& relative)
    {
        return ResolveAgainstRoot(FindAssetsRoot(), relative, "assets");
    }

    /*************************************************************************************
      \brief Resolves a relative Data_Files path to a full usable path.
      \param relative  Relative path inside Data_Files (e.g., "level01.json").
      \return          Best resolved full path to that data file.
    *************************************************************************************/
    std::filesystem::path ResolveDataPath(const std::filesystem::path& relative)
    {
        return ResolveAgainstRoot(FindDataFilesRoot(), relative, "Data_Files");
    }
}
