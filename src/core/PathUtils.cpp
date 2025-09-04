#include "core/PathUtils.h"
#include <Windows.h>
#include <filesystem>
#include <iostream>

namespace Mouse2VR {

std::string PathUtils::GetExecutableDirectory() {
    char buffer[MAX_PATH];
    DWORD result = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    
    if (result == 0 || result == MAX_PATH) {
        std::cerr << "Failed to get executable path\n";
        return "";
    }
    
    std::filesystem::path exePath(buffer);
    return exePath.parent_path().string();
}

std::string PathUtils::GetExecutablePath(const std::string& relativePath) {
    std::string exeDir = GetExecutableDirectory();
    if (exeDir.empty()) {
        // Fallback to current directory
        return relativePath;
    }
    
    std::filesystem::path fullPath = std::filesystem::path(exeDir) / relativePath;
    return fullPath.string();
}

bool PathUtils::EnsureDirectoryExists(const std::string& path) {
    try {
        std::filesystem::create_directories(path);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directory " << path << ": " << e.what() << "\n";
        return false;
    }
}

} // namespace Mouse2VR