#include <iostream>
#include <cmath>
#include <iomanip>

// Simulate the exact calculations from InputProcessor
void testSpeedCalculation(int deltaY, float deltaTime) {
    // Constants from the code
    float countsPerMeter = 39370.1f;  // Default: 1000 DPI * 39.3701 inches/meter
    float sensitivity = 1.0f;         // Default sensitivity
    
    std::cout << "\n=== Test Case ===" << std::endl;
    std::cout << "Input: deltaY=" << deltaY << " counts, deltaTime=" << deltaTime << " seconds" << std::endl;
    std::cout << "Config: countsPerMeter=" << countsPerMeter << ", sensitivity=" << sensitivity << std::endl;
    
    // Method 1: How m_currentSpeed is calculated (line 17 of InputProcessor.cpp)
    float m_currentSpeed = std::abs(deltaY) / countsPerMeter / deltaTime;
    std::cout << "\nMethod 1 (m_currentSpeed calculation):" << std::endl;
    std::cout << "  speed = abs(" << deltaY << ") / " << countsPerMeter << " / " << deltaTime << std::endl;
    std::cout << "  speed = " << m_currentSpeed << " m/s" << std::endl;
    
    // Method 2: How stick deflection is calculated (lines 23-36)
    float dpi = countsPerMeter / 39.3701f;
    std::cout << "\nMethod 2 (stick deflection calculation):" << std::endl;
    std::cout << "  DPI = " << countsPerMeter << " / 39.3701 = " << dpi << std::endl;
    
    float countsPerSecY = deltaY / deltaTime;
    std::cout << "  countsPerSecY = " << deltaY << " / " << deltaTime << " = " << countsPerSecY << std::endl;
    
    // The actual formula used for stick deflection
    float y_deflection = countsPerSecY / dpi * 0.0254f / 6.1f;
    std::cout << "  deflection = " << countsPerSecY << " / " << dpi << " * 0.0254 / 6.1" << std::endl;
    std::cout << "  deflection = " << y_deflection << std::endl;
    
    // Apply sensitivity
    float y_with_sensitivity = y_deflection * sensitivity;
    std::cout << "  deflection with sensitivity = " << y_with_sensitivity << std::endl;
    
    // Calculate game speed (what the user sees in game)
    float gameSpeed = std::abs(y_with_sensitivity) * 6.1f;
    std::cout << "\nGame speed (deflection * 6.1) = " << gameSpeed << " m/s" << std::endl;
    
    // What SHOULD be the correct physical speed
    std::cout << "\n=== Correct Physical Speed Calculation ===" << std::endl;
    float inches_moved = std::abs(deltaY) / dpi;
    float meters_moved = inches_moved * 0.0254f;
    float correct_speed = meters_moved / deltaTime;
    std::cout << "  Inches moved: " << std::abs(deltaY) << " / " << dpi << " = " << inches_moved << " inches" << std::endl;
    std::cout << "  Meters moved: " << inches_moved << " * 0.0254 = " << meters_moved << " meters" << std::endl;
    std::cout << "  Physical speed: " << meters_moved << " / " << deltaTime << " = " << correct_speed << " m/s" << std::endl;
    
    // Compare
    std::cout << "\n=== Comparison ===" << std::endl;
    std::cout << "  Method 1 result: " << m_currentSpeed << " m/s" << std::endl;
    std::cout << "  Correct physical: " << correct_speed << " m/s" << std::endl;
    std::cout << "  Game speed shown: " << gameSpeed << " m/s" << std::endl;
    std::cout << "  Error factor: " << (m_currentSpeed / correct_speed) << "x" << std::endl;
}

int main() {
    std::cout << std::fixed << std::setprecision(6);
    
    std::cout << "Testing Mouse2VR Speed Calculations\n";
    std::cout << "====================================\n";
    
    // Test case from your log: 27 counts at ~32Hz
    std::cout << "\n--- Test 1: Your actual mouse movement ---" << std::endl;
    testSpeedCalculation(27, 0.031f);  // 27 counts in 31ms
    
    // Test case: smaller movement
    std::cout << "\n--- Test 2: Small movement ---" << std::endl;
    testSpeedCalculation(5, 0.031f);
    
    // Test case: 1 count per update
    std::cout << "\n--- Test 3: Minimal movement ---" << std::endl;
    testSpeedCalculation(1, 0.031f);
    
    // Test case: What 1 cm/s SHOULD look like at 1000 DPI
    std::cout << "\n--- Test 4: What 1 cm/s should produce ---" << std::endl;
    // At 1000 DPI, 1 cm = 0.393701 inches = 393.7 counts
    // In 31ms: 393.7 * 0.031 = 12.2 counts
    testSpeedCalculation(12, 0.031f);
    
    return 0;
}