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
    
    auto config = configManager->GetConfig();  // Get copy, not reference
    
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
        auto currentConfig = configManager->GetConfig();  // Copy, not reference
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
    // Open debug log file
    FILE* debugLog = nullptr;
    fopen_s(&debugLog, "C:\\Tools\\mouse2vr_debug.log", "w");
    if (!debugLog) {
        MessageBox(nullptr, "Failed to create debug log", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    fprintf(debugLog, "DEBUG: Starting Mouse2VR GUI initialization...\n");
    fflush(debugLog);
    
    // Load configuration
    Mouse2VR::ConfigManager configManager("config.json");
    if (!configManager.Load()) {
        fprintf(debugLog, "DEBUG: Created default configuration file\n");
        fflush(debugLog);
    }
    
    fprintf(debugLog, "DEBUG: Creating window...\n");
    fflush(debugLog);
    
    // Create and initialize window FIRST - before any other components
    Mouse2VR::MainWindow window;
    if (!window.Initialize(hInstance)) {
        DWORD error = GetLastError();
        fprintf(debugLog, "ERROR: Failed to create window. Error code: %lu\n", error);
        fflush(debugLog);
        fclose(debugLog);
        return 1;
    }
    
    fprintf(debugLog, "DEBUG: Window created, showing window...\n");
    fflush(debugLog);
    
    // Show window immediately so user sees something
    window.Show();
    fprintf(debugLog, "DEBUG: Window shown, initializing components...\n");
    fflush(debugLog);
    
    // Initialize components
    Mouse2VR::RawInputHandler inputHandler;
    Mouse2VR::ViGEmController controller;
    Mouse2VR::InputProcessor processor;
    
    fprintf(debugLog, "DEBUG: Initializing Raw Input...\n");
    fflush(debugLog);
    
    // Initialize Raw Input
    if (!inputHandler.Initialize()) {
        fprintf(debugLog, "ERROR: Failed to initialize Raw Input\n");
        fflush(debugLog);
        fclose(debugLog);
        MessageBox(nullptr, "Failed to initialize Raw Input", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    fprintf(debugLog, "DEBUG: Raw Input initialized, initializing ViGEm...\n");
    fflush(debugLog);
    
    // Initialize ViGEm controller
    if (!controller.Initialize()) {
        fprintf(debugLog, "ERROR: Failed to initialize ViGEm controller\n");
        fflush(debugLog);
        fclose(debugLog);
        MessageBox(nullptr, 
                  "Failed to initialize virtual controller.\nMake sure ViGEmBus is installed.",
                  "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    fprintf(debugLog, "DEBUG: ViGEm initialized successfully!\n");
    fflush(debugLog);
    
    // Configure processor
    fprintf(debugLog, "DEBUG: Configuring processor...\n");
    fflush(debugLog);
    processor.SetConfig(configManager.GetConfig().toProcessingConfig());
    
    // Set components
    fprintf(debugLog, "DEBUG: Setting window components...\n");
    fflush(debugLog);
    window.SetComponents(&inputHandler, &controller, &processor, &configManager);
    
    // NOW start processing thread after everything is initialized
    fprintf(debugLog, "DEBUG: Starting processing thread...\n");
    fflush(debugLog);
    std::thread processingThread(ProcessingThread, &window, &inputHandler, 
                                &controller, &processor, &configManager);
    
    // Run message loop
    fprintf(debugLog, "DEBUG: Entering window message loop (Run())...\n");
    fflush(debugLog);
    int result = window.Run();
    
    fprintf(debugLog, "DEBUG: Window.Run() returned with result: %d\n", result);
    fflush(debugLog);
    
    // Cleanup
    g_running = false;
    
    if (processingThread.joinable()) {
        fprintf(debugLog, "DEBUG: Waiting for processing thread to finish...\n");
        fflush(debugLog);
        processingThread.join();
    }
    
    // Save configuration
    fprintf(debugLog, "DEBUG: Saving configuration...\n");
    fflush(debugLog);
    configManager.Save();
    
    fprintf(debugLog, "DEBUG: Shutting down components...\n");
    fflush(debugLog);
    controller.Shutdown();
    inputHandler.Shutdown();
    
    fprintf(debugLog, "DEBUG: Application exit complete\n");
    fclose(debugLog);
    
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