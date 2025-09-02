#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#endif

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <iostream>

namespace Mouse2VR {

class Logger {
public:
    enum Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR_LEVEL  // Renamed from ERROR to avoid Windows.h macro conflict
    };

    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    void Initialize(const std::string& logPath = "logs/debug.log") {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Create logs directory if it doesn't exist
        std::filesystem::path logDir = std::filesystem::path(logPath).parent_path();
        if (!logDir.empty()) {
            std::filesystem::create_directories(logDir);
        }
        
        // Open log file
        m_file.open(logPath, std::ios::app);
        if (m_file.is_open()) {
            Log(INFO, "Logger", "=== Logger Initialized ===");
            Log(INFO, "Logger", "Log file: " + logPath);
        }
    }

    void Log(Level level, const std::string& component, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::stringstream ss;
        ss << "[" << GetTimestamp() << "] "
           << "[" << LevelToString(level) << "] "
           << "[" << component << "] "
           << message;
        
        std::string logLine = ss.str();
        
        // Write to file
        if (m_file.is_open()) {
            m_file << logLine << std::endl;
            m_file.flush();
        }
        
        // Console output for console builds
        #ifndef WINUI_BUILD
        if (level >= INFO) {
            std::cout << logLine << std::endl;
        }
        #endif
        
        // Output to debug console in Debug builds
        #ifdef _DEBUG
        #ifdef _WIN32
        OutputDebugStringA((logLine + "\n").c_str());
        #endif
        #endif
    }

    void LogWithData(Level level, const std::string& component, const std::string& message,
                     const std::string& key1, const std::string& value1) {
        std::stringstream ss;
        ss << message << " | " << key1 << "=" << value1;
        Log(level, component, ss.str());
    }

    void LogWithData(Level level, const std::string& component, const std::string& message,
                     const std::string& key1, const std::string& value1,
                     const std::string& key2, const std::string& value2) {
        std::stringstream ss;
        ss << message << " | " << key1 << "=" << value1 << ", " << key2 << "=" << value2;
        Log(level, component, ss.str());
    }

    void Flush() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            m_file.flush();
        }
    }

    void Close() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            Log(INFO, "Logger", "=== Logger Closing ===");
            m_file.close();
        }
    }

    ~Logger() {
        Close();
    }

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream m_file;
    std::mutex m_mutex;
    
    std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    const char* LevelToString(Level level) {
        switch(level) {
            case DEBUG: return "DEBUG";
            case INFO: return "INFO";
            case WARNING: return "WARN";
            case ERROR_LEVEL: return "ERROR";
        }
        return "UNKNOWN";
    }
};

// Convenience macros
#define LOG_DEBUG(component, msg) Mouse2VR::Logger::Instance().Log(Mouse2VR::Logger::DEBUG, component, msg)
#define LOG_INFO(component, msg) Mouse2VR::Logger::Instance().Log(Mouse2VR::Logger::INFO, component, msg)
#define LOG_WARNING(component, msg) Mouse2VR::Logger::Instance().Log(Mouse2VR::Logger::WARNING, component, msg)
#define LOG_ERROR(component, msg) Mouse2VR::Logger::Instance().Log(Mouse2VR::Logger::ERROR_LEVEL, component, msg)

// Convenience macros with data
#define LOG_DEBUG_DATA(component, msg, key, value) \
    Mouse2VR::Logger::Instance().LogWithData(Mouse2VR::Logger::DEBUG, component, msg, key, std::to_string(value))
#define LOG_INFO_DATA(component, msg, key, value) \
    Mouse2VR::Logger::Instance().LogWithData(Mouse2VR::Logger::INFO, component, msg, key, std::to_string(value))

// Performance timing helper
class ScopedTimer {
public:
    ScopedTimer(const std::string& name) : m_name(name) {
        m_start = std::chrono::high_resolution_clock::now();
    }
    
    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
        LOG_DEBUG("Performance", m_name + " took " + std::to_string(duration.count()) + " us");
    }
    
private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
};

#ifdef ENABLE_PROFILING
#define SCOPED_TIMER(name) Mouse2VR::ScopedTimer _timer(name)
#else
#define SCOPED_TIMER(name)
#endif

} // namespace Mouse2VR