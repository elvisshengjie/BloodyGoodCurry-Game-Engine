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
