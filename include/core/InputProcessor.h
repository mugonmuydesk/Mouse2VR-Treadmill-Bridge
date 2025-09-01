#pragma once
#include "core/RawInputHandler.h"
#include <atomic>

namespace Mouse2VR {

struct ProcessingConfig {
    float sensitivity = 1.0f;      // Mouse to stick sensitivity
    float deadzone = 0.0f;         // Stick deadzone (0.0 = off)
    bool invertX = false;          // Invert X axis
    bool invertY = false;          // Invert Y axis
    bool lockX = false;            // Lock X axis (no horizontal movement)
    bool lockY = false;            // Lock Y axis (no vertical movement)
    float maxSpeed = 1.0f;         // Maximum stick deflection (0.0 to 1.0)
    
    // Calibration values
    float countsPerMeter = 1000.0f;  // Mouse counts per meter of treadmill movement
};

class InputProcessor {
public:
    InputProcessor();
    
    // Process raw mouse delta and return stick position
    void ProcessDelta(const MouseDelta& delta, float deltaTime, float& outX, float& outY);
    
    // Update configuration
    void SetConfig(const ProcessingConfig& config);
    ProcessingConfig GetConfig() const;
    
    // Calibration
    void StartCalibration();
    void EndCalibration(float distanceMeters);
    bool IsCalibrating() const { return m_calibrating; }
    
    // Get current speed in m/s
    float GetSpeedMetersPerSecond() const { return m_currentSpeed; }
    
    // Get stick deflection percentage (0-100)
    float GetStickDeflectionPercent() const;

private:
    ProcessingConfig m_config;
    std::atomic<bool> m_calibrating{false};
    
    // Calibration data
    MouseDelta m_calibrationDeltas;
    
    // Current state
    float m_currentSpeed = 0.0f;
    float m_lastStickX = 0.0f;
    float m_lastStickY = 0.0f;
    
    // Apply deadzone to stick value
    float ApplyDeadzone(float value) const;
};

} // namespace Mouse2VR