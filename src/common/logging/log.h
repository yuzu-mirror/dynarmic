// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

namespace Log {

/// Specifies the severity or level of detail of the log message.
enum class Level : u8 {
    Trace,    ///< Extremely detailed and repetitive debugging information that is likely to
    ///  pollute logs.
            Debug,    ///< Less detailed debugging information.
    Info,     ///< Status information from important points during execution.
    Warning,  ///< Minor or potential problems found during execution of a task.
    Error,    ///< Major problems found during execution of a task that prevent it from being
    ///  completed.
            Critical, ///< Major problems during execution that threathen the stability of the entire
    ///  application.

    Count ///< Total number of logging levels
};

typedef u8 ClassType;

/**
 * Specifies the sub-system that generated the log message.
 *
 * @note If you add a new entry here, also add a corresponding one to `ALL_LOG_CLASSES` in backend.cpp.
 */
enum class Class : ClassType {
    Log,
    Common,
    Common_Memory,
    Debug,
    Count ///< Total number of logging classes
};

/// Logs a message to the global logger.
void LogMessage(Class log_class, Level log_level,
                const char* filename, unsigned int line_nr, const char* function,
#ifdef _MSC_VER
        _Printf_format_string_
#endif
                const char* format, ...)
#ifdef __GNUC__
        __attribute__((format(gnu_printf, 6, 7)))
#endif
;

} // namespace Log

#define LOG_GENERIC(log_class, log_level, ...) \
    ::Log::LogMessage(log_class, log_level, __FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef _DEBUG
#define LOG_TRACE(   log_class, ...) LOG_GENERIC(::Log::Class::log_class, ::Log::Level::Trace,    __VA_ARGS__)
#else
#define LOG_TRACE(   log_class, ...) (void(0))
#endif

#define LOG_DEBUG(   log_class, ...) LOG_GENERIC(::Log::Class::log_class, ::Log::Level::Debug,    __VA_ARGS__)
#define LOG_INFO(    log_class, ...) LOG_GENERIC(::Log::Class::log_class, ::Log::Level::Info,     __VA_ARGS__)
#define LOG_WARNING( log_class, ...) LOG_GENERIC(::Log::Class::log_class, ::Log::Level::Warning,  __VA_ARGS__)
#define LOG_ERROR(   log_class, ...) LOG_GENERIC(::Log::Class::log_class, ::Log::Level::Error,    __VA_ARGS__)
#define LOG_CRITICAL(log_class, ...) LOG_GENERIC(::Log::Class::log_class, ::Log::Level::Critical, __VA_ARGS__)
