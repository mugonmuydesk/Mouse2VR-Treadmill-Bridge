#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>
#include <chrono>

namespace Mouse2VR {

class Logger {
public:
    enum Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR_LEVEL
    };

    static Logger& Instance();

    void Initialize(const std::string& logPath = "logs/debug.log", bool useExeRelative = true);
    
    void Log(Level level, const std::string& component, const std::string& message);
    
    void LogWithData(Level level, const std::string& component, const std::string& message,
                     const std::string& key1, const std::string& value1);
    
    void LogWithData(Level level, const std::string& component, const std::string& message,
                     const std::string& key1, const std::string& value1,
                     const std::string& key2, const std::string& value2);
    
    void Flush();
    void Close();
    
    ~Logger();

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::shared_ptr<spdlog::logger> m_logger;
    std::chrono::steady_clock::time_point m_lastWarningTime;
    static constexpr auto WARNING_RATE_LIMIT = std::chrono::seconds(1);
    
    spdlog::level::level_enum ConvertLevel(Level level);
    bool ShouldRateLimit(Level level, const std::string& message);
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
    ScopedTimer(const std::string& name);
    ~ScopedTimer();
    
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