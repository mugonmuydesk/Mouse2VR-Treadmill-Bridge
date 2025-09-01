#include <windows.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdio>
#include <exception>

#include "ui/MainWindow.h"
#include "input/RawInputHandler.h"
#include "output/ViGEmController.h"
#include "processing/InputProcessor.h"
#include "config/ConfigManager.h"

std::atomic<bool> g_running{true};

void ProcessingThread(Mouse2VR::MainWindow* window,
                     Mouse2VR::RawInputHandler* inputHandler,
                     Mouse2VR::ViGEmController* controller,
                     Mouse2VR::InputProcessor* processor,
                     Mouse2VR::ConfigManager* configManager) {
    
    const auto config = configManager->GetConfig();  // Get copy, not reference
    
    // Main loop timing
    auto lastUpdate = std::chrono::steady_clock::now();
    auto updateInterval = std::chrono::milliseconds(config.updateIntervalMs);
    auto idleInterval = std::chrono::milliseconds(config.idleUpdateIntervalMs);
    
    // Metrics
    float averageUpdateRate = 0.0f;
    int updateCount = 0;
    auto metricsResetTime = std::chrono::steady_clock::now();
    
    // Movement detection for adaptive mode
    bool isMoving = false;
    int idleFrames = 0;
    const int IDLE_THRESHOLD = 10;
    
    while (g_running && !window->ShouldExit()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<float>(now - lastUpdate).count();
        
        // Reload config for dynamic updates
        const auto currentConfig = configManager->GetConfig();
        updateInterval = std::chrono::milliseconds(currentConfig.updateIntervalMs);
        
        // Sleep for interval
        auto currentInterval = (currentConfig.adaptiveMode && !isMoving) ? idleInterval : updateInterval;
        std::this_thread::sleep_until(lastUpdate + currentInterval);
        
        // Get mouse deltas
        Mouse2VR::MouseDelta delta = inputHandler->GetAndResetDeltas();
        
        // Detect movement for adaptive mode
        if (currentConfig.adaptiveMode) {
            if (delta.y != 0 || delta.x != 0) {
                isMoving = true;
                idleFrames = 0;
            } else {
                idleFrames++;
                if (idleFrames >= IDLE_THRESHOLD) {
                    isMoving = false;
                }
            }
        }
        
        // Process input
        float stickX, stickY;
        processor->ProcessDelta(delta, elapsed, stickX, stickY);
        
        // Update virtual controller (Y-axis only for treadmill)
        controller->SetLeftStick(0.0f, stickY);
        controller->Update();
        
        // Calculate update rate
        updateCount++;
        auto metricsDuration = std::chrono::duration<float>(now - metricsResetTime).count();
        if (metricsDuration >= 1.0f) {
            averageUpdateRate = updateCount / metricsDuration;
            updateCount = 0;
            metricsResetTime = now;
        }
        
        // Update UI
        if (delta.x != 0 || delta.y != 0 || updateCount % 10 == 0) {
            window->UpdateStatus(delta, 
                               processor->GetSpeedMetersPerSecond(),
                               processor->GetStickDeflectionPercent(),
                               averageUpdateRate);
        }
        
        lastUpdate = now;
    }
}

int RunApplication(HINSTANCE hInstance) {
    // Load configuration
    Mouse2VR::ConfigManager configManager("config.json");
    if (!configManager.Load()) {
        MessageBox(nullptr, "Created default configuration file", "Mouse2VR", MB_OK | MB_ICONINFORMATION);
    }
    
    // Initialize components
    Mouse2VR::RawInputHandler inputHandler;
    Mouse2VR::ViGEmController controller;
    Mouse2VR::InputProcessor processor;
    
    // Initialize Raw Input
    if (!inputHandler.Initialize()) {
        MessageBox(nullptr, "Failed to initialize Raw Input", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Initialize ViGEm controller
    if (!controller.Initialize()) {
        MessageBox(nullptr, 
                  "Failed to initialize virtual controller.\nMake sure ViGEmBus is installed.",
                  "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Configure processor
    processor.SetConfig(configManager.GetConfig().toProcessingConfig());
    
    // Create and initialize window
    Mouse2VR::MainWindow window;
    if (!window.Initialize(hInstance)) {
        DWORD error = GetLastError();
        char msg[256];
        sprintf_s(msg, "Failed to create window. Error code: %lu", error);
        MessageBox(nullptr, msg, "Window Creation Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Set components
    window.SetComponents(&inputHandler, &controller, &processor, &configManager);
    
    // Show window first
    window.Show();
    
    // NOW start processing thread after everything is initialized
    std::thread processingThread(ProcessingThread, &window, &inputHandler, 
                                &controller, &processor, &configManager);
    
    // Run message loop
    int result = window.Run();
    
    // Cleanup
    g_running = false;
    
    if (processingThread.joinable()) {
        processingThread.join();
    }
    
    // Save configuration
    configManager.Save();
    
    controller.Shutdown();
    inputHandler.Shutdown();
    
    return result;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    try {
        return RunApplication(hInstance);
    } catch (const std::exception& ex) {
        char msg[512];
        sprintf_s(msg, "Unhandled exception: %s", ex.what());
        MessageBox(nullptr, msg, "Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    } catch (...) {
        MessageBox(nullptr, "Unknown fatal error occurred", "Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    }
}