#include <windows.h>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <condition_variable>
#include <mutex>

#include "input/RawInputHandler.h"
#include "output/ViGEmController.h"
#include "processing/InputProcessor.h"
#include "config/ConfigManager.h"

std::atomic<bool> g_running{true};
std::condition_variable g_updateCV;
std::mutex g_updateMutex;

void SignalHandler(int signal) {
    std::cout << "\nShutting down...\n";
    g_running = false;
    g_updateCV.notify_all();  // Wake up main loop to exit
}

void PrintStatus(const Mouse2VR::InputProcessor& processor, 
                 const Mouse2VR::MouseDelta& delta,
                 float updateRate) {
    // Clear line and print status
    std::cout << "\r";
    std::cout << "Speed: " << std::fixed << std::setprecision(2) 
              << processor.GetSpeedMetersPerSecond() << " m/s | ";
    std::cout << "Stick: " << std::fixed << std::setprecision(0)
              << processor.GetStickDeflectionPercent() << "% | ";
    std::cout << "Delta: Y=" << delta.y << " | ";
    std::cout << "Rate: " << std::fixed << std::setprecision(0) 
              << updateRate << " Hz     ";
    std::cout.flush();
}

int main(int argc, char* argv[]) {
    std::cout << "Mouse2VR Treadmill Bridge v1.1\n";
    std::cout << "================================\n\n";
    
    // Set up signal handler for clean shutdown
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    // Load configuration
    Mouse2VR::ConfigManager configManager("config.json");
    if (!configManager.Load()) {
        std::cout << "Using default configuration\n";
    }
    
    const auto& config = configManager.GetConfig();
    
    // Initialize components
    Mouse2VR::RawInputHandler inputHandler;
    Mouse2VR::ViGEmController controller;
    Mouse2VR::InputProcessor processor;
    
    // Create a message-only window for Raw Input (console app needs one)
    HWND messageWindow = CreateWindowEx(0, "STATIC", "Mouse2VR Console", 
                                        0, 0, 0, 0, 0, 
                                        HWND_MESSAGE, nullptr, 
                                        GetModuleHandle(nullptr), nullptr);
    
    // Initialize Raw Input
    if (!inputHandler.Initialize(messageWindow)) {
        std::cerr << "Failed to initialize Raw Input\n";
        if (messageWindow) DestroyWindow(messageWindow);
        return 1;
    }
    std::cout << "✓ Raw Input initialized\n";
    
    // Initialize ViGEm controller
    if (!controller.Initialize()) {
        std::cerr << "Failed to initialize virtual controller\n";
        std::cerr << "Make sure ViGEmBus is installed\n";
        return 1;
    }
    std::cout << "✓ Virtual Xbox 360 controller created\n";
    
    // Configure processor from config file (thread-safe)
    processor.SetConfig(configManager.GetConfig().toProcessingConfig());
    
    std::cout << "\nConfiguration:\n";
    std::cout << "  Update Rate: " << (1000 / config.updateIntervalMs) << " Hz\n";
    std::cout << "  Sensitivity: " << config.sensitivity << "\n";
    std::cout << "  X-Axis: " << (config.lockX ? "Locked" : "Active") << "\n";
    std::cout << "  Y-Axis: " << (config.lockY ? "Locked" : "Active") 
              << (config.invertY ? " (Inverted)" : "") << "\n";
    if (config.adaptiveMode) {
        std::cout << "  Adaptive Mode: ON (" << (1000 / config.idleUpdateIntervalMs) 
                  << " Hz idle)\n";
    }
    
    std::cout << "\nStarting main loop. Press Ctrl+C to exit.\n";
    std::cout << "Walk on your treadmill to move in VR!\n\n";
    
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
    const int IDLE_THRESHOLD = 10;  // Frames without movement before switching to idle
    
    // Message pump thread for Raw Input
    std::thread messageThread([&]() {
        MSG msg;
        while (g_running && GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            
            // Notify main loop when we get input
            if (msg.message == WM_INPUT) {
                g_updateCV.notify_one();
            }
        }
    });
    
    while (g_running) {
        // Wait for timeout or notification
        std::unique_lock<std::mutex> lock(g_updateMutex);
        auto currentInterval = (config.adaptiveMode && !isMoving) ? idleInterval : updateInterval;
        g_updateCV.wait_for(lock, currentInterval);
        lock.unlock();
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<float>(now - lastUpdate).count();
        
        // Get mouse deltas
        Mouse2VR::MouseDelta delta = inputHandler.GetAndResetDeltas();
        
        // Detect movement for adaptive mode
        if (config.adaptiveMode) {
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
        processor.ProcessDelta(delta, elapsed, stickX, stickY);
        
        // Update virtual controller (Y-axis only for treadmill)
        controller.SetLeftStick(0.0f, stickY);
        controller.Update();  // Now optimized to only send on changes
        
        // Calculate update rate
        updateCount++;
        auto metricsDuration = std::chrono::duration<float>(now - metricsResetTime).count();
        if (metricsDuration >= 1.0f) {
            averageUpdateRate = updateCount / metricsDuration;
            updateCount = 0;
            metricsResetTime = now;
        }
        
        // Print status if configured and there's movement
        if (config.showDebugInfo && (delta.x != 0 || delta.y != 0)) {
            PrintStatus(processor, delta, averageUpdateRate);
        }
        
        lastUpdate = now;
    }
    
    // Cleanup
    std::cout << "\n\nShutting down components...\n";
    
    // Save configuration
    configManager.Save();
    
    // Send quit message to message thread
    PostQuitMessage(0);
    if (messageThread.joinable()) {
        messageThread.join();
    }
    
    controller.Shutdown();
    inputHandler.Shutdown();
    
    std::cout << "Configuration saved to config.json\n";
    std::cout << "Goodbye!\n";
    return 0;
}