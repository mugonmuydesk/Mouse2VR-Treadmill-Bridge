#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "processing/InputProcessor.h"

namespace Mouse2VR {

struct AppConfig {
    // Processing settings
    float sensitivity = 1.0f;
    float deadzone = 0.0f;
    bool invertX = false;
    bool invertY = false;
    bool lockX = true;  // Default: lock X for treadmill
    bool lockY = false;
    float maxSpeed = 1.0f;
    float countsPerMeter = 1000.0f;
    
    // Update settings
    int updateIntervalMs = 20;  // 50Hz default
    bool adaptiveMode = false;  // Switch between high/low update rates
    int idleUpdateIntervalMs = 33;  // ~30Hz when idle
    
    // Debug settings
    bool showDebugInfo = true;
    bool logToFile = false;
    std::string logFilePath = "mouse2vr.log";
    
    // Convert to ProcessingConfig
    ProcessingConfig toProcessingConfig() const {
        ProcessingConfig config;
        config.sensitivity = sensitivity;
        config.deadzone = deadzone;
        config.invertX = invertX;
        config.invertY = invertY;
        config.lockX = lockX;
        config.lockY = lockY;
        config.maxSpeed = maxSpeed;
        config.countsPerMeter = countsPerMeter;
        return config;
    }
};

class ConfigManager {
public:
    ConfigManager(const std::string& configPath = "config.json");
    
    // Load configuration from file
    bool Load();
    
    // Save current configuration to file
    bool Save() const;
    
    // Get current configuration
    const AppConfig& GetConfig() const { return m_config; }
    AppConfig& GetConfig() { return m_config; }
    
    // Set configuration
    void SetConfig(const AppConfig& config) { m_config = config; }
    
    // Create default config file if it doesn't exist
    bool CreateDefaultConfig() const;
    
private:
    std::string m_configPath;
    AppConfig m_config;
    
    // JSON serialization
    static nlohmann::json ConfigToJson(const AppConfig& config);
    static AppConfig JsonToConfig(const nlohmann::json& j);
};

} // namespace Mouse2VR