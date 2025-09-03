// Test program to verify mouse input processing
#include <iostream>
#include <iomanip>
#include "core/InputProcessor.h"
#include "core/RawInputHandler.h"

using namespace Mouse2VR;

void testMouseInput(int mouseY, const ProcessingConfig& config) {
    // Simulate mouse delta
    MouseDelta delta;
    delta.x = 0;
    delta.y = mouseY;
    
    // Process through InputProcessor
    InputProcessor processor;
    processor.SetConfig(config);
    
    float stickX, stickY;
    float deltaTime = 0.016f; // 60Hz
    processor.ProcessDelta(delta, deltaTime, stickX, stickY);
    
    // Show results
    std::cout << "Mouse Y: " << std::setw(4) << mouseY 
              << " -> Stick Y: " << std::fixed << std::setprecision(3) << std::setw(7) << stickY
              << " (" << (stickY >= 0 ? "forward" : "backward") << ")"
              << " | Speed: " << processor.GetSpeedMetersPerSecond() << " m/s"
              << std::endl;
}

int main() {
    std::cout << "=== Mouse2VR Input Processing Test ===" << std::endl;
    std::cout << std::endl;
    
    // Default config
    ProcessingConfig config;
    config.sensitivity = 1.0f;
    config.invertY = false;
    config.lockX = false;
    config.lockY = false;
    config.maxSpeed = 1.0f;
    config.deadzone = 0.0f;
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Sensitivity: " << config.sensitivity << std::endl;
    std::cout << "  Invert Y: " << (config.invertY ? "true" : "false") << std::endl;
    std::cout << "  Lock X: " << (config.lockX ? "true" : "false") << std::endl;
    std::cout << "  Lock Y: " << (config.lockY ? "true" : "false") << std::endl;
    std::cout << "  Max Speed: " << config.maxSpeed << std::endl;
    std::cout << "  Deadzone: " << config.deadzone << std::endl;
    std::cout << std::endl;
    
    std::cout << "Test 1: Default Configuration" << std::endl;
    std::cout << "------------------------------" << std::endl;
    testMouseInput(-100, config);
    testMouseInput(-50, config);
    testMouseInput(-10, config);
    testMouseInput(0, config);
    testMouseInput(10, config);
    testMouseInput(50, config);
    testMouseInput(100, config);
    std::cout << std::endl;
    
    std::cout << "Test 2: With Invert Y = true" << std::endl;
    std::cout << "-----------------------------" << std::endl;
    config.invertY = true;
    testMouseInput(-100, config);
    testMouseInput(-50, config);
    testMouseInput(-10, config);
    testMouseInput(0, config);
    testMouseInput(10, config);
    testMouseInput(50, config);
    testMouseInput(100, config);
    std::cout << std::endl;
    
    std::cout << "Test 3: Higher Sensitivity (2.0)" << std::endl;
    std::cout << "---------------------------------" << std::endl;
    config.sensitivity = 2.0f;
    config.invertY = false;
    testMouseInput(-100, config);
    testMouseInput(-50, config);
    testMouseInput(-10, config);
    testMouseInput(0, config);
    testMouseInput(10, config);
    testMouseInput(50, config);
    testMouseInput(100, config);
    std::cout << std::endl;
    
    std::cout << "Test 4: With Deadzone (0.05)" << std::endl;
    std::cout << "-----------------------------" << std::endl;
    config.sensitivity = 1.0f;
    config.deadzone = 0.05f;
    testMouseInput(-100, config);
    testMouseInput(-50, config);
    testMouseInput(-10, config);
    testMouseInput(-5, config);
    testMouseInput(-2, config);
    testMouseInput(0, config);
    testMouseInput(2, config);
    testMouseInput(5, config);
    testMouseInput(10, config);
    testMouseInput(50, config);
    testMouseInput(100, config);
    
    return 0;
}