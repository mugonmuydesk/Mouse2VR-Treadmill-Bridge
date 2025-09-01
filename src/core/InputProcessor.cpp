#include "core/InputProcessor.h"
#include <cmath>
#include <algorithm>

namespace Mouse2VR {

InputProcessor::InputProcessor() {
    // Default config is already set via member initialization
}

void InputProcessor::ProcessDelta(const MouseDelta& delta, float deltaTime, float& outX, float& outY) {
    // Apply sensitivity (scale down less aggressively for better response)
    float x = delta.x * m_config.sensitivity / 100.0f;  // Changed from 1000 to 100 for better scaling
    float y = delta.y * m_config.sensitivity / 100.0f;
    
    // Note: Mouse forward (away from user) = negative delta.y
    //       Mouse backward (toward user) = positive delta.y
    // We want: forward = positive stick, backward = negative stick
    // So we need to invert the Y axis by default
    y = -y;  // Invert so forward mouse = positive stick
    
    // Apply user inversion preferences (on top of default inversion)
    if (m_config.invertX) x = -x;
    if (m_config.invertY) y = -y;
    
    // Apply axis locks
    if (m_config.lockX) x = 0.0f;
    if (m_config.lockY) y = 0.0f;
    
    // Clamp to max speed
    float magnitude = std::sqrt(x * x + y * y);
    if (magnitude > m_config.maxSpeed && magnitude > 0.0f) {
        float scale = m_config.maxSpeed / magnitude;
        x *= scale;
        y *= scale;
    }
    
    // Apply deadzone
    x = ApplyDeadzone(x);
    y = ApplyDeadzone(y);
    
    // Force X to 0 for treadmill (Y-axis only movement)
    x = 0.0f;
    
    // Store for metrics
    m_lastStickX = x;
    m_lastStickY = y;
    
    // Calculate speed (assuming Y is forward/back for treadmill)
    if (m_config.countsPerMeter > 0 && deltaTime > 0) {
        // Convert counts to meters, then to m/s using actual delta time
        m_currentSpeed = std::abs(delta.y) / m_config.countsPerMeter / deltaTime;
    }
    
    // Handle calibration
    if (m_calibrating) {
        m_calibrationDeltas += delta;
    }
    
    outX = x;
    outY = y;
}

void InputProcessor::SetConfig(const ProcessingConfig& config) {
    m_config = config;
}

ProcessingConfig InputProcessor::GetConfig() const {
    return m_config;
}

void InputProcessor::StartCalibration() {
    m_calibrating = true;
    m_calibrationDeltas.reset();
}

void InputProcessor::EndCalibration(float distanceMeters) {
    if (!m_calibrating || distanceMeters <= 0) {
        return;
    }
    
    // For treadmill, use only Y-axis (forward/back movement)
    float totalCounts = std::abs(static_cast<float>(m_calibrationDeltas.y));
    
    if (totalCounts > 0) {
        m_config.countsPerMeter = totalCounts / distanceMeters;
    }
    
    m_calibrating = false;
    m_calibrationDeltas.reset();
}

float InputProcessor::GetStickDeflectionPercent() const {
    float magnitude = std::sqrt(m_lastStickX * m_lastStickX + m_lastStickY * m_lastStickY);
    return std::min(1.0f, magnitude) * 100.0f;
}

float InputProcessor::ApplyDeadzone(float value) const {
    if (m_config.deadzone <= 0.0f) {
        return value;
    }
    
    float absValue = std::abs(value);
    if (absValue < m_config.deadzone) {
        return 0.0f;
    }
    
    // Scale the value to maintain full range after deadzone
    float sign = (value < 0) ? -1.0f : 1.0f;
    return sign * (absValue - m_config.deadzone) / (1.0f - m_config.deadzone);
}

} // namespace Mouse2VR