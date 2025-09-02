#pragma once
#include <string>

namespace Mouse2VR {
    class Mouse2VRCore;
}

class Bridge {
public:
    Bridge(Mouse2VR::Mouse2VRCore* core) : m_core(core) {}
    
    // Methods exposed to JavaScript
    void setSensitivity(double value);
    void setUpdateRate(int hz);
    std::string getStatus();
    double getCurrentSpeed();
    
private:
    Mouse2VR::Mouse2VRCore* m_core;
};