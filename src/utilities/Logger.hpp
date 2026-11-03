#pragma once

#include <string>
#include <iostream>
#include <mutex>

namespace tally::utilities {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static void init(const std::string& logFilePath = "");
    static void log(LogLevel level, const std::string& message);
    static void debug(const std::string& message) { log(LogLevel::Debug, message); }
    static void info(const std::string& message) { log(LogLevel::Info, message); }
    static void warn(const std::string& message) { log(LogLevel::Warning, message); }
    static void error(const std::string& message) { log(LogLevel::Error, message); }

private:
    static std::mutex s_mutex;
    static std::string s_logFilePath;
};

} // namespace tally::utilities
