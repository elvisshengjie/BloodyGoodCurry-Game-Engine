/*********************************************************************************************
 \file      BuildSizeAnalyzerPanel.h
 \par       SofaSpuds
 \author    OpenAI Codex - Primary Author, 100%
 \brief     Declares an editor window that analyzes project asset sizes and exports selections.
 \details   The Build Size Analyzer scans the active project's asset directory, displays each
            asset with its file size, allows users to choose assets for export, persists the
            selection between uses, and copies selected assets to a destination folder while
            preserving the relative directory layout.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

namespace mygame {

    class BuildSizeAnalyzerPanel {
    public:
        void Initialize(const std::filesystem::path& assetsRoot);
        void ForceRefresh();
        void Draw(bool* isOpen = nullptr);

    private:
        struct AssetEntry {
            std::filesystem::path absolutePath;
            std::filesystem::path relativePath;
            std::string key;
            std::uintmax_t size = 0;
            bool selected = false;
        };

        void RefreshEntries();
        void RecomputeSelectionTotals();
        void LoadSelectionState();
        void SaveSelectionState() const;
        void DrawStatusLine() const;
        void DrawExportPopup();
        void SetStatus(const std::string& message, bool isError);
        void SetExportPathBuffer(const std::filesystem::path& path);
        std::filesystem::path ResolveSelectionStatePath() const;
        bool ExportSelectedAssets(const std::filesystem::path& destination,
            std::size_t& exportedCount,
            std::size_t& failedCount) const;

        static std::string PrettySize(std::uintmax_t size);
        static std::string ToLower(std::string value);
        static std::string TrimCopy(std::string value);
        static std::string PathKey(const std::filesystem::path& relativePath);

        std::filesystem::path m_assetsRoot;
        std::filesystem::path m_selectionStatePath;
        std::vector<AssetEntry> m_assets;
        std::unordered_set<std::string> m_selectedKeys;

        std::uintmax_t m_totalSelectedBytes = 0;
        std::size_t m_selectedCount = 0;

        std::array<char, 512> m_exportBuffer{};
        bool m_openExportPopup = false;
        std::string m_exportError;
        std::string m_statusMessage;
        bool m_statusIsError = false;
    };

} // namespace mygame
