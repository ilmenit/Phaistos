/**
 * @file logger.cpp
 * @brief Implementation of the Logger class
 */
#include "logger.hpp"
#include <chrono>
#include <iomanip>

namespace phaistos {

    Logger::Logger()
        : current_level(INFO), extended_info(false), out_stream(&std::cout) {
    }

    Logger& Logger::getInstance() {
        static Logger instance;
        return instance;
    }

    void Logger::setLevel(LogLevel level) {
        current_level = level;
    }

    void Logger::setExtendedInfo(bool enabled) {
        extended_info = enabled;
    }

    void Logger::setOutputStream(std::ostream& os) {
        out_stream = &os;
    }

    void Logger::debug(const std::string& message) {
        log(DEBUG, message);
    }

    void Logger::info(const std::string& message) {
        log(INFO, message);
    }

    void Logger::warning(const std::string& message) {
        log(WARNING, message);
    }

    void Logger::error(const std::string& message) {
        log(ERROR, message);
    }

    Logger::LogLevel Logger::getLevel() const {
        return current_level;
    }

    bool Logger::isLevelEnabled(LogLevel level) const {
        // Compare based on severity - higher values are more verbose
        return level <= current_level;
    }

    void Logger::log(LogLevel level, const std::string& message) {
        if (!isLevelEnabled(level)) {
            return;
        }

        std::ostringstream oss;

        if (extended_info) {
            oss << "[" << getTimestamp() << "] ";
            oss << "[" << levelToString(level) << "] ";
        }

        oss << message;

        *out_stream << oss.str() << std::endl;
    }

    std::string Logger::getTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

        return oss.str();
    }

    Logger::LogLevel Logger::stringToLevel(const std::string& levelStr) {
        std::string level_lower = levelStr;
        std::transform(level_lower.begin(), level_lower.end(), level_lower.begin(), ::tolower);

        if (level_lower == "debug") return DEBUG;
        if (level_lower == "info") return INFO;
        if (level_lower == "warning") return WARNING;
        if (level_lower == "error") return ERROR;

        // Default to INFO if unknown
        return INFO;
    }

    std::string Logger::levelToString(LogLevel level) {
        switch (level) {
        case DEBUG: return "DEBUG";
        case INFO: return "INFO";
        case WARNING: return "WARNING";
        case ERROR: return "ERROR";
        default: return "UNKNOWN";
        }
    }

    Logger& getLogger() {
        return Logger::getInstance();
    }

} // namespace phaistos