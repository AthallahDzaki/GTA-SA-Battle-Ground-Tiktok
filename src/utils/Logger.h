#pragma once

#include <string>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace BattleGround {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static Logger& GetInstance() {
        static Logger instance;
        return instance;
    }

    void Initialize(const std::string& filename = "BattleGround.log") {
        if (m_file.is_open()) {
            m_file.close();
        }
        m_file.open(filename, std::ios::out | std::ios::trunc);
        m_initialized = m_file.is_open();
        if (m_initialized) {
            Log(LogLevel::INFO, "Logger initialized");
        }
    }

    void Shutdown() {
        if (m_file.is_open()) {
            Log(LogLevel::INFO, "Logger shutting down");
            m_file.close();
        }
        m_initialized = false;
    }

    void SetMinLevel(LogLevel level) {
        m_minLevel = level;
    }

    void Log(LogLevel level, const std::string& message) {
        if (!m_initialized || level < m_minLevel) {
            return;
        }

        std::string timestamp = GetTimestamp();
        std::string levelStr = GetLevelString(level);
        
        std::ostringstream oss;
        oss << "[" << timestamp << "] [" << levelStr << "] " << message;
        
        m_file << oss.str() << std::endl;
        m_file.flush();
    }

    void Debug(const std::string& message) { Log(LogLevel::DEBUG, message); }
    void Info(const std::string& message) { Log(LogLevel::INFO, message); }
    void Warning(const std::string& message) { Log(LogLevel::WARNING, message); }
    void Error(const std::string& message) { Log(LogLevel::ERROR, message); }

private:
    Logger() : m_initialized(false), m_minLevel(LogLevel::DEBUG) {}
    ~Logger() { Shutdown(); }
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string GetTimestamp() {
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(4) << (1900 + timeinfo->tm_year) << "-"
            << std::setw(2) << (1 + timeinfo->tm_mon) << "-"
            << std::setw(2) << timeinfo->tm_mday << " "
            << std::setw(2) << timeinfo->tm_hour << ":"
            << std::setw(2) << timeinfo->tm_min << ":"
            << std::setw(2) << timeinfo->tm_sec;
        
        return oss.str();
    }

    std::string GetLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR:   return "ERROR";
            default:                return "UNKNOWN";
        }
    }

    std::ofstream m_file;
    bool m_initialized;
    LogLevel m_minLevel;
};

// Convenience macros
#define LOG_DEBUG(msg) BattleGround::Logger::GetInstance().Debug(msg)
#define LOG_INFO(msg) BattleGround::Logger::GetInstance().Info(msg)
#define LOG_WARNING(msg) BattleGround::Logger::GetInstance().Warning(msg)
#define LOG_ERROR(msg) BattleGround::Logger::GetInstance().Error(msg)

} // namespace BattleGround
