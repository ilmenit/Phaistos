/**
 * @file logger.hpp
 * @brief Simple logging system for Phaistos
 */
#pragma once

#include "common.hpp"

namespace phaistos {

/**
 * @class Logger
 * @brief Singleton logger class for Phaistos
 */
class Logger {
public:
    /**
     * @enum LogLevel
     * @brief Logging level enumeration
     */
    enum LogLevel {
        ERROR,  // Only error messages
        INFO,   // Error and info messages
        WARNING, // Warning messages
        DEBUG   // All messages
    };

    /**
     * @brief Get the singleton instance
     * @return Reference to the logger instance
     */
    static Logger& getInstance();

    /**
     * @brief Set the logging level
     * @param level The logging level to set
     */
    void setLevel(LogLevel level);

    /**
     * @brief Set whether to show extended info (timestamps, log level)
     * @param enabled Whether extended info is enabled
     */
    void setExtendedInfo(bool enabled);

    /**
     * @brief Set the output stream for the logger
     * @param os Output stream to use
     */
    void setOutputStream(std::ostream& os);

    /**
     * @brief Log a debug message
     * @param message Message to log
     */
    void debug(const std::string& message);

    /**
     * @brief Log an info message
     * @param message Message to log
     */
    void info(const std::string& message);


    /**
     * @brief Log an warning message
     * @param message Message to log
     */
    void warning(const std::string& message);

    /**
     * @brief Log an error message
     * @param message Message to log
     */
    void error(const std::string& message);

    /**
     * @brief Get the current log level
     * @return Current log level
     */
    LogLevel getLevel() const;

    /**
     * @brief Check if a log level is enabled
     * @param level Level to check
     * @return True if the level is enabled
     */
    bool isLevelEnabled(LogLevel level) const;

    /**
     * @brief Convert a string to a log level
     * @param levelStr String representation of level
     * @return LogLevel enum value
     */
    static LogLevel stringToLevel(const std::string& levelStr);

    /**
     * @brief Convert a log level to a string
     * @param level Log level to convert
     * @return String representation of the level
     */
    static std::string levelToString(LogLevel level);

private:
    // Private constructor for singleton
    Logger();

    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Internal log method
     * @param level Log level
     * @param message Message to log
     */
    void log(LogLevel level, const std::string& message);

    /**
     * @brief Get the current timestamp as a string
     * @return Formatted timestamp
     */
    std::string getTimestamp() const;

    // Logger state
    LogLevel current_level;
    bool extended_info;
    std::ostream* out_stream;
};

/**
 * @brief Convenience function to get logger instance
 * @return Reference to the logger instance
 */
Logger& getLogger();

} // namespace phaistos
