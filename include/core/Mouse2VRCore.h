#pragma once
#include <memory>
#include <atomic>
#include <mutex>
#include <chrono>

// Include Windows.h for HWND
#include "common/WindowsHeaders.h"

namespace Mouse2VR {

// Forward declarations
class RawInputHandler;
class ViGEmController;
class InputProcessor;
class ConfigManager;
struct AppConfig;

// Simple data structure for mouse/controller state
struct ControllerState {
    double speed = 0.0;
    double stickX = 0.0;
    double stickY = 0.0;
    int updateRate = 60;
};

// Main core class that manages all the components
class Mouse2VRCore {
public:
    Mouse2VRCore();
    ~Mouse2VRCore();
    
    // Lifecycle
    bool Initialize();
    bool Initialize(HWND hwnd);
    void Start();
    void Stop();
    void Shutdown();
    
    // State
    bool IsRunning() const { return m_isRunning; }
    ControllerState GetCurrentState() const;
    
    // Configuration
    void SetSensitivity(double sensitivity);
    double GetSensitivity() const;
    void SetUpdateRate(int hz);
    int GetUpdateRate() const;
    void SetInvertY(bool invert);
    void SetLockX(bool lock);
    void SetCountsPerMeter(float countsPerMeter);
    
    // Statistics
    double GetCurrentSpeed() const;
    double GetAverageSpeed() const;
    int GetActualUpdateRate() const;
    
    // Testing
    void StartMovementTest();
    bool IsTestRunning() const { return m_isTestRunning; }
    
    // Internal access for WM_INPUT processing
    RawInputHandler* GetInputHandler() const { return m_inputHandler.get(); }
    
    // Test interfaces
    struct ProcessorConfig {
        float countsPerMeter = 39370.1f;
        float sensitivity = 1.0f;
        bool invertY = false;
        bool lockX = false;
        bool lockY = false;
        int dpi = 1000;
    };
    
    ProcessorConfig GetProcessorConfig() const;
    void UpdateSettings(const struct AppConfig& config);
    void ForceUpdate();
    
private:
    std::unique_ptr<RawInputHandler> m_inputHandler;
    std::unique_ptr<ViGEmController> m_controller;
    std::unique_ptr<InputProcessor> m_processor;
    std::unique_ptr<ConfigManager> m_config;
    
    std::atomic<bool> m_isRunning;
    std::atomic<bool> m_isInitialized;
    
    // Current state
    mutable std::mutex m_stateMutex;
    ControllerState m_currentState;
    
    // Timing
    std::chrono::steady_clock::time_point m_lastUpdate;
    std::atomic<int> m_updateRateHz{60};  // Default 60Hz
    
    // Actual update rate tracking
    std::chrono::steady_clock::time_point m_rateTrackingStart;
    std::atomic<int> m_updateCount{0};
    std::atomic<int> m_actualUpdateRate{0};
    
    // Testing
    std::atomic<bool> m_isTestRunning{false};
    std::chrono::steady_clock::time_point m_testStartTime;
    float m_testDuration = 5.0f;
    int m_testUpdateCount = 0;
    float m_testTotalDistance = 0.0f;
    float m_testPeakSpeed = 0.0f;
    float m_testTotalSpeed = 0.0f;
    
    // Internal methods
    void ProcessingLoop();
    void UpdateController();
};

} // namespace Mouse2VR