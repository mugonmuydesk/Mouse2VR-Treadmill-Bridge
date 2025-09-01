#include "config/ConfigManager.h"
#include <fstream>
#include <iostream>

namespace Mouse2VR {

ConfigManager::ConfigManager(const std::string& configPath) 
    : m_configPath(configPath) {
}

bool ConfigManager::Load() {
    std::ifstream file(m_configPath);
    if (!file.is_open()) {
        std::cout << "Config file not found, creating default config at " << m_configPath << "\n";
        return CreateDefaultConfig();
    }
    
    try {
        nlohmann::json j;
        file >> j;
        
        std::lock_guard<std::mutex> lock(m_configMutex);
        m_config = JsonToConfig(j);
        
        std::cout << "Configuration loaded from " << m_configPath << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse config file: " << e.what() << "\n";
        std::cerr << "Using default configuration\n";
        
        std::lock_guard<std::mutex> lock(m_configMutex);
        m_config = AppConfig{};
        
        return false;
    }
}

bool ConfigManager::Save() {
    try {
        std::ofstream file(m_configPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file for writing: " << m_configPath << "\n";
            return false;
        }
        
        std::lock_guard<std::mutex> lock(m_configMutex);
        nlohmann::json j = ConfigToJson(m_config);
        file << j.dump(4);  // Pretty print with 4 spaces
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save config: " << e.what() << "\n";
        return false;
    }
}

bool ConfigManager::CreateDefaultConfig() const {
    AppConfig defaultConfig;
    
    try {
        std::ofstream file(m_configPath);
        if (!file.is_open()) {
            return false;
        }
        
        nlohmann::json j = ConfigToJson(defaultConfig);
        file << j.dump(4);
        std::cout << "Created default configuration file\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create default config: " << e.what() << "\n";
        return false;
    }
}

nlohmann::json ConfigManager::ConfigToJson(const AppConfig& config) {
    return nlohmann::json{
        {"processing", {
            {"sensitivity", config.sensitivity},
            {"deadzone", config.deadzone},
            {"invertX", config.invertX},
            {"invertY", config.invertY},
            {"lockX", config.lockX},
            {"lockY", config.lockY},
            {"maxSpeed", config.maxSpeed},
            {"countsPerMeter", config.countsPerMeter}
        }},
        {"update", {
            {"updateIntervalMs", config.updateIntervalMs},
            {"adaptiveMode", config.adaptiveMode},
            {"idleUpdateIntervalMs", config.idleUpdateIntervalMs}
        }},
        {"debug", {
            {"showDebugInfo", config.showDebugInfo},
            {"logToFile", config.logToFile},
            {"logFilePath", config.logFilePath}
        }}
    };
}

AppConfig ConfigManager::JsonToConfig(const nlohmann::json& j) {
    AppConfig config;
    
    // Processing settings
    if (j.contains("processing")) {
        auto& proc = j["processing"];
        if (proc.contains("sensitivity")) config.sensitivity = proc["sensitivity"];
        if (proc.contains("deadzone")) config.deadzone = proc["deadzone"];
        if (proc.contains("invertX")) config.invertX = proc["invertX"];
        if (proc.contains("invertY")) config.invertY = proc["invertY"];
        if (proc.contains("lockX")) config.lockX = proc["lockX"];
        if (proc.contains("lockY")) config.lockY = proc["lockY"];
        if (proc.contains("maxSpeed")) config.maxSpeed = proc["maxSpeed"];
        if (proc.contains("countsPerMeter")) config.countsPerMeter = proc["countsPerMeter"];
    }
    
    // Update settings
    if (j.contains("update")) {
        auto& upd = j["update"];
        if (upd.contains("updateIntervalMs")) config.updateIntervalMs = upd["updateIntervalMs"];
        if (upd.contains("adaptiveMode")) config.adaptiveMode = upd["adaptiveMode"];
        if (upd.contains("idleUpdateIntervalMs")) config.idleUpdateIntervalMs = upd["idleUpdateIntervalMs"];
    }
    
    // Debug settings
    if (j.contains("debug")) {
        auto& dbg = j["debug"];
        if (dbg.contains("showDebugInfo")) config.showDebugInfo = dbg["showDebugInfo"];
        if (dbg.contains("logToFile")) config.logToFile = dbg["logToFile"];
        if (dbg.contains("logFilePath")) config.logFilePath = dbg["logFilePath"];
    }
    
    return config;
}

AppConfig ConfigManager::GetConfig() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config;
}

void ConfigManager::SetConfig(const AppConfig& config) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config = config;
}

} // namespace Mouse2VR