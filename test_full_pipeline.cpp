// Test full pipeline from raw input to display
#include <iostream>
#include <iomanip>
#include <chrono>
#include "core/InputProcessor.h"
#include "core/RawInputHandler.h"

using namespace Mouse2VR;

// Simulate what happens in UpdateController
void simulateFullPipeline(int rawMouseY) {
    std::cout << "\n=== Testing Raw Mouse Y = " << rawMouseY << " ===" << std::endl;
    
    // Step 1: Raw input captured
    MouseDelta delta;
    delta.x = 0;
    delta.y = rawMouseY;
    std::cout << "1. Raw Input: delta.y = " << delta.y << std::endl;
    
    // Step 2: Process through InputProcessor
    ProcessingConfig config;
    config.sensitivity = 1.0f;
    config.invertY = false;
    config.lockX = false;
    config.lockY = false;
    config.maxSpeed = 1.0f;
    config.deadzone = 0.0f;
    
    InputProcessor processor;
    processor.SetConfig(config);
    
    float stickX, stickY;
    float deltaTime = 0.016f; // 60Hz
    
    // Internal processing steps (mimicking InputProcessor::ProcessDelta)
    float y = delta.y * config.sensitivity / 100.0f;
    std::cout << "2. After sensitivity: y = " << delta.y << " * 1.0 / 100 = " << y << std::endl;
    
    // Invert for natural movement
    y = -y;
    std::cout << "3. After default inversion: y = -(" << -y << ") = " << y << std::endl;
    
    // User inversion (if enabled)
    if (config.invertY) {
        y = -y;
        std::cout << "4. After user inversion: y = " << y << std::endl;
    }
    
    // Actually process
    processor.ProcessDelta(delta, deltaTime, stickX, stickY);
    std::cout << "5. ProcessDelta output: stickY = " << stickY << std::endl;
    
    // Step 3: What would be sent to controller
    std::cout << "6. SetLeftStick(0.0f, " << stickY << ")" << std::endl;
    
    // Step 4: Xbox controller value
    float clampedY = std::max(-1.0f, std::min(1.0f, stickY));
    short xboxValue = static_cast<short>(clampedY * 32767.0f);
    std::cout << "7. Xbox controller value: " << xboxValue << std::endl;
    
    // Step 5: Speed calculation
    float speed = std::abs(delta.y) / 1000.0f / deltaTime;
    std::cout << "8. Speed: " << speed << " m/s" << std::endl;
    
    // Step 6: What JavaScript would see
    std::cout << "\n--- JavaScript Side ---" << std::endl;
    std::cout << "updateSpeed(" << speed << ", " << stickY << ")" << std::endl;
    
    // Simulating the JavaScript bug
    float stickPercent = std::abs(stickY) * 100;  // BUG: uses abs()
    std::cout << "stickPercent = Math.abs(" << stickY << ") * 100 = " << stickPercent << "%" << std::endl;
    
    // What gets passed to visualization
    float stickValue = stickY;  // Should use original signed value
    std::cout << "updateStickVisualization(" << stickValue << ")" << std::endl;
    
    // Visual position
    std::cout << "Stick visual: " << (stickValue > 0 ? "UP (forward)" : stickValue < 0 ? "DOWN (backward)" : "CENTER") << std::endl;
    std::cout << "Movement: " << (stickY > 0 ? "Character moves FORWARD" : stickY < 0 ? "Character moves BACKWARD" : "STOPPED") << std::endl;
}

int main() {
    std::cout << "=== Full Pipeline Test ===" << std::endl;
    std::cout << "Testing what happens from raw mouse input to display\n" << std::endl;
    
    // Test forward movement (mouse moves away from user)
    simulateFullPipeline(-50);
    
    // Test backward movement (mouse moves toward user)
    simulateFullPipeline(50);
    
    // Test small movements
    simulateFullPipeline(-10);
    simulateFullPipeline(10);
    
    return 0;
}