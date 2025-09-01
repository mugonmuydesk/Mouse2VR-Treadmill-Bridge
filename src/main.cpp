#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

#include "input/RawInputHandler.h"
#include "output/ViGEmController.h"
#include "processing/InputProcessor.h"

std::atomic<bool> g_running{true};

void SignalHandler(int signal) {
    std::cout << "\nShutting down...\n";
    g_running = false;
}

void PrintStatus(const Mouse2VR::InputProcessor& processor, const Mouse2VR::MouseDelta& delta) {
    // Clear line and print status
    std::cout << "\r";
    std::cout << "Speed: " << std::fixed << std::setprecision(2) 
              << processor.GetSpeedMetersPerSecond() << " m/s | ";
    std::cout << "Stick: " << std::fixed << std::setprecision(0)
              << processor.GetStickDeflectionPercent() << "% | ";
    std::cout << "Delta: X=" << delta.x << " Y=" << delta.y << "     ";
    std::cout.flush();
}

int main(int argc, char* argv[]) {
    std::cout << "Mouse2VR Treadmill Bridge v1.0\n";
    std::cout << "================================\n\n";
    
    // Set up signal handler for clean shutdown
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    // Initialize components
    Mouse2VR::RawInputHandler inputHandler;
    Mouse2VR::ViGEmController controller;
    Mouse2VR::InputProcessor processor;
    
    // Initialize Raw Input
    if (!inputHandler.Initialize()) {
        std::cerr << "Failed to initialize Raw Input\n";
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
    
    // Configure processor (will be loaded from config file in future)
    Mouse2VR::ProcessingConfig config;
    config.sensitivity = 1.0f;
    config.invertY = false;  // Treadmill forward = mouse up = stick forward
    config.lockX = true;     // Only forward/back movement for treadmill
    processor.SetConfig(config);
    
    std::cout << "\nStarting main loop. Press Ctrl+C to exit.\n";
    std::cout << "Walk on your treadmill to move in VR!\n\n";
    
    // Main loop
    auto lastUpdate = std::chrono::steady_clock::now();
    const auto updateInterval = std::chrono::milliseconds(10);  // 100 Hz update
    
    // Message pump thread for Raw Input
    std::thread messageThread([&]() {
        MSG msg;
        while (g_running && GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    });
    
    while (g_running) {
        auto now = std::chrono::steady_clock::now();
        
        if (now - lastUpdate >= updateInterval) {
            // Get mouse deltas
            Mouse2VR::MouseDelta delta = inputHandler.GetAndResetDeltas();
            
            // Process input
            float stickX, stickY;
            processor.ProcessDelta(delta, stickX, stickY);
            
            // Update virtual controller
            controller.SetLeftStick(stickX, stickY);
            controller.Update();
            
            // Print status
            if (delta.x != 0 || delta.y != 0) {
                PrintStatus(processor, delta);
            }
            
            lastUpdate = now;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Cleanup
    std::cout << "\n\nShutting down components...\n";
    
    // Send quit message to message thread
    PostQuitMessage(0);
    if (messageThread.joinable()) {
        messageThread.join();
    }
    
    controller.Shutdown();
    inputHandler.Shutdown();
    
    std::cout << "Goodbye!\n";
    return 0;
}