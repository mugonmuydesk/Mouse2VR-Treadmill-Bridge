#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <atomic>
#include <mutex>

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
    
    bool Initialize();
    void Shutdown();
    
    // Get accumulated deltas since last call (thread-safe)
    MouseDelta GetAndResetDeltas();
    
    // Get current deltas without resetting (thread-safe)
    MouseDelta GetDeltas() const;
    
    // Process Raw Input message
    void ProcessRawInput(LPARAM lParam);
    
    // Get if initialized
    bool IsInitialized() const { return m_initialized; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    HWND m_window = nullptr;
    std::atomic<bool> m_initialized{false};
    
    mutable std::mutex m_deltaMutex;
    MouseDelta m_accumulatedDeltas;
    
    static RawInputHandler* s_instance;
};

} // namespace Mouse2VR