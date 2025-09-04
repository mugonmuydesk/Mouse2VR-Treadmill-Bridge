#include "common/Logger.h"
#include "core/PathUtils.h"
#include <filesystem>
#include <iostream>

#ifdef _WIN32
#include "common/WindowsHeaders.h"
#endif

namespace Mouse2VR {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::Initialize(const std::string& logPath, bool useExeRelative) {
    try {
        std::string actualPath = logPath;
        if (useExeRelative) {
            actualPath = PathUtils::GetExecutablePath(logPath);
        }
        
        // Create logs directory if it doesn't exist
        std::filesystem::path logDir = std::filesystem::path(actualPath).parent_path();
        if (!logDir.empty()) {
            std::filesystem::create_directories(logDir);
        }
        
        // Create async logger with thread pool
        spdlog::init_thread_pool(8192, 1); // 8KB queue, 1 background thread
        
        // Create sinks
        auto sinks = std::vector<spdlog::sink_ptr>();
        
        // Rotating file sink: 5MB x 3 files
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            actualPath, 5 * 1024 * 1024, 3);
        file_sink->set_level(spdlog::level::debug);
        sinks.push_back(file_sink);
        
        // Console sink for debug builds
        #ifdef _DEBUG
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        sinks.push_back(console_sink);
        #endif
        
        // Create async logger
        m_logger = std::make_shared<spdlog::async_logger>(
            "mouse2vr", 
            sinks.begin(), 
            sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::overrun_oldest);
        
        // Set pattern: [timestamp] [level] [component] message
        m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%v]");
        m_logger->set_level(spdlog::level::debug);
        
        // Register as default logger
        spdlog::register_logger(m_logger);
        
        Log(INFO, "Logger", "=== Logger Initialized ===");
        Log(INFO, "Logger", "Log file: " + actualPath);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
    }
}

void Logger::SetSettingsProvider(std::function<std::string()> provider) {
    m_settingsProvider = provider;
}

void Logger::Log(Level level, const std::string& component, const std::string& message) {
    if (!m_logger) {
        std::cerr << "[" << component << "] " << message << std::endl;
        return;
    }
    
    // Rate limit "running behind" messages
    if (level == DEBUG && message.find("running behind") != std::string::npos) {
        if (!ShouldRateLimit(level, message)) {
            return;
        }
    }
    
    // Format message with component
    std::string formatted = component + "] " + message;
    
    // Append current settings if provider is set
    if (m_settingsProvider) {
        try {
            std::string settings = m_settingsProvider();
            if (!settings.empty()) {
                formatted += " [" + settings + "]";
            }
        } catch (...) {
            // Ignore errors in settings provider to avoid recursive logging issues
        }
    }
    
    switch (level) {
        case DEBUG:
            m_logger->debug(formatted);
            break;
        case INFO:
            m_logger->info(formatted);
            break;
        case WARNING:
            m_logger->warn(formatted);
            break;
        case ERROR_LEVEL:
            m_logger->error(formatted);
            break;
    }
}

void Logger::LogWithData(Level level, const std::string& component, const std::string& message,
                         const std::string& key1, const std::string& value1) {
    std::string fullMsg = message + " | " + key1 + "=" + value1;
    Log(level, component, fullMsg);
}

void Logger::LogWithData(Level level, const std::string& component, const std::string& message,
                         const std::string& key1, const std::string& value1,
                         const std::string& key2, const std::string& value2) {
    std::string fullMsg = message + " | " + key1 + "=" + value1 + ", " + key2 + "=" + value2;
    Log(level, component, fullMsg);
}

void Logger::Flush() {
    if (m_logger) {
        m_logger->flush();
    }
}

void Logger::Close() {
    if (m_logger) {
        Log(INFO, "Logger", "=== Logger Closing ===");
        m_logger->flush();
        spdlog::shutdown();
    }
}

Logger::~Logger() {
    Close();
}

spdlog::level::level_enum Logger::ConvertLevel(Level level) {
    switch (level) {
        case DEBUG: return spdlog::level::debug;
        case INFO: return spdlog::level::info;
        case WARNING: return spdlog::level::warn;
        case ERROR_LEVEL: return spdlog::level::err;
        default: return spdlog::level::info;
    }
}

bool Logger::ShouldRateLimit(Level level, const std::string& message) {
    auto now = std::chrono::steady_clock::now();
    if (now - m_lastWarningTime >= WARNING_RATE_LIMIT) {
        m_lastWarningTime = now;
        return true;
    }
    return false;
}

// ScopedTimer implementation
ScopedTimer::ScopedTimer(const std::string& name) : m_name(name) {
    m_start = std::chrono::high_resolution_clock::now();
}

ScopedTimer::~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
    LOG_DEBUG("Performance", m_name + " took " + std::to_string(duration.count()) + " us");
}

} // namespace Mouse2VR