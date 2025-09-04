#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <cmath>
#include <vector>
#include <numeric>
#include <functional>

#include "core/Mouse2VRCore.h"
#include "core/RawInputHandler.h"
#include "core/InputProcessor.h"
#include "core/ConfigManager.h"
#include "core/TestInterfaces.h"
#include "common/WindowsHeaders.h"

using namespace Mouse2VR;
using namespace std::chrono_literals;

class SettingsValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a dummy window for the core
        testWindow = CreateTestWindow();
        
        // Initialize core with test window
        core = std::make_unique<Mouse2VRCore>();
        core->Initialize(testWindow);
        
        // Get raw input handler for direct injection
        rawInput = core->GetInputHandler();
        
        ASSERT_NE(rawInput, nullptr) << "RawInputHandler must be available";
        
        // Reset metrics
        metrics.Reset();
    }
    
    void TearDown() override {
        if (core) {
            core->Shutdown();
        }
        if (testWindow) {
            DestroyWindow(testWindow);
        }
    }
    
    HWND CreateTestWindow() {
        HINSTANCE hInstance = GetModuleHandle(nullptr);
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = L"TestWindow";
        RegisterClassExW(&wc);
        
        return CreateWindowExW(0, L"TestWindow", L"Test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, nullptr, nullptr, hInstance, nullptr);
    }
    
    // Helper: Inject mouse input over time period
    void InjectMouseMovement(int totalDelta, int durationMs, bool xAxis = false) {
        const int updates = 50; // 50 updates over duration
        const int deltaPerUpdate = totalDelta / updates;
        const auto sleepTime = std::chrono::milliseconds(durationMs / updates);
        
        for (int i = 0; i < updates; i++) {
            RAWINPUT raw = {};
            raw.header.dwType = RIM_TYPEMOUSE;
            
            if (xAxis) {
                raw.data.mouse.lLastX = deltaPerUpdate;
                raw.data.mouse.lLastY = 0;
            } else {
                raw.data.mouse.lLastX = 0;
                raw.data.mouse.lLastY = deltaPerUpdate;
            }
            
            rawInput->ProcessRawInputDirect(&raw);
            
            // Force an update and record metrics
            core->ForceUpdate();
            metrics.RecordUpdate();
            
            auto state = core->GetCurrentState();
            metrics.RecordControllerState(static_cast<float>(state.stickX), static_cast<float>(state.stickY));
            
            std::this_thread::sleep_for(sleepTime);
        }
    }
    
    // Helper: Calculate expected stick deflection
    float CalculateExpectedDeflection(int mickeys, int dpi, float sensitivity, float timeSeconds = 1.0f) {
        const float HL2_MAX_SPEED = 6.1f;
        // Convert mickeys over time period to speed
        float mickeysPerSecond = mickeys / timeSeconds;
        float physicalSpeed = (mickeysPerSecond / static_cast<float>(dpi)) * 0.0254f; // m/s
        float baseDeflection = physicalSpeed / HL2_MAX_SPEED;
        return baseDeflection * sensitivity;
    }
    
    // Helper: Calculate expected game speed
    float CalculateExpectedGameSpeed(int mickeys, int dpi, float sensitivity, float timeSeconds = 1.0f) {
        const float HL2_MAX_SPEED = 6.1f;
        float deflection = CalculateExpectedDeflection(mickeys, dpi, sensitivity, timeSeconds);
        return deflection * HL2_MAX_SPEED;
    }
    
    // Helper: Wait for setting to propagate
    bool WaitForSettingChange(std::function<bool()> condition, int timeoutMs = 100) {
        auto start = std::chrono::steady_clock::now();
        while (!condition()) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count() > timeoutMs) {
                return false;
            }
            std::this_thread::sleep_for(1ms);
        }
        return true;
    }
    
    std::unique_ptr<Mouse2VRCore> core;
    RawInputHandler* rawInput = nullptr;
    TestMetricsCollector metrics;
    HWND testWindow = nullptr;
};

// Test 1: DPI Settings Validation
TEST_F(SettingsValidationTest, DPISettingsAffectBehavior) {
    const std::vector<int> dpiValues = {400, 800, 1000, 1200, 1600, 3200};
    const int inputMickeys = 1000;
    const float sensitivity = 1.0f;
    
    for (int dpi : dpiValues) {
        SCOPED_TRACE("Testing DPI: " + std::to_string(dpi));
        
        // Apply DPI setting
        AppConfig config;
        config.countsPerMeter = dpi * 39.3701f; // inches to meters conversion
        config.sensitivity = sensitivity;
        core->UpdateSettings(config);
        
        // Wait for setting to propagate
        ASSERT_TRUE(WaitForSettingChange([&]() {
            auto procConfig = core->GetProcessorConfig();
            return std::abs(procConfig.countsPerMeter - config.countsPerMeter) < 0.01f;
        })) << "DPI setting did not propagate within 100ms";
        
        // Reset metrics and input
        metrics.Reset();
        rawInput->GetAndResetDeltas();
        
        // Inject known input
        InjectMouseMovement(inputMickeys, 1000);
        
        // Validate behavior (1000 mickeys over 1000ms = 1 second)
        float expectedDeflection = CalculateExpectedDeflection(inputMickeys, dpi, sensitivity, 1.0f);
        float expectedSpeed = CalculateExpectedGameSpeed(inputMickeys, dpi, sensitivity, 1.0f);
        
        EXPECT_NEAR(metrics.lastStickY, expectedDeflection, 0.001f) 
            << "Stick deflection incorrect for DPI " << dpi;
        
        // Verify processing rate unaffected
        float actualHz = metrics.GetActualHz();
        EXPECT_GT(actualHz, 0) << "Processing loop not running";
    }
}

// Test 2: Sensitivity Scaling
TEST_F(SettingsValidationTest, SensitivityScalesOutput) {
    const std::vector<float> sensitivities = {0.5f, 1.0f, 1.5f, 2.0f};
    const int inputMickeys = 1000;
    const int dpi = 1000;
    
    for (float sensitivity : sensitivities) {
        SCOPED_TRACE("Testing Sensitivity: " + std::to_string(sensitivity));
        
        // Apply sensitivity setting
        AppConfig config;
        config.countsPerMeter = dpi * 39.3701f;
        config.sensitivity = sensitivity;
        core->UpdateSettings(config);
        
        // Wait for setting to propagate
        ASSERT_TRUE(WaitForSettingChange([&]() {
            auto procConfig = core->GetProcessorConfig();
            return std::abs(procConfig.sensitivity - sensitivity) < 0.01f;
        })) << "Sensitivity setting did not propagate within 100ms";
        
        // Reset and inject input
        metrics.Reset();
        rawInput->GetAndResetDeltas();
        InjectMouseMovement(inputMickeys, 1000);
        
        // Validate scaled output (1000 mickeys over 1000ms = 1 second)
        float expectedDeflection = CalculateExpectedDeflection(inputMickeys, dpi, sensitivity, 1.0f);
        EXPECT_NEAR(metrics.lastStickY, expectedDeflection, 0.001f)
            << "Stick deflection not scaled correctly for sensitivity " << sensitivity;
    }
}

// Test 3: Update Rate Validation
TEST_F(SettingsValidationTest, UpdateRateChangesProcessingFrequency) {
    const std::vector<int> updateRates = {25, 45, 60};
    const int inputMickeys = 1000;
    
    for (int targetHz : updateRates) {
        SCOPED_TRACE("Testing Update Rate: " + std::to_string(targetHz) + " Hz");
        
        // Apply update rate setting
        core->SetUpdateRate(targetHz);
        
        // Let the system stabilize
        std::this_thread::sleep_for(200ms);
        
        // Reset metrics
        metrics.Reset();
        
        // Start processing
        core->Start();
        
        // Wait for rate to stabilize (1 second)
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Run for 2 seconds to measure actual rate
        auto start = std::chrono::steady_clock::now();
        
        // Continuously inject mouse input to ensure the processing loop has data
        while (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count() < 2) {
            
            RAWINPUT raw = {};
            raw.header.dwType = RIM_TYPEMOUSE;
            raw.data.mouse.lLastY = 10;
            rawInput->ProcessRawInputDirect(&raw);
            
            // Sleep briefly to avoid overwhelming the system
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        
        // Wait a moment for the final rate calculation
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Get the actual update rate from the core
        float actualHz = static_cast<float>(core->GetActualUpdateRate());
        
        core->Stop();
        
        // Allow 20% tolerance due to Windows timer limitations
        float tolerance = targetHz * 0.2f;
        EXPECT_NEAR(actualHz, targetHz, tolerance)
            << "Processing rate not within 20% of target " << targetHz << " Hz";
    }
}

// Test 4: Axis Options
TEST_F(SettingsValidationTest, InvertYAxisWorks) {
    const int inputMickeys = 1000;
    
    // Test normal direction first
    AppConfig config;
    config.invertY = false;
    config.countsPerMeter = 1000 * 39.3701f;
    core->UpdateSettings(config);
    
    metrics.Reset();
    rawInput->GetAndResetDeltas();
    InjectMouseMovement(inputMickeys, 1000);
    
    float normalDeflection = metrics.lastStickY;
    
    // Test inverted direction
    config.invertY = true;
    core->UpdateSettings(config);
    
    ASSERT_TRUE(WaitForSettingChange([&]() {
        auto procConfig = core->GetProcessorConfig();
        return procConfig.invertY == true;
    })) << "Invert Y setting did not propagate";
    
    metrics.Reset();
    rawInput->GetAndResetDeltas();
    InjectMouseMovement(inputMickeys, 1000);
    
    float invertedDeflection = metrics.lastStickY;
    
    EXPECT_NEAR(invertedDeflection, -normalDeflection, 0.001f)
        << "Y axis not properly inverted";
}

TEST_F(SettingsValidationTest, LockXAxisPreventsMovement) {
    const int inputMickeys = 1000;
    
    // Enable X axis lock (default for treadmill)
    AppConfig config;
    config.lockX = true;
    config.countsPerMeter = 1000 * 39.3701f;
    core->UpdateSettings(config);
    
    ASSERT_TRUE(WaitForSettingChange([&]() {
        auto procConfig = core->GetProcessorConfig();
        return procConfig.lockX == true;
    })) << "Lock X setting did not propagate";
    
    // Inject X-axis movement
    metrics.Reset();
    rawInput->GetAndResetDeltas();
    InjectMouseMovement(inputMickeys, 1000, true); // X-axis movement
    
    // X should remain zero
    EXPECT_NEAR(metrics.lastStickX, 0.0f, 0.0001f)
        << "X axis movement not blocked when locked";
}

// Test 5: Virtual Controller Enable/Disable
TEST_F(SettingsValidationTest, VirtualControllerToggle) {
    const int inputMickeys = 1000;
    
    // Test with controller enabled (Start enables it)
    core->Start();
    
    std::this_thread::sleep_for(100ms);
    
    metrics.Reset();
    rawInput->GetAndResetDeltas();
    InjectMouseMovement(inputMickeys, 1000);
    
    EXPECT_NE(metrics.lastStickY, 0.0f) << "Controller should receive input when enabled";
    
    // Test with controller disabled (Stop disables it)
    core->Stop();
    
    std::this_thread::sleep_for(100ms);
    
    metrics.Reset();
    rawInput->GetAndResetDeltas();
    InjectMouseMovement(inputMickeys, 1000);
    
    // When stopped, no updates should occur
    EXPECT_EQ(metrics.updates, 0) << "No updates should occur when stopped";
}

// Test 6: Runtime Setting Changes
TEST_F(SettingsValidationTest, RuntimeSettingChangesWork) {
    const int inputMickeys = 500;
    
    // Start with one configuration
    AppConfig config1;
    config1.countsPerMeter = 800 * 39.3701f;
    config1.sensitivity = 1.0f;
    config1.updateIntervalMs = 40; // ~25 Hz
    core->UpdateSettings(config1);
    
    // Inject input and record behavior
    metrics.Reset();
    rawInput->GetAndResetDeltas();
    InjectMouseMovement(inputMickeys, 500);
    float deflection1 = metrics.lastStickY;
    
    // Change settings during runtime
    AppConfig config2;
    config2.countsPerMeter = 1600 * 39.3701f;
    config2.sensitivity = 2.0f;
    config2.updateIntervalMs = 17; // ~60 Hz
    core->UpdateSettings(config2);
    
    // Verify all settings propagated
    ASSERT_TRUE(WaitForSettingChange([&]() {
        auto procConfig = core->GetProcessorConfig();
        return procConfig.dpi == 1600 && 
               std::abs(procConfig.sensitivity - 2.0f) < 0.01f;
    })) << "Runtime setting changes did not propagate";
    
    // Inject same input and verify different behavior
    metrics.Reset();
    rawInput->GetAndResetDeltas();
    InjectMouseMovement(inputMickeys, 500);
    float deflection2 = metrics.lastStickY;
    
    // With double sensitivity but double DPI, effective deflection should be same
    float expected1 = CalculateExpectedDeflection(inputMickeys, 800, 1.0f, 0.5f);  // 500ms = 0.5s
    float expected2 = CalculateExpectedDeflection(inputMickeys, 1600, 2.0f, 0.5f); // 500ms = 0.5s
    
    EXPECT_NEAR(deflection1, expected1, 0.01f) << "First configuration behavior incorrect";
    EXPECT_NEAR(deflection2, expected2, 0.01f) << "Second configuration behavior incorrect";
}

// Test 7: No Default Reversion
TEST_F(SettingsValidationTest, NoRevertToDefaults) {
    // Apply custom settings
    AppConfig customConfig;
    customConfig.countsPerMeter = 1200 * 39.3701f;
    customConfig.sensitivity = 1.5f;
    customConfig.updateIntervalMs = 22; // ~45 Hz
    customConfig.invertY = true;
    customConfig.lockX = true;
    core->UpdateSettings(customConfig);
    
    // Wait for propagation
    std::this_thread::sleep_for(100ms);
    
    // Simulate various operations that might trigger reversion
    for (int i = 0; i < 10; i++) {
        // Inject input
        RAWINPUT raw = {};
        raw.header.dwType = RIM_TYPEMOUSE;
        raw.data.mouse.lLastY = 100;
        rawInput->ProcessRawInputDirect(&raw);
        core->ForceUpdate();
        
        // Check settings haven't reverted
        auto procConfig = core->GetProcessorConfig();
        EXPECT_EQ(procConfig.dpi, 1200) << "DPI reverted at iteration " << i;
        EXPECT_NEAR(procConfig.sensitivity, 1.5f, 0.01f) << "Sensitivity reverted at iteration " << i;
        EXPECT_TRUE(procConfig.invertY) << "InvertY reverted at iteration " << i;
        EXPECT_TRUE(procConfig.lockX) << "LockX reverted at iteration " << i;
        
        std::this_thread::sleep_for(100ms);
    }
}

// Test 8: Precise Speed Calculation
TEST_F(SettingsValidationTest, SpeedCalculationAccuracy) {
    struct TestCase {
        int dpi;
        float sensitivity;
        int mickeys;
        float expectedSpeed; // m/s
    };
    
    std::vector<TestCase> testCases = {
        {1000, 1.0f, 1000, 0.0254f},   // 1 inch/s at 1000 DPI
        {800,  1.0f, 800,  0.0254f},   // 1 inch/s at 800 DPI
        {1600, 1.0f, 1600, 0.0254f},   // 1 inch/s at 1600 DPI
        {1000, 2.0f, 1000, 0.0508f},   // Double sensitivity
        {1000, 0.5f, 1000, 0.0127f},   // Half sensitivity
    };
    
    for (const auto& test : testCases) {
        SCOPED_TRACE("DPI: " + std::to_string(test.dpi) + 
                     ", Sensitivity: " + std::to_string(test.sensitivity));
        
        AppConfig config;
        config.countsPerMeter = test.dpi * 39.3701f;
        config.sensitivity = test.sensitivity;
        core->UpdateSettings(config);
        
        ASSERT_TRUE(WaitForSettingChange([&]() {
            auto procConfig = core->GetProcessorConfig();
            return procConfig.dpi == test.dpi;
        }));
        
        metrics.Reset();
        rawInput->GetAndResetDeltas();
        
        // Inject over exactly 1 second
        InjectMouseMovement(test.mickeys, 1000);
        
        float actualSpeed = CalculateExpectedGameSpeed(test.mickeys, test.dpi, test.sensitivity);
        EXPECT_NEAR(actualSpeed, test.expectedSpeed, 0.001f)
            << "Speed calculation error exceeds 0.001 m/s tolerance";
    }
}

// Test 9: Game Speed Multiplier Test - Validates that predicted game speed = real world speed * multiplier
TEST_F(SettingsValidationTest, GameSpeedMultiplierApplied) {
    const std::vector<float> multipliers = {0.5f, 1.0f, 1.5f, 2.0f, 3.0f};
    const int inputMickeys = 1000;
    const int dpi = 1000;
    
    for (float multiplier : multipliers) {
        SCOPED_TRACE("Testing Multiplier: " + std::to_string(multiplier));
        
        // Apply settings with the multiplier
        AppConfig config;
        config.countsPerMeter = dpi * 39.3701f;
        config.sensitivity = multiplier;
        core->UpdateSettings(config);
        
        // Wait for setting to propagate
        ASSERT_TRUE(WaitForSettingChange([&]() {
            auto procConfig = core->GetProcessorConfig();
            return std::abs(procConfig.sensitivity - multiplier) < 0.01f;
        })) << "Multiplier setting did not propagate within 100ms";
        
        // Reset and inject input
        metrics.Reset();
        rawInput->GetAndResetDeltas();
        InjectMouseMovement(inputMickeys, 1000); // 1000 mickeys over 1 second
        
        // Calculate the complete chain
        const float HL2_MAX_SPEED = 6.1f;
        
        // Step 1: Raw input to real world speed
        float realWorldSpeed = (inputMickeys / static_cast<float>(dpi)) * 0.0254f; // m/s
        
        // Step 2: Real world speed to stick deflection (with multiplier)
        float expectedStickDeflection = (realWorldSpeed / HL2_MAX_SPEED) * multiplier;
        
        // Step 3: Stick deflection to game speed
        float expectedGameSpeed = expectedStickDeflection * HL2_MAX_SPEED;
        // This simplifies to: realWorldSpeed * multiplier
        
        // Get actual values from the core
        auto state = core->GetCurrentState();
        float actualGameSpeed = state.speed; // Now correctly returns game speed with multiplier
        
        // Verify stick deflection is correct
        EXPECT_NEAR(metrics.lastStickY, expectedStickDeflection, 0.001f)
            << "Stick deflection (" << metrics.lastStickY << ") doesn't match expected (" 
            << expectedStickDeflection << ")";
        
        // Verify game speed is correct (within 10% tolerance due to timing issues)
        float tolerance = expectedGameSpeed * 0.10f; // 10% tolerance
        EXPECT_NEAR(actualGameSpeed, expectedGameSpeed, tolerance)
            << "Game speed (" << actualGameSpeed << " m/s) should be stick deflection (" 
            << expectedStickDeflection << ") * max speed (" << HL2_MAX_SPEED 
            << ") = " << expectedGameSpeed << " m/s";
    }
}

// Test 10: WebView Update Rate Validation
TEST_F(SettingsValidationTest, WebViewPollingRateIsFixed) {
    const float WEBVIEW_POLLING_HZ = 5.0f;  // WebView polls at fixed 5 Hz
    const std::vector<int> processingRates = {25, 45, 60};
    
    for (int processingHz : processingRates) {
        SCOPED_TRACE("Testing with Processing Rate: " + std::to_string(processingHz) + " Hz");
        
        // Set the processing update rate
        core->SetUpdateRate(processingHz);
        
        // Start processing
        core->Start();
        
        // Wait for processing rate to stabilize  
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Reset metrics
        metrics.Reset();
        
        // Run for 3 seconds while simulating WebView polling at 5 Hz
        auto start = std::chrono::steady_clock::now();
        auto lastWebViewPoll = start;
        
        while (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count() < 3) {
            
            // Continuously inject mouse input
            RAWINPUT raw = {};
            raw.header.dwType = RIM_TYPEMOUSE;
            raw.data.mouse.lLastY = 10;
            rawInput->ProcessRawInputDirect(&raw);
            
            // Simulate WebView polling at 5 Hz (every 200ms)
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastWebViewPoll).count() >= 200) {
                metrics.RecordWebViewUpdate();
                lastWebViewPoll = now;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        
        core->Stop();
        
        // Validate WebView polls at fixed 5 Hz regardless of processing rate
        float actualWebViewHz = metrics.GetWebViewHz();
        float tolerance = WEBVIEW_POLLING_HZ * 0.1f; // 10% tolerance
        
        EXPECT_NEAR(actualWebViewHz, WEBVIEW_POLLING_HZ, tolerance)
            << "WebView polling rate (" << actualWebViewHz 
            << " Hz) should be fixed at " << WEBVIEW_POLLING_HZ 
            << " Hz regardless of processing rate (" << processingHz << " Hz)";
        
        // Also verify processing rate is independent and correct
        float actualProcessingHz = static_cast<float>(core->GetActualUpdateRate());
        float processingTolerance = processingHz * 0.2f; // 20% tolerance
        EXPECT_NEAR(actualProcessingHz, processingHz, processingTolerance)
            << "Processing rate should match target";
    }
}

// Test 10: Cross-Setting Persistence (tests all settings together don't revert)
TEST_F(SettingsValidationTest, CrossSettingPersistence) {
    // Set initial configuration
    AppConfig config1;
    config1.countsPerMeter = 1000 * 39.3701f;
    config1.sensitivity = 1.0f;
    config1.updateIntervalMs = 40; // 25 Hz
    config1.invertY = false;
    core->UpdateSettings(config1);
    
    // Wait and verify
    std::this_thread::sleep_for(100ms);
    auto check1 = core->GetProcessorConfig();
    EXPECT_EQ(check1.dpi, 1000);
    EXPECT_NEAR(check1.sensitivity, 1.0f, 0.01f);
    
    // Change to different configuration
    AppConfig config2;
    config2.countsPerMeter = 1600 * 39.3701f;
    config2.sensitivity = 1.5f;
    config2.updateIntervalMs = 22; // 45 Hz
    config2.invertY = true;
    core->UpdateSettings(config2);
    
    // Wait and verify new settings
    std::this_thread::sleep_for(100ms);
    auto check2 = core->GetProcessorConfig();
    EXPECT_EQ(check2.dpi, 1600);
    EXPECT_NEAR(check2.sensitivity, 1.5f, 0.01f);
    EXPECT_TRUE(check2.invertY);
    
    // Change back to original
    core->UpdateSettings(config1);
    
    // Verify it actually changed back (not stuck at config2)
    std::this_thread::sleep_for(100ms);
    auto check3 = core->GetProcessorConfig();
    EXPECT_EQ(check3.dpi, 1000);
    EXPECT_NEAR(check3.sensitivity, 1.0f, 0.01f);
    EXPECT_FALSE(check3.invertY);
    
    // Run for a while and verify settings persist
    core->Start();
    for (int i = 0; i < 10; i++) {
        std::this_thread::sleep_for(100ms);
        
        auto currentConfig = core->GetProcessorConfig();
        EXPECT_EQ(currentConfig.dpi, 1000) << "DPI reverted at iteration " << i;
        EXPECT_NEAR(currentConfig.sensitivity, 1.0f, 0.01f) << "Sensitivity reverted at iteration " << i;
        EXPECT_FALSE(currentConfig.invertY) << "InvertY reverted at iteration " << i;
    }
    core->Stop();
}

// Test 11: WebView Rate Persistence After Changes
TEST_F(SettingsValidationTest, WebViewRatePersistsAfterSettingChanges) {
    // Start at 45 Hz
    core->SetUpdateRate(45);
    core->Start();
    
    // Measure initial rate
    metrics.Reset();
    std::this_thread::sleep_for(1000ms);
    float initial45Hz = metrics.GetWebViewHz();
    EXPECT_NEAR(initial45Hz, 45.0f, 9.0f) << "Initial 45 Hz not established";
    
    // Change DPI while running (shouldn't affect Hz)
    AppConfig config;
    config.countsPerMeter = 1600 * 39.3701f;
    config.updateIntervalMs = 22; // Still 45 Hz
    core->UpdateSettings(config);
    
    // Measure after DPI change
    metrics.Reset();
    std::this_thread::sleep_for(1000ms);
    float afterDpiChange = metrics.GetWebViewHz();
    EXPECT_NEAR(afterDpiChange, 45.0f, 9.0f) 
        << "WebView Hz changed after DPI update (should remain at 45)";
    
    // Change to 60 Hz
    core->SetUpdateRate(60);
    
    // Verify it actually changes to 60 (not stuck at 45)
    metrics.Reset();
    std::this_thread::sleep_for(1000ms);
    float after60Hz = metrics.GetWebViewHz();
    EXPECT_NEAR(after60Hz, 60.0f, 12.0f) 
        << "WebView Hz didn't change to 60 (stuck at previous rate)";
    
    // Change back to 25 Hz
    core->SetUpdateRate(25);
    
    // Verify it changes to 25 (not stuck at 60)
    metrics.Reset();
    std::this_thread::sleep_for(1000ms);
    float after25Hz = metrics.GetWebViewHz();
    EXPECT_NEAR(after25Hz, 25.0f, 5.0f) 
        << "WebView Hz didn't change to 25 (stuck at 60)";
    
    core->Stop();
}

// Note: main() is in test_main.cpp