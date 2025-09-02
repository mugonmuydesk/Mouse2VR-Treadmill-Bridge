#include "common/Logger.h"

#ifdef _WIN32
#include "common/WindowsHeaders.h"
#endif

namespace Mouse2VR {

// Initialize the singleton instance
Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

#ifdef _DEBUG
#ifdef _WIN32
void Logger::OutputToDebugger(const std::string& msg) {
    OutputDebugStringA((msg + "\n").c_str());
}
#endif
#endif

} // namespace Mouse2VR