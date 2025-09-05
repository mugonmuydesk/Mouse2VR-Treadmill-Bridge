#pragma once

#include <string>

namespace Mouse2VR {

class PathUtils {
public:
    // Get the directory where the executable is located
    static std::string GetExecutableDirectory();
    
    // Get the directory where the executable is located (wide string version)
    static std::wstring GetExecutableDirectoryW();
    
    // Get the full path to a file relative to the executable directory
    static std::string GetExecutablePath(const std::string& relativePath);
    
    // Get the full path to a file relative to the executable directory (wide string version)
    static std::wstring GetExecutablePathW(const std::wstring& relativePath);
    
    // Convert path to file:/// URL format for WebView2
    static std::wstring PathToFileURL(const std::wstring& path);
    
    // Ensure a directory exists, create if necessary
    static bool EnsureDirectoryExists(const std::string& path);
};

} // namespace Mouse2VR