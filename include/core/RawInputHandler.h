#pragma once
#include <atomic>
#include <mutex>
#include "common/WindowsHeaders.h"

namespace Mouse2VR {

struct MouseDelta {
    long x = 0;
    long y = 0;
    
    void reset() {
        x = 0;
        y = 0;
    }
    
    MouseDelta operator+(const MouseDelta& other) const {
        return {x + other.x, y + other.y};
    }
    
    MouseDelta& operator+=(const MouseDelta& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
};

class RawInputHandler {
public:
    RawInputHandler();
    ~RawInputHandler();
    
    bool Initialize(HWND targetWindow);
    void Shutdown();
    
    // Get accumulated deltas since last call (thread-safe)
    MouseDelta GetAndResetDeltas();
    
    // Get current deltas without resetting (thread-safe)
    MouseDelta GetDeltas() const;
    
    // Process Raw Input message
    void ProcessRawInput(LPARAM lParam);
    
    // Direct input injection for testing
    void ProcessRawInputDirect(const RAWINPUT* rawInput);
    
    // Get if initialized
    bool IsInitialized() const { return m_initialized; }

private:
    HWND m_targetWindow = nullptr;
    std::atomic<bool> m_initialized{false};
    
    mutable std::mutex m_deltaMutex;
    MouseDelta m_accumulatedDeltas;
    
    static RawInputHandler* s_instance;
};

} // namespace Mouse2VR