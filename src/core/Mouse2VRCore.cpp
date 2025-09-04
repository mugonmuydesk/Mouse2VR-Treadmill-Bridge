// Include C runtime headers first to prevent Windows.h conflicts
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <algorithm>

#include "core/Mouse2VRCore.h"
#include "common/Logger.h"

// Include complete type definitions for std::unique_ptr destructors
#include "core/ConfigManager.h"
#include "core/PathUtils.h"
#include "core/RawInputHandler.h"
#include "core/ViGEmController.h"
#include "core/InputProcessor.h"

// Windows multimedia for timeBeginPeriod
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

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
    
    // Enable high-resolution timers (1ms resolution)
    timeBeginPeriod(1);
    
    LOG_INFO("Core", "Initializing Mouse2VR Core...");
    
    // Initialize actual components
    m_inputHandler = std::make_unique<RawInputHandler>();
    m_controller = std::make_unique<ViGEmController>();
    m_processor = std::make_unique<InputProcessor>();
    // Use exe-relative path for config
    std::string configPath = PathUtils::GetExecutablePath("config.json");
    m_config = std::make_unique<ConfigManager>(configPath);
    
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
    procConfig.countsPerMeter = config.countsPerMeter;
    m_processor->SetConfig(procConfig);
    
    // Set update rate from config
    if (config.updateIntervalMs > 0) {
        m_updateRateHz = 1000 / config.updateIntervalMs;
    }
    
    // Register settings provider with logger
    Logger::Instance().SetSettingsProvider([this]() {
        return GetCurrentSettingsSnapshot();
    });
    
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
    m_actualUpdateRate = 0;  // Reset the rate from any previous runs
    
    // Start processing thread (ensure no existing thread is running)
    if (m_processingThread && m_processingThread->joinable()) {
        m_processingThread->join();
    }
    m_processingThread = std::make_unique<std::thread>(&Mouse2VRCore::ProcessingLoop, this);
}

void Mouse2VRCore::Stop() {
    if (!m_isRunning) {
        return;
    }
    
    LOG_INFO("Core", "Stopping Mouse2VR Core...");
    m_isRunning = false;
    
    // Wait for processing thread to finish
    if (m_processingThread && m_processingThread->joinable()) {
        m_processingThread->join();
        m_processingThread.reset();
    }
}

void Mouse2VRCore::Shutdown() {
    Stop();
    m_isInitialized = false;
    
    // Ensure thread is cleaned up
    if (m_processingThread && m_processingThread->joinable()) {
        m_processingThread->join();
        m_processingThread.reset();
    }
    
    // Restore default timer resolution
    timeEndPeriod(1);
    
    LOG_INFO("Core", "Mouse2VR Core shut down");
}

ControllerState Mouse2VRCore::GetCurrentState() const {
    m_speedQueryCount++;  // Track that speed was queried
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
    LOG_INFO("Core", "[VR Scheduler] Starting with target rate: " + std::to_string(m_updateRateHz.load()) + " Hz");
    
    // === VR-Safe Startup: Enable precise sleeps only while running ===
    timeBeginPeriod(1);
    
    // === High-precision timing with QueryPerformanceCounter ===
    LARGE_INTEGER frequency, lastTick, now;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTick);
    
    // === Scheduler state ===
    uint64_t tickCount = 0;
    double accumulatedError = 0.0;
    int missedFrames = 0;
    LARGE_INTEGER schedulerStartTime = lastTick;
    
    while (m_isRunning) {
        // === Dynamic rate updates from config/UI ===
        double targetHz = static_cast<double>(m_updateRateHz.load());
        double targetInterval = 1.0 / targetHz;
        
        // === Process treadmill inputs → stick deflection → game speed ===
        UpdateController();
        tickCount++;
        
        // === Calculate next frame time ===
        lastTick.QuadPart += static_cast<LONGLONG>(targetInterval * frequency.QuadPart);
        
        // === VR-Safe timing: sleep most, spin-wait last 2ms ===
        QueryPerformanceCounter(&now);
        double remaining = (lastTick.QuadPart - now.QuadPart) / static_cast<double>(frequency.QuadPart);
        
        // === Handle late frames (VR-safe: skip instead of blocking) ===
        if (remaining < 0) {
            missedFrames++;
            accumulatedError += -remaining;
            
            // Reset schedule to prevent death spiral
            lastTick = now;
            
            // Only log significant delays (>5ms) to avoid spam
            if (-remaining > 0.005) {
                LOG_DEBUG("Core", "[VR Scheduler] Skipped frame (late by " + 
                         std::to_string(-remaining * 1000.0) + " ms)");
            }
        }
        else {
            // === Sleep phase: leave CPU for VR compositor ===
            while (remaining > 0.002) { // More than 2ms left
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                QueryPerformanceCounter(&now);
                remaining = (lastTick.QuadPart - now.QuadPart) / static_cast<double>(frequency.QuadPart);
            }
            
            // === Spin-wait phase: precise timing for last 2ms ===
            while (remaining > 0) {
                QueryPerformanceCounter(&now);
                remaining = (lastTick.QuadPart - now.QuadPart) / static_cast<double>(frequency.QuadPart);
            }
        }
        
        // === Comprehensive logging every second ===
        if (tickCount % static_cast<uint64_t>(targetHz) == 0) {
            QueryPerformanceCounter(&now);
            double totalElapsed = (now.QuadPart - schedulerStartTime.QuadPart) / static_cast<double>(frequency.QuadPart);
            double achievedHz = tickCount / totalElapsed;
            
            // Calculate drift in milliseconds
            double driftMs = accumulatedError * 1000.0;
            
            // Update actual rate for UI display
            m_actualUpdateRate = static_cast<int>(achievedHz + 0.5);
            
            // Log scheduler performance
            LOG_INFO("Core", "[VR Scheduler] Target=" + std::to_string(static_cast<int>(targetHz)) + 
                    " Hz, Achieved=" + std::to_string(achievedHz) + 
                    " Hz, Drift=" + (driftMs >= 0 ? "+" : "") + std::to_string(driftMs) + 
                    " ms, Missed=" + std::to_string(missedFrames) + " frames");
            
            // Reset per-second tracking
            accumulatedError = 0.0;
            missedFrames = 0;
        }
    }
    
    // === VR-Safe shutdown: disable high-res timing ===
    timeEndPeriod(1);
    
    LOG_INFO("Core", "[VR Scheduler] Stopped");
}

void Mouse2VRCore::UpdateController() {
    if (!m_inputHandler || !m_processor || !m_controller) {
        return;
    }
    
    // === Get mouse deltas ===
    MouseDelta delta = m_inputHandler->GetAndResetDeltas();
    
    // === Calculate elapsed time for velocity calculations ===
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - m_lastUpdate).count();
    m_lastUpdate = now;
    
    // Skip if no time has passed (prevent division by zero)
    if (elapsed <= 0.0f) {
        return;
    }
    
    // === Process input (treadmill → stick deflection) ===
    float stickX, stickY;
    m_processor->ProcessDelta(delta, elapsed, stickX, stickY);
    
    // === Update virtual controller (Y-axis only for treadmill) ===
    m_controller->SetLeftStick(0.0f, stickY);
    m_controller->Update();
    
    // === Extended diagnostic logging (if enabled) ===
    static bool enableDetailedLogging = false; // Can be toggled via config
    static int logCounter = 0;
    if (enableDetailedLogging && ++logCounter % 50 == 0) { // Log every 50th update
        auto config = m_processor->GetConfig();
        float dpi = config.countsPerMeter / 39.3701f;
        float physicalSpeed = (delta.y / elapsed) / dpi * 0.0254f; // m/s
        float gameSpeed = stickY * 6.1f * config.sensitivity / 100.0f; // m/s in game
        
        LOG_DEBUG("Core", "[VR Detail] DeltaY=" + std::to_string(delta.y) + 
                 " counts, Physical=" + std::to_string(physicalSpeed) + 
                 " m/s, Game=" + std::to_string(gameSpeed) + 
                 " m/s, Stick=" + std::to_string(stickY * 100) + "%");
    }
    
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

// Test interface implementations
Mouse2VRCore::ProcessorConfig Mouse2VRCore::GetProcessorConfig() const {
    ProcessorConfig config;
    if (m_processor) {
        auto procConfig = m_processor->GetConfig();
        config.countsPerMeter = procConfig.countsPerMeter;
        config.sensitivity = procConfig.sensitivity;
        config.invertY = procConfig.invertY;
        config.lockX = procConfig.lockX;
        config.lockY = procConfig.lockY;
        config.dpi = static_cast<int>(procConfig.countsPerMeter / 39.3701f);
    }
    return config;
}

void Mouse2VRCore::UpdateSettings(const AppConfig& newConfig) {
    if (m_config) {
        m_config->SetConfig(newConfig);
        
        // Apply settings to processor
        if (m_processor) {
            auto procConfig = m_processor->GetConfig();
            procConfig.countsPerMeter = newConfig.countsPerMeter;
            procConfig.sensitivity = newConfig.sensitivity;
            procConfig.invertY = newConfig.invertY;
            procConfig.lockX = newConfig.lockX;
            procConfig.lockY = newConfig.lockY;
            m_processor->SetConfig(procConfig);
        }
        
        // Apply update rate (convert ms to Hz)
        if (newConfig.updateIntervalMs > 0) {
            m_updateRateHz = 1000 / newConfig.updateIntervalMs;
        }
    }
}

void Mouse2VRCore::ForceUpdate() {
    // Force a single update cycle for testing
    UpdateController();
}

std::string Mouse2VRCore::GetCurrentSettingsSnapshot() const {
    std::string snapshot;
    
    try {
        // Get processor config
        if (m_processor) {
            auto procConfig = m_processor->GetConfig();
            int dpi = static_cast<int>(procConfig.countsPerMeter / 39.3701f);
            
            // Format: DPI:1000|Sens:1.0|Hz:45|InvY:0|LockX:1|Running:1|ActHz:44|Speed:0.05
            snapshot = "DPI:" + std::to_string(dpi);
            
            // Add sensitivity (format to 1 decimal)
            char sensStr[16];
            snprintf(sensStr, sizeof(sensStr), "%.1f", procConfig.sensitivity);
            snapshot += "|Sens:" + std::string(sensStr);
            
            // Add target Hz
            snapshot += "|Hz:" + std::to_string(m_updateRateHz.load());
            
            // Add axis settings
            snapshot += "|InvY:" + std::to_string(procConfig.invertY ? 1 : 0);
            snapshot += "|LockX:" + std::to_string(procConfig.lockX ? 1 : 0);
            
            // Add running status
            snapshot += "|Run:" + std::to_string(m_isRunning ? 1 : 0);
            
            // Add actual update rate
            snapshot += "|ActHz:" + std::to_string(m_actualUpdateRate.load());
            
            // Add current speed (format to 2 decimals)
            float speed = m_processor->GetSpeedMetersPerSecond();
            char speedStr[16];
            snprintf(speedStr, sizeof(speedStr), "%.2f", speed);
            snapshot += "|Spd:" + std::string(speedStr);
            
            // Add stick deflection percentage (format to 1 decimal)
            float deflection = m_processor->GetStickDeflectionPercent();
            char deflStr[16];
            snprintf(deflStr, sizeof(deflStr), "%.1f", deflection);
            snapshot += "|Stk:" + std::string(deflStr) + "%";
            
            // Add test status if running
            if (m_isTestRunning) {
                snapshot += "|TEST:1";
            }
        }
    } catch (...) {
        // Return partial snapshot on error
    }
    
    return snapshot;
}

} // namespace Mouse2VR