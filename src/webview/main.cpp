// STL headers first - MUST come before Windows headers
#include <string>
#include <memory>
#include <filesystem>

// Then Windows-specific headers
#include "common/WindowsHeaders.h"
#include <shellapi.h>
#include <wrl.h>
#include <wil/com.h>
#include <WebView2.h>
#include <Shlwapi.h>
#include "SystemTray.h"
#include "WebViewWindow.h"
#include "core/Mouse2VRCore.h"
#include "common/Logger.h"

using namespace Microsoft::WRL;

// Utility to build a path to the bundled runtime
std::wstring GetFixedRuntimePath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    // Remove filename
    PathRemoveFileSpecW(exePath);

    // Append "WebView2Runtime" (relative folder copied by CMake)
    wchar_t runtimePath[MAX_PATH];
    PathCombineW(runtimePath, exePath, L"WebView2Runtime");

    return std::wstring(runtimePath);
}

// Check if Fixed Runtime is available (preferred over Evergreen)
bool IsFixedRuntimeAvailable() {
    std::wstring runtimePath = GetFixedRuntimePath();
    
    // Check if runtime folder exists
    DWORD attrs = GetFileAttributesW(runtimePath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }
    
    // Check if main WebView2 executable exists in the runtime folder
    wchar_t runtimeExe[MAX_PATH];
    PathCombineW(runtimeExe, runtimePath.c_str(), L"msedgewebview2.exe");
    
    attrs = GetFileAttributesW(runtimeExe);
    return (attrs != INVALID_FILE_ATTRIBUTES);
}

// Check WebView2 availability (fallback to Evergreen if Fixed Runtime not found)
bool IsWebView2Available() {
    // First check for Fixed Runtime (preferred)
    if (IsFixedRuntimeAvailable()) {
        return true;
    }
    
    // Fallback: check for Evergreen Runtime
    try {
        wil::unique_cotaskmem_string version;
        HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &version);
        return SUCCEEDED(hr) && version.get() != nullptr;
    } catch (...) {
        return false;
    }
}

// Application class that manages everything
class Mouse2VRApp {
public:
    Mouse2VRApp() = default;
    ~Mouse2VRApp() = default;

    bool Initialize(HINSTANCE hInstance) {
        m_hInstance = hInstance;
        
        // Check WebView2 availability first
        if (!IsWebView2Available()) {
            std::wstring runtimePath = GetFixedRuntimePath();
            std::string message = "WebView2 Runtime is not available.\n\n";
            
            if (!IsFixedRuntimeAvailable()) {
                message += "Fixed Runtime not found at:\n";
                message += std::string(runtimePath.begin(), runtimePath.end()) + "\n\n";
                message += "Please ensure WebView2Runtime folder is next to the executable,\n";
                message += "or install WebView2 Runtime from:\n";
                message += "https://developer.microsoft.com/en-us/microsoft-edge/webview2/\n\n";
                message += "Or run: winget install Microsoft.EdgeWebView2Runtime";
            } else {
                message += "System WebView2 Runtime not found.\n\n";
                message += "Please install WebView2 Runtime or ensure the\n";
                message += "bundled runtime is properly deployed.";
            }
            
            MessageBoxA(nullptr, message.c_str(), "WebView2 Runtime Missing", MB_OK | MB_ICONERROR);
            return false;
        }
        
        // Initialize logger with error handling
        try {
            Mouse2VR::Logger::Instance().Initialize("logs/mouse2vr.log");
            LOG_INFO("App", "Mouse2VR WebView2 starting...");
        } catch (const std::exception& e) {
            MessageBoxA(nullptr, ("Failed to initialize logger: " + std::string(e.what())).c_str(), 
                       "Mouse2VR Error", MB_OK | MB_ICONERROR);
            return false;
        }
        
        // Initialize core functionality
        m_core = std::make_unique<Mouse2VR::Mouse2VRCore>();
        if (!m_core->Initialize()) {
            LOG_ERROR("App", "Failed to initialize Mouse2VR core");
            return false;
        }
        
        // Register window class
        WNDCLASSEXW wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WindowProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = L"Mouse2VRWebView";
        wcex.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
        
        if (!RegisterClassExW(&wcex)) {
            LOG_ERROR("App", "Failed to register window class");
            return false;
        }
        
        // Create main window with standard title bar
        m_hwnd = CreateWindowExW(
            0,
            L"Mouse2VRWebView",
            L"Mouse2VR Treadmill Bridge",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            1200, 800,
            nullptr,
            nullptr,
            hInstance,
            this  // Pass this as lpParam for WM_CREATE
        );
        
        if (!m_hwnd) {
            LOG_ERROR("App", "Failed to create window");
            return false;
        }
        
        // Initialize system tray
        m_systemTray = std::make_unique<SystemTray>();
        if (!m_systemTray->Initialize(m_hwnd)) {
            LOG_ERROR("App", "Failed to initialize system tray");
            return false;
        }
        
        // Initialize WebView2
        m_webView = std::make_unique<WebViewWindow>();
        if (!m_webView->Initialize(m_hwnd, m_core.get())) {
            LOG_ERROR("App", "Failed to initialize WebView2");
            return false;
        }
        
        // Show window
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        
        // Start core processing
        m_core->Start();
        
        LOG_INFO("App", "Mouse2VR initialized successfully");
        return true;
    }
    
    int Run() {
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // Cleanup
        m_core->Stop();
        m_systemTray->Cleanup();
        
        return static_cast<int>(msg.wParam);
    }
    
    void MinimizeToTray() {
        ShowWindow(m_hwnd, SW_HIDE);
        m_systemTray->ShowBalloon(L"Mouse2VR", L"Running in background");
        m_isMinimizedToTray = true;
        LOG_INFO("App", "Minimized to system tray");
    }
    
    void RestoreFromTray() {
        ShowWindow(m_hwnd, SW_SHOW);
        ShowWindow(m_hwnd, SW_RESTORE);
        SetForegroundWindow(m_hwnd);
        m_isMinimizedToTray = false;
        LOG_INFO("App", "Restored from system tray");
    }
    
private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        Mouse2VRApp* app = nullptr;
        
        if (message == WM_CREATE) {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            app = reinterpret_cast<Mouse2VRApp*>(cs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        } else {
            app = reinterpret_cast<Mouse2VRApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }
        
        if (app) {
            return app->HandleMessage(hwnd, message, wParam, lParam);
        }
        
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
            case WM_SIZE:
                if (m_webView) {
                    RECT bounds;
                    GetClientRect(hwnd, &bounds);
                    m_webView->Resize(bounds);
                }
                
                // Let Windows handle normal minimize behavior
                // if (wParam == SIZE_MINIMIZED) {
                //     MinimizeToTray();
                //     return 0;
                // }
                break;
                
            case WM_SYSCOMMAND:
                // Let Windows handle normal minimize behavior
                // if (wParam == SC_MINIMIZE) {
                //     MinimizeToTray();
                //     return 0;
                // }
                break;
                
            case WM_CLOSE:
                // Normal close behavior
                DestroyWindow(hwnd);
                return 0;
                
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
                
            case WM_TRAYICON:
                return HandleTrayMessage(wParam, lParam);
                
            default:
                return DefWindowProc(hwnd, message, wParam, lParam);
        }
        
        return 0;
    }
    
    LRESULT HandleTrayMessage(WPARAM wParam, LPARAM lParam) {
        switch (lParam) {
            case WM_LBUTTONDBLCLK:
                // Double-click to restore
                if (m_isMinimizedToTray) {
                    RestoreFromTray();
                }
                break;
                
            case WM_RBUTTONUP:
                // Right-click for context menu
                ShowTrayMenu();
                break;
        }
        
        return 0;
    }
    
    void ShowTrayMenu() {
        POINT pt;
        GetCursorPos(&pt);
        
        HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING, ID_TRAY_OPEN, L"Open");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        
        if (m_core->IsRunning()) {
            AppendMenuW(menu, MF_STRING, ID_TRAY_STOP, L"Stop");
        } else {
            AppendMenuW(menu, MF_STRING, ID_TRAY_START, L"Start");
        }
        
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Exit");
        
        SetForegroundWindow(m_hwnd);
        
        int cmd = TrackPopupMenu(menu, 
            TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
            pt.x, pt.y, 0, m_hwnd, nullptr);
        
        DestroyMenu(menu);
        
        switch (cmd) {
            case ID_TRAY_OPEN:
                RestoreFromTray();
                break;
            case ID_TRAY_START:
                m_core->Start();
                m_webView->ExecuteScript(L"updateStatus(true)");
                LOG_INFO("App", "Started from system tray");
                break;
            case ID_TRAY_STOP:
                m_core->Stop();
                m_webView->ExecuteScript(L"updateStatus(false)");
                LOG_INFO("App", "Stopped from system tray");
                break;
            case ID_TRAY_EXIT:
                DestroyWindow(m_hwnd);
                break;
        }
    }
    
    // Tray menu IDs
    enum {
        ID_TRAY_OPEN = 1001,
        ID_TRAY_START = 1002,
        ID_TRAY_STOP = 1003,
        ID_TRAY_EXIT = 1004
    };
    
    HINSTANCE m_hInstance = nullptr;
    HWND m_hwnd = nullptr;
    std::unique_ptr<SystemTray> m_systemTray;
    std::unique_ptr<WebViewWindow> m_webView;
    std::unique_ptr<Mouse2VR::Mouse2VRCore> m_core;
    bool m_isMinimizedToTray = false;
};

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    try {
        // Initialize COM for WebView2
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) {
            MessageBoxA(nullptr, "Failed to initialize COM", "Mouse2VR Error", MB_OK | MB_ICONERROR);
            return 1;
        }
        
        Mouse2VRApp app;
        if (!app.Initialize(hInstance)) {
            MessageBoxW(nullptr, L"Failed to initialize Mouse2VR", L"Error", MB_OK | MB_ICONERROR);
            CoUninitialize();
            return 1;
        }
        
        int result = app.Run();
        
        CoUninitialize();
        return result;
        
    } catch (const std::exception& e) {
        std::string error = "Unhandled exception: ";
        error += e.what();
        MessageBoxA(nullptr, error.c_str(), "Mouse2VR Fatal Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    } catch (...) {
        MessageBoxA(nullptr, "Unknown fatal error occurred", "Mouse2VR Fatal Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }
}