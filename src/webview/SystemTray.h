#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAYICON 1

class SystemTray {
public:
    SystemTray();
    ~SystemTray();
    
    bool Initialize(HWND hwnd);
    void Cleanup();
    
    void ShowBalloon(LPCWSTR title, LPCWSTR message, DWORD flags = NIIF_INFO);
    void SetTooltip(LPCWSTR tooltip);
    void SetIcon(HICON icon);
    void UpdateStatus(bool isRunning);
    
    bool IsVisible() const { return m_isVisible; }
    void Show();
    void Hide();
    
private:
    NOTIFYICONDATAW m_nid;
    HWND m_hwnd;
    bool m_isInitialized;
    bool m_isVisible;
    HICON m_iconRunning;
    HICON m_iconStopped;
    
    void CreateDefaultIcons();
};