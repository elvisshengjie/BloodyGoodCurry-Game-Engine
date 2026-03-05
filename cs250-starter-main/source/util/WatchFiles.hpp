/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once


#include <filesystem>
#include <functional>
#include <memory>

namespace util
{
    enum class FileStatus
    {
        Created,
        Modified,
        Erased
    };

    class WatchFiles
    {
    public:
        using Callback = std::function<void(FileStatus status)>;
        explicit WatchFiles(std::chrono::milliseconds the_delay = std::chrono::milliseconds(500));
        void Watch(const std::filesystem::path& file_path, const Callback& notify_changed);
        void Update();

    public:
        WatchFiles(const WatchFiles&)            = delete;
        WatchFiles& operator=(const WatchFiles&) = delete;
        WatchFiles(WatchFiles&&) noexcept;
        WatchFiles& operator=(WatchFiles&&) noexcept;
        ~WatchFiles();

    private:
        struct Implementation;
        class WatchFilesThreaded;
        class WatchFilesNoThreads;
        std::unique_ptr<Implementation> impl;
    };
}
