#include "output/ViGEmController.h"
#include <iostream>
#include <algorithm>

namespace Mouse2VR {

ViGEmController::ViGEmController() {
    // Initialize report to neutral state
    memset(&m_report, 0, sizeof(XUSB_REPORT));
}

ViGEmController::~ViGEmController() {
    Shutdown();
}

bool ViGEmController::Initialize() {
    if (m_connected) {
        return true;
    }
    
    // Allocate ViGEm client
    m_client = vigem_alloc();
    if (!m_client) {
        std::cerr << "Failed to allocate ViGEm client\n";
        return false;
    }
    
    // Connect to ViGEm bus
    const auto retval = vigem_connect(m_client);
    if (!VIGEM_SUCCESS(retval)) {
        std::cerr << "Failed to connect to ViGEm bus: 0x" << std::hex << retval << std::dec << "\n";
        vigem_free(m_client);
        m_client = nullptr;
        return false;
    }
    
    // Allocate Xbox 360 controller
    m_pad = vigem_target_x360_alloc();
    if (!m_pad) {
        std::cerr << "Failed to allocate Xbox 360 controller\n";
        vigem_disconnect(m_client);
        vigem_free(m_client);
        m_client = nullptr;
        return false;
    }
    
    // Add controller to bus
    const auto pir = vigem_target_add(m_client, m_pad);
    if (!VIGEM_SUCCESS(pir)) {
        std::cerr << "Failed to add controller to bus: 0x" << std::hex << pir << std::dec << "\n";
        vigem_target_free(m_pad);
        vigem_disconnect(m_client);
        vigem_free(m_client);
        m_client = nullptr;
        m_pad = nullptr;
        return false;
    }
    
    m_connected = true;
    std::cout << "Virtual Xbox 360 controller connected\n";
    return true;
}

void ViGEmController::Shutdown() {
    if (m_pad) {
        vigem_target_remove(m_client, m_pad);
        vigem_target_free(m_pad);
        m_pad = nullptr;
    }
    
    if (m_client) {
        vigem_disconnect(m_client);
        vigem_free(m_client);
        m_client = nullptr;
    }
    
    m_connected = false;
}

void ViGEmController::SetLeftStick(float x, float y) {
    m_report.sThumbLX = FloatToStick(x);
    m_report.sThumbLY = FloatToStick(y);
}

void ViGEmController::SetRightStick(float x, float y) {
    m_report.sThumbRX = FloatToStick(x);
    m_report.sThumbRY = FloatToStick(y);
}

void ViGEmController::SetButton(int button, bool pressed) {
    if (pressed) {
        m_report.wButtons |= button;
    } else {
        m_report.wButtons &= ~button;
    }
}

void ViGEmController::Update() {
    if (!m_connected || !m_pad) {
        return;
    }
    
    // Only send update if state has changed
    if (memcmp(&m_report, &m_lastReport, sizeof(XUSB_REPORT)) != 0) {
        vigem_target_x360_update(m_client, m_pad, m_report);
        m_lastReport = m_report;
    }
}

SHORT ViGEmController::FloatToStick(float value) {
    // Clamp to -1.0 to 1.0
    value = std::max(-1.0f, std::min(1.0f, value));
    
    // Convert to SHORT range
    return static_cast<SHORT>(value * 32767.0f);
}

} // namespace Mouse2VR