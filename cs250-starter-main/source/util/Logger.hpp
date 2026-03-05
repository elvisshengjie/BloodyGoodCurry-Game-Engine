/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 *
 * This header provides a logging utility for the application. The logging functionality is categorized into
 * different levels of severity: Verbose, Debug, Event, and Error. The logging level is controlled by compile-time
 * configuration (e.g., DEVELOPER_VERSION).
 *
 * Usage:
 * - Include this header in your source file.
 * - Use `util::log_verbose`, `util::log_debug`, `util::log_event`, and `util::log_error` to log messages
 *   at different severity levels.
 * - These log functions accept a format string followed by a variable number of arguments, similar to `std::format`.
 *
 * Example:
 * \code
 * util::log_verbose("\n{} {} {} {}", 5, 'A', 3.14f, "fun");
 * util::log_verbose("Hello World");
 * util::log_debug("\tDebug check {}", "check");
 * util::log_event("event check {}", "check");
 * util::log_error("error check {}", "check");
 * \endcode
 *
 * The output will vary depending on the LOG_LEVEL defined during compilation.
 */

#pragma once

#include <format>
#include <source_location>

namespace util
{
    enum class LogSeverity
    {
        Verbose, ///< Minor messages, most detailed logs.
        Debug,   ///< Debug messages, useful during development.
        Event,   ///< General events, such as state changes.
        Error    ///< Error messages, for reporting issues and failures.
    };

    // Compile-time configuration for logging level and console output
#if defined(DEVELOPER_VERSION)
    constexpr LogSeverity LOG_LEVEL      = LogSeverity::Verbose;
    constexpr bool        LOG_TO_CONSOLE = true;
#else
    constexpr LogSeverity LOG_LEVEL = LogSeverity::Event;
#    if defined(__EMSCRIPTEN__)
    constexpr bool LOG_TO_CONSOLE = true;
#    else
    constexpr bool LOG_TO_CONSOLE = false;
#    endif
#endif

    /**
     * \brief Internal code section. Users should not depend on this code!
     */
    namespace details
    {

        void log_information(LogSeverity severity, const std::source_location& caller_location, const std::string& message) noexcept;

        template <LogSeverity S, typename... T>
        struct logger_helper
        {
            logger_helper(
                [[maybe_unused]] const std::format_string<T...>& fs, [[maybe_unused]] T&&... ts, [[maybe_unused]] const std::source_location& caller_location = std::source_location::current())
            {
                if constexpr (LOG_LEVEL <= S)
                {
                    log_information(S, caller_location, std::format(fs, std::forward<decltype(ts)>(ts)...));
                }
            }
        };

        // need deduction guides to get the format arguments and the source location
        // https://stackoverflow.com/questions/66376553/using-std-source-location-to-create-variadic-template-based-logging-function

        template <typename... T>
        struct verbose_helper : public logger_helper<LogSeverity::Verbose, T...>
        {
            using logger_helper<LogSeverity::Verbose, T...>::logger_helper;
        };

        template <typename... T>
        verbose_helper(const std::format_string<T...>&, T&&...) -> verbose_helper<T...>;

        template <typename... T>
        struct debug_helper : public logger_helper<LogSeverity::Debug, T...>
        {
            using logger_helper<LogSeverity::Debug, T...>::logger_helper;
        };

        template <typename... T>
        debug_helper(const std::format_string<T...>&, T&&...) -> debug_helper<T...>;

        template <typename... T>
        struct event_helper : public logger_helper<LogSeverity::Event, T...>
        {
            using logger_helper<LogSeverity::Event, T...>::logger_helper;
        };

        template <typename... T>
        event_helper(const std::format_string<T...>&, T&&...) -> event_helper<T...>;

        template <typename... T>
        struct error_helper : public logger_helper<LogSeverity::Error, T...>
        {
            using logger_helper<LogSeverity::Error, T...>::logger_helper;
        };

        template <typename... T>
        error_helper(const std::format_string<T...>&, T&&...) -> error_helper<T...>;
    }

    /** \brief Public interface for logging at different severity levels. Users should use this code. */

    /**
     * \brief Logs a verbose message.
     * \details This is the most detailed level of logging, used for general information.
     *
     * \tparam T Variadic template arguments for the format string.
     *
     * Example:
     * \code
     * util::log_verbose("Loading configuration: {} with value: {}", "config_name", value);
     * \endcode
     */
    template <typename... T>
    using log_verbose = details::verbose_helper<T...>;

    /**
     * \brief Logs a debug message.
     * \details This level is useful during development to trace and debug code.
     *
     * \tparam T Variadic template arguments for the format string.
     *
     * Example:
     * \code
     * util::log_debug("Variable x has value: {}", x);
     * \endcode
     */
    template <typename... T>
    using log_debug = details::debug_helper<T...>;

    /**
     * \brief Logs an event message.
     * \details Use this level to log general events such as user actions or state changes.
     *
     * \tparam T Variadic template arguments for the format string.
     *
     * Example:
     * \code
     * util::log_event("User clicked on button {}", button_name);
     * \endcode
     */
    template <typename... T>
    using log_event = details::event_helper<T...>;

    /**
     * \brief Logs an error message.
     * \details Use this level to log errors or critical issues in the application.
     *
     * \tparam T Variadic template arguments for the format string.
     *
     * Example:
     * \code
     * util::log_error("Failed to open file: {}", file_name);
     * \endcode
     */
    template <typename... T>
    using log_error = details::error_helper<T...>;

}
