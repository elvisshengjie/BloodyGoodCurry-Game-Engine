/*********************************************************************************************
 \file      ProjectContext.h
 \par       SofaSpuds
 \author
 \brief     Declares the active game project context and project-scoped path accessors.
 \details   Provides a small engine-level API for tracking the currently selected game
            project root and resolving its asset, data, and save directories.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include <filesystem>

namespace Framework
{
    void SetCurrentProjectRoot(const std::filesystem::path& root);
    const std::filesystem::path& GetCurrentProjectRoot();

    bool HasCurrentProject();
    bool InitializeProjectFromExecutableLayout();

    std::filesystem::path GetCurrentAssetsRoot();
    std::filesystem::path GetCurrentDataRoot();
    std::filesystem::path GetCurrentSavesRoot();
}
