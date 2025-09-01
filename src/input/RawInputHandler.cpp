#include "input/RawInputHandler.h"
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

bool RawInputHandler::Initialize() {
    if (m_initialized) {
        return true;
    }
    
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "Mouse2VRRawInput";
    
    if (!RegisterClassEx(&wc)) {
        std::cerr << "Failed to register window class\n";
        return false;
    }
    
    // Create message-only window
    m_window = CreateWindowEx(
        0,
        "Mouse2VRRawInput",
        "Mouse2VR Raw Input Handler",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!m_window) {
        std::cerr << "Failed to create raw input window\n";
        return false;
    }
    
    // Register for raw mouse input
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;  // Generic desktop controls
    rid.usUsage = 0x02;      // Mouse
    rid.dwFlags = RIDEV_INPUTSINK;  // Get input even when not in foreground
    rid.hwndTarget = m_window;
    
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        std::cerr << "Failed to register raw input device\n";
        DestroyWindow(m_window);
        m_window = nullptr;
        return false;
    }
    
    m_initialized = true;
    std::cout << "Raw Input initialized successfully\n";
    return true;
}

void RawInputHandler::Shutdown() {
    if (m_window) {
        DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    UnregisterClass("Mouse2VRRawInput", GetModuleHandle(nullptr));
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
    }
}

// WindowProc removed - Raw input now handled by MainWindow

} // namespace Mouse2VR