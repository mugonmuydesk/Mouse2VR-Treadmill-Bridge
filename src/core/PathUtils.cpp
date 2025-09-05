#include "core/PathUtils.h"
#include <Windows.h>
#include <filesystem>
#include <iostream>
#include <algorithm>

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

std::wstring PathUtils::GetExecutableDirectoryW() {
    wchar_t buffer[MAX_PATH];
    DWORD result = GetModuleFileNameW(NULL, buffer, MAX_PATH);
    
    if (result == 0 || result == MAX_PATH) {
        std::wcerr << L"Failed to get executable path\n";
        return L"";
    }
    
    std::filesystem::path exePath(buffer);
    return exePath.parent_path().wstring();
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

std::wstring PathUtils::GetExecutablePathW(const std::wstring& relativePath) {
    std::wstring exeDir = GetExecutableDirectoryW();
    if (exeDir.empty()) {
        // Fallback to current directory
        return relativePath;
    }
    
    std::filesystem::path fullPath = std::filesystem::path(exeDir) / relativePath;
    return fullPath.wstring();
}

std::wstring PathUtils::PathToFileURL(const std::wstring& path) {
    // Convert to absolute path if needed
    std::filesystem::path fsPath(path);
    if (!fsPath.is_absolute()) {
        fsPath = std::filesystem::absolute(fsPath);
    }
    
    // Get the native path string and convert backslashes to forward slashes
    std::wstring urlPath = fsPath.wstring();
    std::replace(urlPath.begin(), urlPath.end(), L'\\', L'/');
    
    // Add file:/// prefix
    return L"file:///" + urlPath;
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