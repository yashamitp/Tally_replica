#include "Logger.hpp"
#include "DateUtils.hpp"
#include <fstream>

namespace tally::utilities {

std::mutex Logger::s_mutex;
std::string Logger::s_logFilePath;

void Logger::init(const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_logFilePath = logFilePath;
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(s_mutex);

    const char* levelStr = "INFO";
    switch (level) {
        case LogLevel::Debug: levelStr = "DEBUG"; break;
        case LogLevel::Info: levelStr = "INFO"; break;
        case LogLevel::Warning: levelStr = "WARN"; break;
        case LogLevel::Error: levelStr = "ERROR"; break;
    }

    std::string timestamp = DateUtils::currentIsoDateTime();
    std::string line = "[" + timestamp + "] [" + levelStr + "] " + message;

    std::cout << line << std::endl;

    if (!s_logFilePath.empty()) {
        std::ofstream ofs(s_logFilePath, std::ios::app);
        if (ofs.is_open()) {
            ofs << line << "\n";
        }
    }
}

} // namespace tally::utilities
