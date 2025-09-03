#include "core/InputProcessor.h"
#include "common/Logger.h"
#include <cmath>
#include <algorithm>

namespace Mouse2VR {

InputProcessor::InputProcessor() {
    // Default config is already set via member initialization
}

void InputProcessor::ProcessDelta(const MouseDelta& delta, float deltaTime, float& outX, float& outY) {
    // Calculate physical treadmill speed first (before sensitivity)
    if (m_config.countsPerMeter > 0 && deltaTime > 0) {
        // Convert counts/sec to m/s using DPI-based calibration
        // countsPerMeter = DPI * 39.3701 (inches per meter)
        m_currentSpeed = std::abs(delta.y) / m_config.countsPerMeter / deltaTime;
    }
    
    // Calculate stick deflection based on physical speed
    // Formula: deflection = (counts/sec / DPI * 0.0254) / 6.1
    // With DPI from countsPerMeter: DPI = countsPerMeter / 39.3701
    float dpi = m_config.countsPerMeter / 39.3701f;
    
    // Calculate stick deflection for X and Y
    float x = 0.0f;
    float y = 0.0f;
    
    if (deltaTime > 0 && dpi > 0) {
        // Convert counts/sec to stick deflection
        float countsPerSecX = delta.x / deltaTime;
        float countsPerSecY = delta.y / deltaTime;
        
        // Apply the formula: deflection = counts/sec / DPI * 0.0254 / 6.1
        x = countsPerSecX / dpi * 0.0254f / 6.1f;
        y = countsPerSecY / dpi * 0.0254f / 6.1f;
        
        // Apply sensitivity as a multiplier
        x *= m_config.sensitivity;
        y *= m_config.sensitivity;
    }
    
    // Debug logging for input processing
    if (delta.y != 0) {
        LOG_DEBUG("Processor", "Input deltaY=" + std::to_string(delta.y) + 
                  " -> deflection=" + std::to_string(y) + 
                  " (DPI=" + std::to_string(dpi) + ")");
    }
    
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