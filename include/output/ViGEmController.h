#pragma once
#include <windows.h>
#include <ViGEm/Client.h>
#include <memory>

namespace Mouse2VR {

class ViGEmController {
public:
    ViGEmController();
    ~ViGEmController();
    
    bool Initialize();
    void Shutdown();
    
    // Update stick position (-1.0 to 1.0)
    void SetLeftStick(float x, float y);
    void SetRightStick(float x, float y);
    
    // Update button states
    void SetButton(int button, bool pressed);
    
    // Send current state to virtual controller
    void Update();
    
    bool IsConnected() const { return m_connected; }
    
private:
    PVIGEM_CLIENT m_client = nullptr;
    PVIGEM_TARGET m_pad = nullptr;
    XUSB_REPORT m_report = {};
    XUSB_REPORT m_lastReport = {};  // Track last sent state
    bool m_connected = false;
    
    // Convert float (-1.0 to 1.0) to SHORT (-32768 to 32767)
    static SHORT FloatToStick(float value);
};

} // namespace Mouse2VR