/*********************************************************************************************
 \file      BuildSizeAnalyzerPanel.cpp
 \par       SofaSpuds
 \author    OpenAI Codex - Primary Author, 100%
 \brief     Implements the editor window used to review asset sizes before export.
 \details   Recursively scans the project's Assets folder, renders an ImGui table with
            per-file sizes, tracks checkbox selection state, persists that state under the
            active project's Saves folder, and exports the chosen files to a user-provided
            destination while preserving relative paths.
 \copyright
            All content ©2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "Debug/BuildSizeAnalyzerPanel.h"

#if SOFASPUDS_ENABLE_EDITOR

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <imgui.h>
#include <iterator>
#include <sstream>
#include <system_error>
#include <unordered_set>

#include "Common/CRTDebug.h"
#include "Core/ProjectContext.h"
#include "Serialization/JsonSerialization.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif

namespace mygame {

    namespace {

        constexpr const char* kWindowTitle = "Build Size Analyzer";
        constexpr const char* kExportPopupTitle = "Export Selected Assets";
        constexpr const char* kFolderBrowserPopupTitle = "Select Export Folder";

    }

    void BuildSizeAnalyzerPanel::Initialize(const std::filesystem::path& assetsRoot)
    {
        std::error_code ec;
        if (assetsRoot.empty())
        {
            m_assetsRoot.clear();
            m_selectionStatePath.clear();
            m_assets.clear();
            m_selectedKeys.clear();
            m_totalSelectedBytes = 0;
            m_selectedCount = 0;
            m_exportBuffer.fill('\0');
            m_exportError.clear();
            m_statusMessage.clear();
            m_statusIsError = false;
            m_openExportPopup = false;
            m_openFolderBrowserPopup = false;
            m_folderBrowserPath.clear();
            m_folderBrowserSelection.clear();
            return;
        }

        auto canonical = std::filesystem::weakly_canonical(assetsRoot, ec);
        m_assetsRoot = ec ? std::filesystem::absolute(assetsRoot) : canonical;
        m_selectionStatePath = ResolveSelectionStatePath();
        m_assets.clear();
        m_selectedKeys.clear();
        m_totalSelectedBytes = 0;
        m_selectedCount = 0;
        m_exportBuffer.fill('\0');
        m_exportError.clear();
        m_statusMessage.clear();
        m_statusIsError = false;
        m_openExportPopup = false;
        m_openFolderBrowserPopup = false;

        const auto defaultExportPath = m_assetsRoot.parent_path() / "BuildExports";
        SetExportPathBuffer(defaultExportPath);
        m_folderBrowserPath = m_assetsRoot.parent_path();
        m_folderBrowserSelection = m_folderBrowserPath;
        LoadSelectionState();
        RefreshEntries();
    }

    void BuildSizeAnalyzerPanel::ForceRefresh()
    {
        RefreshEntries();
    }

    void BuildSizeAnalyzerPanel::Draw(bool* isOpen)
    {
        if (isOpen && !*isOpen)
            return;

        if (!ImGui::Begin(kWindowTitle, isOpen))
        {
            ImGui::End();
            return;
        }

        if (m_assetsRoot.empty())
        {
            ImGui::TextDisabled("No active project assets folder was found.");
            ImGui::End();
            return;
        }

        DrawStatusLine();

        ImGui::TextWrapped("Review the full asset list, choose what to export, and monitor the combined size of the current selection.");
        ImGui::TextDisabled("Assets Root: %s", m_assetsRoot.string().c_str());

        const bool hasAssets = !m_assets.empty();
        bool selectionChanged = false;

        if (ImGui::Button("Refresh"))
        {
            ForceRefresh();
        }

        ImGui::SameLine();
        ImGui::BeginDisabled(!hasAssets);
        if (ImGui::Button("Select All"))
        {
            for (auto& asset : m_assets)
            {
                asset.selected = true;
                m_selectedKeys.insert(asset.key);
            }
            selectionChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Deselect All"))
        {
            for (auto& asset : m_assets)
                asset.selected = false;
            m_selectedKeys.clear();
            selectionChanged = true;
        }
        ImGui::EndDisabled();

        if (selectionChanged)
            RecomputeSelectionTotals();

        const bool hasSelection = m_selectedCount > 0;
        ImGui::SameLine();
        ImGui::BeginDisabled(!hasSelection);
        if (ImGui::Button("Export Selected"))
        {
            m_exportError.clear();
            m_openExportPopup = true;
        }
        ImGui::EndDisabled();

        ImGui::Separator();
        ImGui::Text("Selected Assets: %zu / %zu", m_selectedCount, m_assets.size());
        ImGui::Text("Total Selected Size: %s", PrettySize(m_totalSelectedBytes).c_str());

        if (m_openExportPopup)
        {
            ImGui::OpenPopup(kExportPopupTitle);
            m_openExportPopup = false;
        }
        DrawExportPopup();

        if (hasAssets)
        {
            ImVec2 tableSize = ImGui::GetContentRegionAvail();
            if (ImGui::BeginTable("BuildSizeAnalyzerAssets",
                3,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_SizingStretchProp,
                tableSize))
            {
                ImGui::TableSetupColumn("Export", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                ImGui::TableSetupColumn("Asset");
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                ImGui::TableHeadersRow();

                for (auto& asset : m_assets)
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushID(asset.key.c_str());
                    bool selected = asset.selected;
                    if (ImGui::Checkbox("##Export", &selected))
                    {
                        asset.selected = selected;
                        if (selected)
                            m_selectedKeys.insert(asset.key);
                        else
                            m_selectedKeys.erase(asset.key);
                        selectionChanged = true;
                    }
                    ImGui::PopID();

                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(asset.relativePath.generic_string().c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(PrettySize(asset.size).c_str());
                }

                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::Spacing();
            ImGui::TextDisabled("No regular files were found under this project's Assets folder.");
        }

        if (selectionChanged)
        {
            RecomputeSelectionTotals();
            SaveSelectionState();
        }

        ImGui::End();
    }

    void BuildSizeAnalyzerPanel::RefreshEntries()
    {
        m_assets.clear();
        m_totalSelectedBytes = 0;
        m_selectedCount = 0;

        if (m_assetsRoot.empty())
            return;

        std::error_code ec;
        if (!std::filesystem::exists(m_assetsRoot, ec) || !std::filesystem::is_directory(m_assetsRoot, ec))
            return;

        std::filesystem::recursive_directory_iterator end;
        std::filesystem::recursive_directory_iterator it(
            m_assetsRoot,
            std::filesystem::directory_options::skip_permission_denied,
            ec);

        while (!ec && it != end)
        {
            const auto& entry = *it;
            std::error_code typeEc;
            if (entry.is_regular_file(typeEc) && !typeEc)
            {
                AssetEntry asset;
                asset.absolutePath = entry.path();
                asset.relativePath = asset.absolutePath.lexically_relative(m_assetsRoot);
                if (!asset.relativePath.empty())
                {
                    const std::string relativeText = asset.relativePath.generic_string();
                    if (!relativeText.empty() && relativeText.rfind("..", 0) != 0)
                    {
                        std::error_code sizeEc;
                        asset.size = entry.file_size(sizeEc);
                        if (sizeEc)
                            asset.size = 0;

                        asset.key = PathKey(asset.relativePath);
                        asset.selected = m_selectedKeys.find(asset.key) != m_selectedKeys.end();
                        m_assets.push_back(std::move(asset));
                    }
                }
            }

            it.increment(ec);
        }

        std::sort(m_assets.begin(), m_assets.end(),
            [](const AssetEntry& lhs, const AssetEntry& rhs) {
                return ToLower(lhs.relativePath.generic_string()) <
                    ToLower(rhs.relativePath.generic_string());
            });

        std::unordered_set<std::string> validSelections;
        validSelections.reserve(m_assets.size());
        for (const auto& asset : m_assets)
        {
            if (asset.selected)
                validSelections.insert(asset.key);
        }

        if (validSelections != m_selectedKeys)
        {
            m_selectedKeys = std::move(validSelections);
            SaveSelectionState();
        }

        RecomputeSelectionTotals();
    }

    void BuildSizeAnalyzerPanel::RecomputeSelectionTotals()
    {
        m_totalSelectedBytes = 0;
        m_selectedCount = 0;

        for (const auto& asset : m_assets)
        {
            if (!asset.selected)
                continue;

            ++m_selectedCount;
            m_totalSelectedBytes += asset.size;
        }
    }

    void BuildSizeAnalyzerPanel::LoadSelectionState()
    {
        if (m_selectionStatePath.empty())
            return;

        std::ifstream in(m_selectionStatePath, std::ios::binary);
        if (!in.is_open())
            return;

        Framework::json root = Framework::json::parse(in, nullptr, false);
        if (root.is_discarded() || !root.is_object())
            return;

        if (const auto it = root.find("selectedAssets");
            it != root.end() && it->is_array())
        {
            for (const auto& item : *it)
            {
                if (item.is_string())
                    m_selectedKeys.insert(item.get<std::string>());
            }
        }

        if (const auto it = root.find("lastExportDirectory");
            it != root.end() && it->is_string())
        {
            SetExportPathBuffer(std::filesystem::path(it->get<std::string>()));
        }
    }

    void BuildSizeAnalyzerPanel::SaveSelectionState() const
    {
        if (m_selectionStatePath.empty())
            return;

        std::error_code ec;
        std::filesystem::create_directories(m_selectionStatePath.parent_path(), ec);
        if (ec)
            return;

        Framework::json root = Framework::json::object();
        std::vector<std::string> selected(m_selectedKeys.begin(), m_selectedKeys.end());
        std::sort(selected.begin(), selected.end());
        root["selectedAssets"] = selected;

        const std::string lastExport = TrimCopy(m_exportBuffer.data());
        if (!lastExport.empty())
            root["lastExportDirectory"] = lastExport;

        std::ofstream out(m_selectionStatePath, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
            return;

        out << root.dump(2);
    }

    void BuildSizeAnalyzerPanel::DrawStatusLine() const
    {
        if (m_statusMessage.empty())
            return;

        const ImVec4 color = m_statusIsError
            ? ImVec4(0.95f, 0.35f, 0.35f, 1.0f)
            : ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
        ImGui::TextColored(color, "%s", m_statusMessage.c_str());
    }

    void BuildSizeAnalyzerPanel::DrawExportPopup()
    {
        if (!ImGui::BeginPopupModal(kExportPopupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        ImGui::TextWrapped("Enter a destination folder. Selected assets will be copied there and keep their project-relative folder structure.");
        ImGui::Text("Assets to Export: %zu", m_selectedCount);
        ImGui::Text("Combined Size: %s", PrettySize(m_totalSelectedBytes).c_str());
        ImGui::InputText("Destination Folder", m_exportBuffer.data(), m_exportBuffer.size());
        ImGui::SameLine();
        if (ImGui::Button("Browse In Editor"))
        {
            const std::string destinationText = TrimCopy(m_exportBuffer.data());
            OpenFolderBrowser(destinationText.empty()
                ? m_assetsRoot.parent_path()
                : std::filesystem::path(destinationText));
        }

        DrawFolderBrowserPopup();

        if (!m_exportError.empty())
        {
            ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "%s", m_exportError.c_str());
        }

        if (ImGui::Button("Export"))
        {
            const std::string rawDestination = TrimCopy(m_exportBuffer.data());
            if (rawDestination.empty())
            {
                m_exportError = "Provide a destination folder before exporting.";
            }
            else
            {
                std::size_t exportedCount = 0;
                std::size_t failedCount = 0;
                if (ExportSelectedAssets(rawDestination, exportedCount, failedCount))
                {
                    std::ostringstream message;
                    message << "Exported " << exportedCount
                        << (exportedCount == 1 ? " asset" : " assets")
                        << " to " << std::filesystem::path(rawDestination).string();
                    if (failedCount > 0)
                    {
                        message << " (" << failedCount
                            << (failedCount == 1 ? " failed copy)." : " failed copies).");
                        SetStatus(message.str(), true);
                    }
                    else
                    {
                        message << ".";
                        SetStatus(message.str(), false);
                    }

                    SaveSelectionState();
                    m_exportError.clear();
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    if (failedCount == 0)
                        m_exportError = "No assets are currently selected for export.";
                    else
                        m_exportError = "The export failed. Check the destination path and file permissions.";
                    SetStatus(m_exportError, true);
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_exportError.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    void BuildSizeAnalyzerPanel::DrawFolderBrowserPopup()
    {
        if (m_openFolderBrowserPopup)
        {
            ImGui::OpenPopup(kFolderBrowserPopupTitle);
            m_openFolderBrowserPopup = false;
        }

        if (!ImGui::BeginPopupModal(kFolderBrowserPopupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        bool closePopup = false;

        ImGui::TextWrapped("Choose a destination folder without leaving the editor.");
        ImGui::TextDisabled("Current Folder: %s", m_folderBrowserPath.string().c_str());

        const bool canGoUp = !m_folderBrowserPath.empty() &&
            m_folderBrowserPath.has_parent_path() &&
            m_folderBrowserPath.parent_path() != m_folderBrowserPath;
        ImGui::BeginDisabled(!canGoUp);
        if (ImGui::Button("Up"))
        {
            m_folderBrowserPath = m_folderBrowserPath.parent_path();
            m_folderBrowserSelection.clear();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Use Current Folder"))
        {
            SetExportPathBuffer(m_folderBrowserPath);
            SaveSelectionState();
            closePopup = true;
        }

        std::vector<std::filesystem::path> directories;
        std::error_code ec;
        if (!m_folderBrowserPath.empty() &&
            std::filesystem::exists(m_folderBrowserPath, ec) &&
            std::filesystem::is_directory(m_folderBrowserPath, ec))
        {
            std::filesystem::directory_iterator end;
            std::filesystem::directory_iterator it(
                m_folderBrowserPath,
                std::filesystem::directory_options::skip_permission_denied,
                ec);

            while (!ec && it != end)
            {
                std::error_code typeEc;
                if (it->is_directory(typeEc) && !typeEc)
                    directories.push_back(it->path());
                it.increment(ec);
            }
        }

        std::sort(directories.begin(), directories.end(),
            [](const std::filesystem::path& lhs, const std::filesystem::path& rhs) {
                return ToLower(lhs.filename().string()) < ToLower(rhs.filename().string());
            });

        ImGui::Separator();
        if (ImGui::BeginChild("ExportFolderBrowserList", ImVec2(520.0f, 260.0f), true))
        {
            if (directories.empty())
            {
                ImGui::TextDisabled("No subfolders available here.");
            }
            else
            {
                for (const auto& directory : directories)
                {
                    const bool isSelected = directory == m_folderBrowserSelection;
                    const std::string label = directory.filename().string().empty()
                        ? directory.string()
                        : directory.filename().string();
                    if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
                    {
                        m_folderBrowserSelection = directory;
                        if (ImGui::IsMouseDoubleClicked(0))
                        {
                            m_folderBrowserPath = directory;
                            m_folderBrowserSelection.clear();
                        }
                    }
                }
            }
        }
        ImGui::EndChild();

        const bool hasFolderSelection = !m_folderBrowserSelection.empty();
        bool chooseSelected = false;
        ImGui::BeginDisabled(!hasFolderSelection);
        if (ImGui::Button("Open Selected"))
        {
            m_folderBrowserPath = m_folderBrowserSelection;
            m_folderBrowserSelection.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Choose Selected"))
        {
            chooseSelected = true;
        }
        ImGui::EndDisabled();

        if (chooseSelected)
        {
            SetExportPathBuffer(m_folderBrowserSelection);
            SaveSelectionState();
            closePopup = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel Folder Picker"))
        {
            closePopup = true;
        }

        if (closePopup)
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    void BuildSizeAnalyzerPanel::OpenFolderBrowser(const std::filesystem::path& initialPath)
    {
        std::error_code ec;
        std::filesystem::path resolved = initialPath;
        if (resolved.empty())
            resolved = m_assetsRoot.parent_path();

        if (!resolved.empty() && !std::filesystem::exists(resolved, ec))
            resolved = resolved.parent_path();

        if (!resolved.empty() && !std::filesystem::is_directory(resolved, ec))
            resolved = resolved.parent_path();

        if (resolved.empty())
            resolved = m_assetsRoot.parent_path();

        auto canonical = std::filesystem::weakly_canonical(resolved, ec);
        m_folderBrowserPath = ec ? resolved : canonical;
        m_folderBrowserSelection.clear();
        m_openFolderBrowserPopup = true;
    }

    void BuildSizeAnalyzerPanel::SetStatus(const std::string& message, bool isError)
    {
        m_statusMessage = message;
        m_statusIsError = isError;
    }

    void BuildSizeAnalyzerPanel::SetExportPathBuffer(const std::filesystem::path& path)
    {
        m_exportBuffer.fill('\0');
        const std::string value = path.string();
        if (value.empty())
            return;

        const std::size_t maxCount = m_exportBuffer.size() - 1;
        const std::size_t count = std::min(maxCount, value.size());
        std::copy_n(value.data(), count, m_exportBuffer.data());
        m_exportBuffer[count] = '\0';
    }

    std::filesystem::path BuildSizeAnalyzerPanel::ResolveSelectionStatePath() const
    {
        std::filesystem::path saveRoot = Framework::GetCurrentSavesRoot();
        if (saveRoot.empty() && !m_assetsRoot.empty())
            saveRoot = m_assetsRoot.parent_path() / "Saves";

        if (saveRoot.empty())
            return {};

        return saveRoot / "Editor" / "build_size_analyzer_selection.json";
    }

    bool BuildSizeAnalyzerPanel::ExportSelectedAssets(const std::filesystem::path& destination,
        std::size_t& exportedCount,
        std::size_t& failedCount) const
    {
        exportedCount = 0;
        failedCount = 0;

        if (destination.empty() || m_selectedCount == 0)
            return false;

        std::error_code ec;
        std::filesystem::create_directories(destination, ec);
        if (ec)
        {
            ++failedCount;
            return false;
        }

        for (const auto& asset : m_assets)
        {
            if (!asset.selected)
                continue;

            const auto target = destination / asset.relativePath;
            ec.clear();
            std::filesystem::create_directories(target.parent_path(), ec);
            if (ec)
            {
                ++failedCount;
                continue;
            }

            ec.clear();
            std::filesystem::copy_file(asset.absolutePath,
                target,
                std::filesystem::copy_options::overwrite_existing,
                ec);
            if (ec)
            {
                ++failedCount;
                continue;
            }

            ++exportedCount;
        }

        return exportedCount > 0;
    }

    std::string BuildSizeAnalyzerPanel::PrettySize(std::uintmax_t size)
    {
        static constexpr const char* units[] = { "B", "KB", "MB", "GB", "TB" };

        double value = static_cast<double>(size);
        std::size_t unitIndex = 0;
        while (value >= 1024.0 && unitIndex + 1 < std::size(units))
        {
            value /= 1024.0;
            ++unitIndex;
        }

        std::ostringstream out;
        out << std::fixed << std::setprecision(value >= 100.0 ? 0 : 2)
            << value << ' ' << units[unitIndex];
        return out.str();
    }

    std::string BuildSizeAnalyzerPanel::ToLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return value;
    }

    std::string BuildSizeAnalyzerPanel::TrimCopy(std::string value)
    {
        auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
        value.erase(value.begin(), std::find_if(value.begin(), value.end(),
            [&](unsigned char ch) { return !isSpace(ch); }));
        value.erase(std::find_if(value.rbegin(), value.rend(),
            [&](unsigned char ch) { return !isSpace(ch); }).base(), value.end());
        return value;
    }

    std::string BuildSizeAnalyzerPanel::PathKey(const std::filesystem::path& relativePath)
    {
        return relativePath.generic_string();
    }

} // namespace mygame

#endif // SOFASPUDS_ENABLE_EDITOR
