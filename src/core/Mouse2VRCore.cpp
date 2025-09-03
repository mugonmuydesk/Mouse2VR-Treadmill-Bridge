// Include C runtime headers first to prevent Windows.h conflicts
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include "core/Mouse2VRCore.h"
#include "common/Logger.h"

// Include complete type definitions for std::unique_ptr destructors
#include "core/ConfigManager.h"
#include "core/RawInputHandler.h"
#include "core/ViGEmController.h"
#include "core/InputProcessor.h"

#include <thread>
#include <chrono>

namespace Mouse2VR {

Mouse2VRCore::Mouse2VRCore() 
    : m_isRunning(false)
    , m_isInitialized(false)
    , m_lastUpdate(std::chrono::steady_clock::now()) {
}

Mouse2VRCore::~Mouse2VRCore() {
    Shutdown();
}

bool Mouse2VRCore::Initialize() {
    // Default initialization without window handle
    return Initialize(nullptr);
}

bool Mouse2VRCore::Initialize(HWND hwnd) {
    if (m_isInitialized) {
        return true;
    }
    
    LOG_INFO("Core", "Initializing Mouse2VR Core...");
    
    // Initialize actual components
    m_inputHandler = std::make_unique<RawInputHandler>();
    m_controller = std::make_unique<ViGEmController>();
    m_processor = std::make_unique<InputProcessor>();
    m_config = std::make_unique<ConfigManager>();
    
    // Initialize RawInputHandler with window handle if provided
    if (hwnd) {
        if (!m_inputHandler->Initialize(hwnd)) {
            LOG_ERROR("Core", "Failed to initialize RawInputHandler");
            return false;
        }
        LOG_INFO("Core", "RawInputHandler initialized with window handle");
    }
    
    // Initialize ViGEmController
    if (!m_controller->Initialize()) {
        LOG_ERROR("Core", "Failed to initialize ViGEmController");
        return false;
    }
    LOG_INFO("Core", "Virtual Xbox 360 controller created");
    
    // Load configuration and apply to processor
    if (m_config->Load()) {
        LOG_INFO("Core", "Configuration loaded from file");
    } else {
        LOG_INFO("Core", "Using default configuration");
    }
    
    // Apply configuration to processor
    auto config = m_config->GetConfig();
    ProcessingConfig procConfig;
    procConfig.sensitivity = config.sensitivity;
    procConfig.invertY = config.invertY;
    procConfig.lockX = config.lockX;
    procConfig.countsPerMeter = 1000.0f; // Default value
    m_processor->SetConfig(procConfig);
    
    // Set update rate from config
    if (config.updateIntervalMs > 0) {
        m_updateRateHz = 1000 / config.updateIntervalMs;
    }
    
    m_isInitialized = true;
    LOG_INFO("Core", "Mouse2VR Core initialized successfully");
    return true;
}

void Mouse2VRCore::Start() {
    if (!m_isInitialized || m_isRunning) {
        return;
    }
    
    LOG_INFO("Core", "Starting Mouse2VR Core...");
    m_isRunning = true;
    
    // Start processing thread
    std::thread(&Mouse2VRCore::ProcessingLoop, this).detach();
}

void Mouse2VRCore::Stop() {
    if (!m_isRunning) {
        return;
    }
    
    LOG_INFO("Core", "Stopping Mouse2VR Core...");
    m_isRunning = false;
    
    // TODO: Stop processing
}

void Mouse2VRCore::Shutdown() {
    Stop();
    m_isInitialized = false;
    LOG_INFO("Core", "Mouse2VR Core shut down");
}

ControllerState Mouse2VRCore::GetCurrentState() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_currentState;
}

void Mouse2VRCore::SetSensitivity(double sensitivity) {
    LOG_INFO("Core", "Setting sensitivity to: " + std::to_string(sensitivity));
    if (m_processor) {
        ProcessingConfig config = m_processor->GetConfig();
        config.sensitivity = static_cast<float>(sensitivity);
        m_processor->SetConfig(config);
    }
    if (m_config) {
        auto cfg = m_config->GetConfig();
        cfg.sensitivity = static_cast<float>(sensitivity);
        m_config->SetConfig(cfg);
        m_config->Save();
    }
}

double Mouse2VRCore::GetSensitivity() const {
    if (m_processor) {
        return m_processor->GetConfig().sensitivity;
    }
    return 1.0;
}

void Mouse2VRCore::SetUpdateRate(int hz) {
    LOG_INFO("Core", "Setting update rate to: " + std::to_string(hz) + " Hz");
    // Clamp to reasonable range
    if (hz < 10) hz = 10;
    if (hz > 200) hz = 200;
    m_updateRateHz = hz;
    
    // Also save to config
    if (m_config) {
        auto cfg = m_config->GetConfig();
        cfg.updateIntervalMs = 1000 / hz;  // Convert Hz to milliseconds
        m_config->SetConfig(cfg);
        m_config->Save();
    }
}

int Mouse2VRCore::GetUpdateRate() const {
    return m_updateRateHz;
}

void Mouse2VRCore::SetInvertY(bool invert) {
    LOG_INFO("Core", "Setting invert Y to: " + std::string(invert ? "true" : "false"));
    if (m_processor) {
        ProcessingConfig config = m_processor->GetConfig();
        config.invertY = invert;
        m_processor->SetConfig(config);
    }
    if (m_config) {
        auto cfg = m_config->GetConfig();
        cfg.invertY = invert;
        m_config->SetConfig(cfg);
        m_config->Save();
    }
}

void Mouse2VRCore::SetLockX(bool lock) {
    LOG_INFO("Core", "Setting lock X to: " + std::string(lock ? "true" : "false"));
    if (m_processor) {
        ProcessingConfig config = m_processor->GetConfig();
        config.lockX = lock;
        m_processor->SetConfig(config);
    }
    if (m_config) {
        auto cfg = m_config->GetConfig();
        cfg.lockX = lock;
        m_config->SetConfig(cfg);
        m_config->Save();
    }
}

double Mouse2VRCore::GetCurrentSpeed() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_currentState.speed;
}

double Mouse2VRCore::GetAverageSpeed() const {
    return 0.0; // TODO: Implement
}

int Mouse2VRCore::GetActualUpdateRate() const {
    return 60; // TODO: Implement
}

void Mouse2VRCore::ProcessingLoop() {
    while (m_isRunning) {
        UpdateController();
        // Use dynamic update rate
        int intervalMs = 1000 / m_updateRateHz;
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}

void Mouse2VRCore::UpdateController() {
    if (!m_inputHandler || !m_processor || !m_controller) {
        return;
    }
    
    // 1. Get mouse deltas
    MouseDelta delta = m_inputHandler->GetAndResetDeltas();
    
    // 2. Calculate elapsed time
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - m_lastUpdate).count();
    m_lastUpdate = now;
    
    // Skip if no time has passed (prevent division by zero)
    if (elapsed <= 0.0f) {
        return;
    }
    
    // 3. Process input
    float stickX, stickY;
    m_processor->ProcessDelta(delta, elapsed, stickX, stickY);
    
    // 4. Update controller (Y-axis only for treadmill)
    m_controller->SetLeftStick(0.0f, stickY);
    m_controller->Update();
    
    // 5. Update state for UI
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_currentState.speed = m_processor->GetSpeedMetersPerSecond();
        m_currentState.stickX = stickX;
        m_currentState.stickY = stickY;
    }
    
    // Debug logging for significant movement
    if (delta.y != 0 || delta.x != 0) {
        LOG_DEBUG("Core", "UpdateController: deltaY=" + std::to_string(delta.y) + 
                          " -> stickY=" + std::to_string(stickY) +
                          " (speed=" + std::to_string(m_processor->GetSpeedMetersPerSecond()) + " m/s)" +
                          " [" + (stickY > 0 ? "FORWARD" : stickY < 0 ? "BACKWARD" : "STOPPED") + "]");
    }
}

} // namespace Mouse2VR