#pragma once

#include <chrono>
#include <vector>
#include <functional>

namespace Mouse2VR {

// Test-specific interface for injecting raw input directly
class ITestableRawInput {
public:
    virtual ~ITestableRawInput() = default;
    
    // Inject raw mouse input for testing (bypasses actual mouse)
    virtual void InjectRawInput(int deltaX, int deltaY) = 0;
    
    // Clear any accumulated input
    virtual void ClearAccumulatedInput() = 0;
    
    // Get current accumulated values
    virtual void GetAccumulatedInput(int& deltaX, int& deltaY) const = 0;
};

// Test-specific interface for accessing internal state
class ITestableMouse2VR {
public:
    virtual ~ITestableMouse2VR() = default;
    
    // Get current processor configuration
    virtual struct ProcessorConfig GetProcessorConfig() const = 0;
    
    // Get current update metrics
    virtual struct UpdateMetrics GetUpdateMetrics() const = 0;
    
    // Force an update cycle (for testing)
    virtual void ForceUpdate() = 0;
    
    // Get last controller state
    virtual struct ControllerState GetLastControllerState() const = 0;
};

// Metrics collection for tests
struct UpdateMetrics {
    float actualUpdateHz = 0.0f;
    float targetUpdateHz = 0.0f;
    float webViewUpdateHz = 0.0f;
    int totalUpdates = 0;
    std::chrono::steady_clock::time_point lastUpdate;
};

struct ControllerState {
    float leftStickX = 0.0f;
    float leftStickY = 0.0f;
    float rightStickX = 0.0f;
    float rightStickY = 0.0f;
    bool isConnected = false;
};

struct ProcessorConfig {
    float countsPerMeter = 39370.1f;  // 1000 DPI default
    float sensitivity = 1.0f;
    bool invertY = false;
    bool lockX = false;
    bool lockY = false;
    int dpi = 1000;
};

// Test helper class for metrics collection
class TestMetricsCollector {
public:
    TestMetricsCollector() : startTime(std::chrono::steady_clock::now()) {}
    
    void RecordUpdate() {
        updates++;
        lastUpdateTime = std::chrono::steady_clock::now();
    }
    
    void RecordWebViewUpdate() {
        webViewUpdates++;
        lastWebViewUpdateTime = std::chrono::steady_clock::now();
    }
    
    void RecordControllerState(float stickX, float stickY) {
        lastStickX = stickX;
        lastStickY = stickY;
        maxStickDeflection = std::max(maxStickDeflection, 
            std::sqrt(stickX * stickX + stickY * stickY));
    }
    
    float GetActualHz() const {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            lastUpdateTime - startTime).count();
        if (elapsed == 0) return 0.0f;
        return (updates * 1000.0f) / elapsed;
    }
    
    float GetWebViewHz() const {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            lastWebViewUpdateTime - startTime).count();
        if (elapsed == 0) return 0.0f;
        return (webViewUpdates * 1000.0f) / elapsed;
    }
    
    void Reset() {
        updates = 0;
        webViewUpdates = 0;
        lastStickX = 0.0f;
        lastStickY = 0.0f;
        maxStickDeflection = 0.0f;
        startTime = std::chrono::steady_clock::now();
    }
    
    int updates = 0;
    int webViewUpdates = 0;
    float lastStickX = 0.0f;
    float lastStickY = 0.0f;
    float maxStickDeflection = 0.0f;
    
private:
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastUpdateTime;
    std::chrono::steady_clock::time_point lastWebViewUpdateTime;
};

} // namespace Mouse2VR