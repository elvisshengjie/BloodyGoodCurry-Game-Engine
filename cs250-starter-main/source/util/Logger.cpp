/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "Logger.hpp"

#include "util/Directory.hpp"
#include "window/Environment.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gsl/gsl>
#include <iostream>
#include <sstream>
#include <version>

namespace
{
    constexpr auto to_string(util::LogSeverity severity) noexcept
    {
        switch (severity)
        {
            case util::LogSeverity::Verbose: return "VERBOSE";
            case util::LogSeverity::Debug: return "DEBUG";
            case util::LogSeverity::Event: return "EVENT";
            case util::LogSeverity::Error: return "ERROR";
        }
        return "Unknown";
    }

    std::tm as_local(std::chrono::system_clock::time_point const tp)
    {
        std::time_t time = std::chrono::system_clock::to_time_t(tp);
        std::tm     local_time;
#ifdef _MSC_VER
        const auto err_code = localtime_s(&local_time, &time); // Use the safe version for MSVC
        assert(err_code == 0);
        if (err_code != 0) [[unlikely]]
        {
            // Initialize local_time to a default value (e.g., epoch time)
            local_time.tm_year = 70; // Years since 1900
            local_time.tm_mon  = 0;  // Month (0 = January)
            local_time.tm_mday = 1;  // Day of the month
            local_time.tm_hour = 0;
            local_time.tm_min  = 0;
            local_time.tm_sec  = 0;
        }
#else
        local_time = *std::localtime(&time); // Use the standard version for GNU/Clang
#endif
        return local_time;
    }

    std::string to_string(std::tm tp)
    {
        std::ostringstream oss;
        oss << std::put_time(&tp, "%F %r %Z");
        return oss.str();
    }

    auto to_string(std::source_location const source)
    {
        return std::format("{}({}) : {}", std::filesystem::path(source.file_name()).filename().string(), source.line(), source.function_name());
    }
}

namespace util::details
{
    void log_information(LogSeverity severity, const std::source_location& caller_location, const std::string& message) noexcept
    {
        static std::ofstream TraceLogFile{ util::get_writable_app_directory() / "Trace.log" };
        TraceLogFile << std::format(
            "[{:^7}] {}\tElapsedTime {:.4f}s\tFrameCount {:^19}\n\t{}\n{}\n", to_string(severity), to_string(as_local(std::chrono::system_clock::now())), window::ElapsedTime, window::FrameCount,
            to_string(caller_location), message);
        if constexpr (LOG_TO_CONSOLE)
        {
            switch (severity)
            {
                case LogSeverity::Verbose:
                case LogSeverity::Debug:
                case LogSeverity::Event: std::cout << std::format("[{:^7}] {}\n", to_string(severity), message); break;
                case LogSeverity::Error: std::cerr << std::format("[{:^7}] {} {}\n", to_string(severity), to_string(caller_location), message); break;
            }
        }
    }
}
