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
    
    // Initialize rate tracking
    m_rateTrackingStart = std::chrono::steady_clock::now();
    m_updateCount = 0;
    
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
    LOG_INFO("Core", "SetUpdateRate called with: " + std::to_string(hz) + " Hz");
    // Clamp to reasonable range
    if (hz < 10) hz = 10;
    if (hz > 200) hz = 200;
    m_updateRateHz = hz;
    LOG_INFO("Core", "Update rate set to: " + std::to_string(m_updateRateHz.load()) + " Hz (interval: " + std::to_string(1000/hz) + " ms)");
    
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

void Mouse2VRCore::SetCountsPerMeter(float countsPerMeter) {
    LOG_INFO("Core", "Setting counts per meter to: " + std::to_string(countsPerMeter));
    if (m_processor) {
        ProcessingConfig config = m_processor->GetConfig();
        config.countsPerMeter = countsPerMeter;
        m_processor->SetConfig(config);
    }
    if (m_config) {
        auto cfg = m_config->GetConfig();
        cfg.countsPerMeter = countsPerMeter;
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
    return m_actualUpdateRate.load();
}

void Mouse2VRCore::StartMovementTest() {
    if (m_isTestRunning) {
        LOG_WARNING("Core", "Test already running");
        return;
    }
    
    LOG_INFO("Core", "===== STARTING 5-SECOND MOVEMENT TEST =====");
    LOG_INFO("Core", "Move the treadmill to generate test data");
    
    m_isTestRunning = true;
    m_testStartTime = std::chrono::steady_clock::now();
    m_testUpdateCount = 0;
    m_testTotalDistance = 0.0f;
    m_testPeakSpeed = 0.0f;
    m_testTotalSpeed = 0.0f;
    
    // Get current settings for logging
    auto config = m_processor->GetConfig();
    float dpi = config.countsPerMeter / 39.3701f;
    
    LOG_INFO("Core", "Test Configuration:");
    LOG_INFO("Core", "  DPI: " + std::to_string(static_cast<int>(dpi)));
    LOG_INFO("Core", "  Sensitivity: " + std::to_string(config.sensitivity));
    LOG_INFO("Core", "  Counts per meter: " + std::to_string(config.countsPerMeter));
    LOG_INFO("Core", "  Invert Y: " + std::string(config.invertY ? "Yes" : "No"));
    LOG_INFO("Core", "  Lock X: " + std::string(config.lockX ? "Yes" : "No"));
}

void Mouse2VRCore::ProcessingLoop() {
    LOG_DEBUG("Core", "ProcessingLoop started with target rate: " + std::to_string(m_updateRateHz.load()) + " Hz");
    
    // Use high precision timing with microseconds
    using clock = std::chrono::steady_clock;
    using microseconds = std::chrono::microseconds;
    
    auto nextUpdate = clock::now();
    
    while (m_isRunning) {
        // Calculate interval in microseconds for better precision
        int targetHz = m_updateRateHz.load();
        auto intervalUs = microseconds(1000000 / targetHz);
        
        // Schedule next update time BEFORE processing
        nextUpdate += intervalUs;
        
        // Do the actual work
        UpdateController();
        
        // Sleep until next scheduled update
        // Use sleep_until for more precise timing
        std::this_thread::sleep_until(nextUpdate);
        
        // If we're running behind, catch up without sleeping
        auto now = clock::now();
        if (now > nextUpdate) {
            // We're running behind schedule, reset to avoid spiral
            nextUpdate = now;
            LOG_DEBUG("Core", "Processing loop running behind, resetting schedule");
        }
    }
    
    LOG_DEBUG("Core", "ProcessingLoop ended");
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
    
    // Track actual update rate
    m_updateCount++;
    auto rateElapsed = std::chrono::duration<float>(now - m_rateTrackingStart).count();
    if (rateElapsed >= 1.0f) {
        m_actualUpdateRate = static_cast<int>(m_updateCount / rateElapsed);
        m_updateCount = 0;
        m_rateTrackingStart = now;
        LOG_DEBUG("Core", "Actual update rate: " + std::to_string(m_actualUpdateRate.load()) + " Hz");
    }
    
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
    
    // Test mode logging
    if (m_isTestRunning) {
        auto testNow = std::chrono::steady_clock::now();
        float testElapsed = std::chrono::duration<float>(testNow - m_testStartTime).count();
        
        if (testElapsed >= m_testDuration) {
            // End test
            m_isTestRunning = false;
            
            // Calculate averages
            float avgSpeed = m_testUpdateCount > 0 ? m_testTotalSpeed / m_testUpdateCount : 0.0f;
            
            LOG_INFO("Core", "===== TEST COMPLETE =====");
            LOG_INFO("Core", "Test Results:");
            LOG_INFO("Core", "  Duration: " + std::to_string(m_testDuration) + " seconds");
            LOG_INFO("Core", "  Updates: " + std::to_string(m_testUpdateCount));
            LOG_INFO("Core", "  Average Speed: " + std::to_string(avgSpeed) + " m/s");
            LOG_INFO("Core", "  Peak Speed: " + std::to_string(m_testPeakSpeed) + " m/s");
            LOG_INFO("Core", "  Total Distance: " + std::to_string(m_testTotalDistance) + " meters");
            LOG_INFO("Core", "  Avg Update Rate: " + std::to_string(static_cast<int>(m_testUpdateCount / m_testDuration)) + " Hz");
            LOG_INFO("Core", "=========================");
        } else {
            // Log detailed test data
            m_testUpdateCount++;
            float currentSpeed = m_processor->GetSpeedMetersPerSecond();
            m_testTotalSpeed += currentSpeed;
            m_testTotalDistance += currentSpeed * elapsed;
            
            if (currentSpeed > m_testPeakSpeed) {
                m_testPeakSpeed = currentSpeed;
            }
            
            // Get config for sensitivity
            auto config = m_processor->GetConfig();
            
            // Calculate game speed (stick deflection * max HL2 speed)
            float gameSpeed = stickY * 6.1f;
            
            // Only log when there's movement
            if (delta.y != 0) {
                LOG_INFO("Core", "[TEST] t=" + std::to_string(testElapsed) + "s" +
                                " | raw_mickeys=" + std::to_string(delta.y) +
                                " | treadmill_speed=" + std::to_string(currentSpeed) + "m/s" +
                                " | sensitivity=" + std::to_string(config.sensitivity) +
                                " | game_speed=" + std::to_string(std::abs(gameSpeed)) + "m/s" +
                                " | deflection=" + std::to_string(std::abs(stickY) * 100.0f) + "%");
            }
        }
    }
    // Regular debug logging (when not testing)
    else if (delta.y != 0 || delta.x != 0) {
        LOG_DEBUG("Core", "UpdateController: deltaY=" + std::to_string(delta.y) + 
                          " -> stickY=" + std::to_string(stickY) +
                          " (speed=" + std::to_string(m_processor->GetSpeedMetersPerSecond()) + " m/s)" +
                          " [" + (stickY > 0 ? "FORWARD" : stickY < 0 ? "BACKWARD" : "STOPPED") + "]");
    }
}

} // namespace Mouse2VR