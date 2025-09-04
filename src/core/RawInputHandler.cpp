#include "core/RawInputHandler.h"
#include "common/Logger.h"
#include <vector>
#include <iostream>

namespace Mouse2VR {

RawInputHandler* RawInputHandler::s_instance = nullptr;

RawInputHandler::RawInputHandler() {
    s_instance = this;
}

RawInputHandler::~RawInputHandler() {
    Shutdown();
    s_instance = nullptr;
}

bool RawInputHandler::Initialize(HWND targetWindow) {
    if (m_initialized) {
        return true;
    }
    
    if (!targetWindow || !IsWindow(targetWindow)) {
        std::cerr << "Invalid target window for Raw Input\n";
        return false;
    }
    
    m_targetWindow = targetWindow;
    
    // Register for raw mouse input on the provided window
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;  // Generic desktop controls
    rid.usUsage = 0x02;      // Mouse
    rid.dwFlags = RIDEV_INPUTSINK;  // Get input even when not in foreground
    rid.hwndTarget = m_targetWindow;
    
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        std::cerr << "Failed to register raw input device\n";
        m_targetWindow = nullptr;
        return false;
    }
    
    m_initialized = true;
    std::cout << "Raw Input initialized successfully\n";
    return true;
}

void RawInputHandler::Shutdown() {
    // Unregister raw input
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_REMOVE;
    rid.hwndTarget = nullptr;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
    
    m_targetWindow = nullptr;
    m_initialized = false;
}

MouseDelta RawInputHandler::GetAndResetDeltas() {
    std::lock_guard<std::mutex> lock(m_deltaMutex);
    MouseDelta result = m_accumulatedDeltas;
    m_accumulatedDeltas.reset();
    return result;
}

MouseDelta RawInputHandler::GetDeltas() const {
    std::lock_guard<std::mutex> lock(m_deltaMutex);
    return m_accumulatedDeltas;
}

void RawInputHandler::ProcessRawInputDirect(const RAWINPUT* raw) {
    if (raw && raw->header.dwType == RIM_TYPEMOUSE) {
        std::lock_guard<std::mutex> lock(m_deltaMutex);
        m_accumulatedDeltas.x += raw->data.mouse.lLastX;
        m_accumulatedDeltas.y += raw->data.mouse.lLastY;
        
        // Debug logging for raw input
        if (raw->data.mouse.lLastY != 0) {
            LOG_DEBUG("RawInput", "Raw mouse Y: " + std::to_string(raw->data.mouse.lLastY) + 
                      " (accumulated: " + std::to_string(m_accumulatedDeltas.y) + ")");
        }
    }
}

void RawInputHandler::ProcessRawInput(LPARAM lParam) {
    UINT dwSize = 0;
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
    
    if (dwSize == 0) {
        return;
    }
    
    std::vector<BYTE> lpb(dwSize);
    
    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
        return;
    }
    
    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(lpb.data());
    
    if (raw->header.dwType == RIM_TYPEMOUSE) {
        std::lock_guard<std::mutex> lock(m_deltaMutex);
        m_accumulatedDeltas.x += raw->data.mouse.lLastX;
        m_accumulatedDeltas.y += raw->data.mouse.lLastY;
        
        // Debug logging for raw input
        if (raw->data.mouse.lLastY != 0) {
            LOG_DEBUG("RawInput", "Raw mouse Y: " + std::to_string(raw->data.mouse.lLastY) + 
                      " (accumulated: " + std::to_string(m_accumulatedDeltas.y) + ")");
        }
    }
}

// WindowProc removed - Raw input now handled by MainWindow

} // namespace Mouse2VR