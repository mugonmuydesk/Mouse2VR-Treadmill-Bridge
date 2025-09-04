#include <iostream>
#include <cmath>

int main() {
    // From the log: deltaY=27, deflection=0.147029
    // We need to reverse-engineer what countsPerMeter value would cause this
    
    float deltaY = 27;
    float deltaTime = 0.031f;
    float observed_deflection = 0.147029f;
    float observed_speed = 0.896876f;
    
    std::cout << "From log: deltaY=" << deltaY << ", deflection=" << observed_deflection 
              << ", speed=" << observed_speed << " m/s\n\n";
    
    // The deflection formula is:
    // deflection = (deltaY / deltaTime) / dpi * 0.0254 / 6.1
    // Where dpi = countsPerMeter / 39.3701
    
    // Rearranging:
    // dpi = (deltaY / deltaTime) * 0.0254 / 6.1 / deflection
    
    float countsPerSec = deltaY / deltaTime;
    float dpi = countsPerSec * 0.0254f / 6.1f / observed_deflection;
    float countsPerMeter = dpi * 39.3701f;
    
    std::cout << "To get deflection=" << observed_deflection << ":\n";
    std::cout << "  DPI must be: " << dpi << "\n";
    std::cout << "  countsPerMeter must be: " << countsPerMeter << "\n\n";
    
    // Also check using speed formula
    // m_currentSpeed = abs(deltaY) / countsPerMeter / deltaTime
    // So: countsPerMeter = abs(deltaY) / (m_currentSpeed * deltaTime)
    
    float countsPerMeter_from_speed = std::abs(deltaY) / (observed_speed * deltaTime);
    std::cout << "To get speed=" << observed_speed << " m/s:\n";
    std::cout << "  countsPerMeter must be: " << countsPerMeter_from_speed << "\n\n";
    
    // The old default before the fix
    std::cout << "Old default countsPerMeter: 1000\n";
    std::cout << "New default countsPerMeter: 39370.1\n\n";
    
    // Check if it matches the old default
    std::cout << "Hypothesis: The app is using the OLD default of 1000!\n";
    float test_deflection = countsPerSec / (1000.0f / 39.3701f) * 0.0254f / 6.1f;
    float test_speed = std::abs(deltaY) / 1000.0f / deltaTime;
    
    std::cout << "With countsPerMeter=1000:\n";
    std::cout << "  Deflection would be: " << test_deflection << " (observed: " << observed_deflection << ")\n";
    std::cout << "  Speed would be: " << test_speed << " m/s (observed: " << observed_speed << " m/s)\n";
    std::cout << "  Speed error factor: " << test_speed / observed_speed << "\n";
    
    return 0;
}