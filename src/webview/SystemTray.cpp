#include "common/Logger.h"  // Must be first - includes Windows.h
#include "SystemTray.h"
#include <strsafe.h>

SystemTray::SystemTray() 
    : m_hwnd(nullptr)
    , m_isInitialized(false)
    , m_isVisible(false)
    , m_iconRunning(nullptr)
    , m_iconStopped(nullptr) {
    ZeroMemory(&m_nid, sizeof(m_nid));
}

SystemTray::~SystemTray() {
    Cleanup();
    
    if (m_iconRunning) {
        DestroyIcon(m_iconRunning);
    }
    if (m_iconStopped) {
        DestroyIcon(m_iconStopped);
    }
}

bool SystemTray::Initialize(HWND hwnd) {
    if (m_isInitialized) {
        return true;
    }
    
    m_hwnd = hwnd;
    
    // Create default icons
    CreateDefaultIcons();
    
    // Setup NOTIFYICONDATA structure
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hwnd;
    m_nid.uID = ID_TRAYICON;
    m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = m_iconStopped;
    m_nid.uVersion = NOTIFYICON_VERSION_4;
    
    // Set default tooltip
    StringCchCopyW(m_nid.szTip, ARRAYSIZE(m_nid.szTip), L"Mouse2VR - Stopped");
    
    // Add the icon to the system tray
    if (!Shell_NotifyIconW(NIM_ADD, &m_nid)) {
        LOG_ERROR("SystemTray", "Failed to add tray icon");
        return false;
    }
    
    // Set version for enhanced features
    Shell_NotifyIconW(NIM_SETVERSION, &m_nid);
    
    m_isInitialized = true;
    m_isVisible = true;
    
    LOG_INFO("SystemTray", "System tray initialized successfully");
    return true;
}

void SystemTray::Cleanup() {
    if (m_isInitialized) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_isInitialized = false;
        m_isVisible = false;
        LOG_INFO("SystemTray", "System tray cleaned up");
    }
}

void SystemTray::ShowBalloon(LPCWSTR title, LPCWSTR message, DWORD flags) {
    if (!m_isInitialized) return;
    
    m_nid.uFlags |= NIF_INFO;
    m_nid.dwInfoFlags = flags;
    
    StringCchCopyW(m_nid.szInfoTitle, ARRAYSIZE(m_nid.szInfoTitle), title);
    StringCchCopyW(m_nid.szInfo, ARRAYSIZE(m_nid.szInfo), message);
    
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
    
    // Clear the balloon notification flags after showing
    m_nid.uFlags &= ~NIF_INFO;
}

void SystemTray::SetTooltip(LPCWSTR tooltip) {
    if (!m_isInitialized) return;
    
    StringCchCopyW(m_nid.szTip, ARRAYSIZE(m_nid.szTip), tooltip);
    m_nid.uFlags |= NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void SystemTray::SetIcon(HICON icon) {
    if (!m_isInitialized || !icon) return;
    
    m_nid.hIcon = icon;
    m_nid.uFlags |= NIF_ICON;
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void SystemTray::UpdateStatus(bool isRunning) {
    if (!m_isInitialized) return;
    
    if (isRunning) {
        SetIcon(m_iconRunning);
        SetTooltip(L"Mouse2VR - Running");
    } else {
        SetIcon(m_iconStopped);
        SetTooltip(L"Mouse2VR - Stopped");
    }
    
    LOG_INFO("SystemTray", "Status updated: " + std::string(isRunning ? "Running" : "Stopped"));
}

void SystemTray::Show() {
    if (m_isInitialized && !m_isVisible) {
        Shell_NotifyIconW(NIM_ADD, &m_nid);
        m_isVisible = true;
    }
}

void SystemTray::Hide() {
    if (m_isInitialized && m_isVisible) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_isVisible = false;
    }
}

void SystemTray::CreateDefaultIcons() {
    // Create simple colored icons programmatically
    // Green for running, red for stopped
    
    // Create device context
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    
    // Create bitmaps for the icons (16x16)
    const int iconSize = 16;
    HBITMAP hbmColor = CreateCompatibleBitmap(hdcScreen, iconSize, iconSize);
    HBITMAP hbmMask = CreateCompatibleBitmap(hdcScreen, iconSize, iconSize);
    
    // Create running icon (green circle)
    SelectObject(hdcMem, hbmColor);
    HBRUSH greenBrush = CreateSolidBrush(RGB(0, 200, 0));
    RECT rect = {0, 0, iconSize, iconSize};
    FillRect(hdcMem, &rect, greenBrush);
    DeleteObject(greenBrush);
    
    // Create mask
    SelectObject(hdcMem, hbmMask);
    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdcMem, &rect, blackBrush);
    DeleteObject(blackBrush);
    
    // Create icon
    ICONINFO iconInfo = {};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = hbmColor;
    iconInfo.hbmMask = hbmMask;
    m_iconRunning = CreateIconIndirect(&iconInfo);
    
    // Create stopped icon (red circle)
    SelectObject(hdcMem, hbmColor);
    HBRUSH redBrush = CreateSolidBrush(RGB(200, 0, 0));
    FillRect(hdcMem, &rect, redBrush);
    DeleteObject(redBrush);
    
    m_iconStopped = CreateIconIndirect(&iconInfo);
    
    // Cleanup
    DeleteObject(hbmColor);
    DeleteObject(hbmMask);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    
    // If icon creation failed, use default application icon
    if (!m_iconRunning) {
        m_iconRunning = LoadIcon(nullptr, IDI_APPLICATION);
    }
    if (!m_iconStopped) {
        m_iconStopped = LoadIcon(nullptr, IDI_ERROR);
    }
}