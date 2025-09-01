#include "processing/InputProcessor.h"
#include <cmath>
#include <algorithm>

namespace Mouse2VR {

InputProcessor::InputProcessor() {
    // Default config is already set via member initialization
}

void InputProcessor::ProcessDelta(const MouseDelta& delta, float& outX, float& outY) {
    // Apply sensitivity
    float x = delta.x * m_config.sensitivity / 1000.0f;  // Normalize to reasonable range
    float y = delta.y * m_config.sensitivity / 1000.0f;
    
    // Apply inversion
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
    
    // Store for metrics
    m_lastStickX = x;
    m_lastStickY = y;
    
    // Calculate speed (assuming Y is forward/back for treadmill)
    if (m_config.countsPerMeter > 0) {
        // Convert counts to meters, then to m/s (assuming 60 Hz update)
        m_currentSpeed = std::abs(delta.y) / m_config.countsPerMeter * 60.0f;
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
    
    // Calculate counts per meter based on accumulated deltas
    float totalCounts = std::sqrt(
        static_cast<float>(m_calibrationDeltas.x * m_calibrationDeltas.x + 
                          m_calibrationDeltas.y * m_calibrationDeltas.y)
    );
    
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