#pragma once

#include <string>

namespace Mouse2VR {

class PathUtils {
public:
    // Get the directory where the executable is located
    static std::string GetExecutableDirectory();
    
    // Get the full path to a file relative to the executable directory
    static std::string GetExecutablePath(const std::string& relativePath);
    
    // Ensure a directory exists, create if necessary
    static bool EnsureDirectoryExists(const std::string& path);
};

} // namespace Mouse2VR