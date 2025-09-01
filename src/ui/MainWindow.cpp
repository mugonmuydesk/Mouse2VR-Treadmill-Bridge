#include "ui/MainWindow.h"
#include "input/RawInputHandler.h"
#include "output/ViGEmController.h"
#include "processing/InputProcessor.h"
#include "config/ConfigManager.h"

#include <commctrl.h>
#include <shellapi.h>
#include <sstream>
#include <iomanip>
#include <algorithm>  // For std::min
#include <cstdio>     // For sprintf_s
#include <cstring>    // For strlen

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

namespace Mouse2VR {

MainWindow* MainWindow::s_instance = nullptr;

MainWindow::MainWindow() {
    s_instance = this;
    // Common controls initialization moved to Initialize() method
}

MainWindow::~MainWindow() {
    RemoveTrayIcon();
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
    s_instance = nullptr;
}

bool MainWindow::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    
    // Initialize Common Controls (required for sliders, etc)
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_BAR_CLASSES | ICC_WIN95_CLASSES;
    if (!InitCommonControlsEx(&icc)) {
        MessageBox(nullptr, "Failed to initialize Common Controls", "Init Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = "Mouse2VRMainWindow";
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wc)) {
        DWORD error = GetLastError();
        char msg[256];
        sprintf_s(msg, "Failed to register window class. Error: %lu", error);
        MessageBox(nullptr, msg, "Window Class Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    // Create window
    m_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        "Mouse2VRMainWindow",
        "Mouse2VR Treadmill Bridge v2.1",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME),
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr,
        nullptr,
        hInstance,
        this
    );
    
    if (!m_hwnd) {
        DWORD error = GetLastError();
        char msg[256];
        sprintf_s(msg, "Failed to create window. Error: %lu", error);
        MessageBox(nullptr, msg, "Window Creation Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    CreateControls();
    CreateTrayIcon();
    
    return true;
}

void MainWindow::CreateControls() {
    // TEMPORARY: Skip ALL controls to test if window works without them
    return;
    
    // Title label
    CreateWindow("STATIC", "Mouse2VR Treadmill Bridge",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 10, 780, 30,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    // Status text
    m_statusText = CreateWindow("STATIC", "Status: Initializing...",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 50, 380, 60,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    // Graph area for input visualization
    m_graphArea = CreateWindow("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | SS_SUNKEN,
        10, 120, 380, 200,
        m_hwnd, (HMENU)1001, m_hInstance, nullptr);
    
    // Stick position preview
    m_stickArea = CreateWindow("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | SS_SUNKEN,
        410, 120, 200, 200,
        m_hwnd, (HMENU)1002, m_hInstance, nullptr);
    
    // Settings panel
    CreateWindow("BUTTON", "Settings",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        10, 340, 780, 200,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    // Sensitivity controls using simple buttons and edit
    CreateWindow("STATIC", "Sensitivity:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, 370, 100, 20,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    // Use an edit control instead of trackbar
    m_sensitivityEdit = CreateWindow("EDIT", "1.0",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
        140, 370, 60, 25,
        m_hwnd, (HMENU)2001, m_hInstance, nullptr);
    
    // Add - and + buttons for adjustment
    CreateWindow("BUTTON", "-",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        210, 370, 30, 25,
        m_hwnd, (HMENU)2006, m_hInstance, nullptr);
    
    CreateWindow("BUTTON", "+",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        250, 370, 30, 25,
        m_hwnd, (HMENU)2007, m_hInstance, nullptr);
    
    m_sensitivityLabel = CreateWindow("STATIC", "(0.1 - 3.0)",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        290, 370, 80, 20,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    // Update rate using radio buttons instead of combo
    CreateWindow("STATIC", "Update Rate:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, 410, 100, 20,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    m_rate30Radio = CreateWindow("BUTTON", "30 Hz",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        140, 410, 70, 20,
        m_hwnd, (HMENU)2008, m_hInstance, nullptr);
    
    m_rate50Radio = CreateWindow("BUTTON", "50 Hz",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        220, 410, 70, 20,
        m_hwnd, (HMENU)2009, m_hInstance, nullptr);
    SendMessage(m_rate50Radio, BM_SETCHECK, BST_CHECKED, 0);  // Default 50 Hz
    
    m_rate60Radio = CreateWindow("BUTTON", "60 Hz",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        300, 410, 70, 20,
        m_hwnd, (HMENU)2010, m_hInstance, nullptr);
    
    m_rate100Radio = CreateWindow("BUTTON", "100 Hz",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        380, 410, 70, 20,
        m_hwnd, (HMENU)2011, m_hInstance, nullptr);
    
    // Checkboxes
    m_invertYCheck = CreateWindow("BUTTON", "Invert Y Axis",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        30, 450, 150, 20,
        m_hwnd, (HMENU)2003, m_hInstance, nullptr);
    
    m_lockXCheck = CreateWindow("BUTTON", "Lock X Axis",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        200, 450, 150, 20,
        m_hwnd, (HMENU)2004, m_hInstance, nullptr);
    SendMessage(m_lockXCheck, BM_SETCHECK, BST_CHECKED, 0);  // Default checked
    
    m_adaptiveModeCheck = CreateWindow("BUTTON", "Adaptive Mode",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        370, 450, 150, 20,
        m_hwnd, (HMENU)2005, m_hInstance, nullptr);
    
    // Apply button
    m_applyButton = CreateWindow("BUTTON", "Apply Settings",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        30, 490, 120, 30,
        m_hwnd, (HMENU)3001, m_hInstance, nullptr);
    
    // Minimize to tray button
    CreateWindow("BUTTON", "Minimize to Tray",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        170, 490, 120, 30,
        m_hwnd, (HMENU)3002, m_hInstance, nullptr);
}

void MainWindow::CreateTrayIcon() {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = m_hwnd;
    nid.uID = ID_TRAYICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    strcpy_s(nid.szTip, "Mouse2VR Treadmill Bridge");
    
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void MainWindow::RemoveTrayIcon() {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = m_hwnd;
    nid.uID = ID_TRAYICON;
    
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void MainWindow::Show() {
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);  // Force immediate paint
    SetForegroundWindow(m_hwnd);
    m_visible = true;
}

void MainWindow::Hide() {
    ShowWindow(m_hwnd, SW_HIDE);
    m_visible = false;
}

void MainWindow::ToggleVisibility() {
    if (m_visible) {
        Hide();
    } else {
        Show();
    }
}

int MainWindow::Run() {
    // Show() is now called from main_gui.cpp before starting the thread
    
    // Simple test - write to file using Windows API instead of C runtime
    HANDLE hFile = CreateFile("C:\\Tools\\mouse2vr_run_debug.log", 
                             GENERIC_WRITE, 
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, 
                             CREATE_ALWAYS, 
                             FILE_ATTRIBUTE_NORMAL, 
                             NULL);
    
    if (hFile != INVALID_HANDLE_VALUE) {
        const char* msg1 = "DEBUG: MainWindow::Run() started\n";
        DWORD written;
        WriteFile(hFile, msg1, strlen(msg1), &written, NULL);
        CloseHandle(hFile);
    }
    
    MSG msg;
    int messageCount = 0;
    
    // Try PeekMessage first to see if there are any messages
    if (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
        // Append to log
        hFile = CreateFile("C:\\Tools\\mouse2vr_run_debug.log", 
                          GENERIC_WRITE, 
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL, 
                          OPEN_ALWAYS, 
                          FILE_ATTRIBUTE_NORMAL, 
                          NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            SetFilePointer(hFile, 0, NULL, FILE_END);
            const char* msg2 = "DEBUG: Messages available in queue\n";
            DWORD written;
            WriteFile(hFile, msg2, strlen(msg2), &written, NULL);
            CloseHandle(hFile);
        }
    }
    
    // Try using PeekMessage instead of GetMessage
    bool running = true;
    while (running && !m_shouldExit) {
        // Use PeekMessage with PM_REMOVE to get and remove messages
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            messageCount++;
            
            // Log first few messages for debugging
            if (messageCount <= 5) {
                hFile = CreateFile("C:\\Tools\\mouse2vr_run_debug.log", 
                                  GENERIC_WRITE, 
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, 
                                  OPEN_ALWAYS, 
                                  FILE_ATTRIBUTE_NORMAL, 
                                  NULL);
                if (hFile != INVALID_HANDLE_VALUE) {
                    SetFilePointer(hFile, 0, NULL, FILE_END);
                    char msgBuf[256];
                    sprintf_s(msgBuf, "DEBUG: Processed message %d (msg=0x%X)\n", messageCount, msg.message);
                    DWORD written;
                    WriteFile(hFile, msgBuf, strlen(msgBuf), &written, NULL);
                    CloseHandle(hFile);
                }
            }
        } else {
            // No messages, yield CPU time
            Sleep(1);
        }
    }
    
    return static_cast<int>(msg.wParam);
}

void MainWindow::UpdateStatus(const MouseDelta& delta, float speed, float stickPercent, float updateRate) {
    // Guard against calling before controls are created
    if (!m_statusText || !m_graphArea || !m_stickArea) {
        return;  // Controls not ready yet
    }
    
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // Update current values
    m_currentSpeed = speed;
    m_currentStickX = 0.0f;  // Always 0 for treadmill (Y-axis only)
    m_currentStickY = stickPercent / 100.0f;  // Convert to 0-1 range
    m_currentUpdateRate = updateRate;
    
    // Add to history
    DataPoint point = {
        static_cast<float>(delta.y),
        speed,
        stickPercent
    };
    
    m_dataHistory.push_back(point);
    if (m_dataHistory.size() > MAX_HISTORY) {
        m_dataHistory.pop_front();
    }
    
    // Update status text
    std::stringstream ss;
    ss << "Status: Running\n";
    ss << "Speed: " << std::fixed << std::setprecision(2) << speed << " m/s\n";
    ss << "Stick: " << std::fixed << std::setprecision(0) << stickPercent << "%\n";
    ss << "Update Rate: " << std::fixed << std::setprecision(0) << updateRate << " Hz";
    
    SetWindowText(m_statusText, ss.str().c_str());
    
    // Trigger redraw of graphs
    InvalidateRect(m_graphArea, nullptr, TRUE);
    InvalidateRect(m_stickArea, nullptr, TRUE);
}

void MainWindow::DrawStickPosition(HDC hdc, RECT rect) {
    // Draw background
    HBRUSH bgBrush = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw crosshair
    HPEN grayPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN oldPen = (HPEN)SelectObject(hdc, grayPen);
    
    int centerX = rect.left + (rect.right - rect.left) / 2;
    int centerY = rect.top + (rect.bottom - rect.top) / 2;
    
    MoveToEx(hdc, rect.left, centerY, nullptr);
    LineTo(hdc, rect.right, centerY);
    MoveToEx(hdc, centerX, rect.top, nullptr);
    LineTo(hdc, centerX, rect.bottom);
    
    // Draw circle boundary
    HPEN blackPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    SelectObject(hdc, blackPen);
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, nullBrush);
    
    int radius = std::min(rect.right - rect.left, rect.bottom - rect.top) / 2 - 10;
    Ellipse(hdc, centerX - radius, centerY - radius, centerX + radius, centerY + radius);
    
    // Draw stick position
    int stickX = centerX + static_cast<int>(m_currentStickX * radius);
    int stickY = centerY - static_cast<int>(m_currentStickY * radius);  // Invert Y for display
    
    HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
    SelectObject(hdc, redBrush);
    Ellipse(hdc, stickX - 5, stickY - 5, stickX + 5, stickY + 5);
    
    // Cleanup
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(grayPen);
    DeleteObject(blackPen);
    DeleteObject(redBrush);
}

void MainWindow::DrawInputGraph(HDC hdc, RECT rect) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // Draw background
    HBRUSH bgBrush = CreateSolidBrush(RGB(250, 250, 250));
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    if (m_dataHistory.empty()) {
        return;
    }
    
    // Draw grid
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(230, 230, 230));
    HPEN oldPen = (HPEN)SelectObject(hdc, gridPen);
    
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    // Horizontal lines
    for (int i = 1; i < 4; i++) {
        int y = rect.top + (height * i) / 4;
        MoveToEx(hdc, rect.left, y, nullptr);
        LineTo(hdc, rect.right, y);
    }
    
    // Draw data
    HPEN dataPen = CreatePen(PS_SOLID, 2, RGB(0, 120, 215));
    SelectObject(hdc, dataPen);
    
    float xStep = static_cast<float>(width) / MAX_HISTORY;
    float maxValue = 100.0f;  // Max stick percentage
    
    for (size_t i = 1; i < m_dataHistory.size(); i++) {
        int x1 = rect.left + static_cast<int>((i - 1) * xStep);
        int x2 = rect.left + static_cast<int>(i * xStep);
        
        int y1 = rect.bottom - static_cast<int>((m_dataHistory[i - 1].stickPercent / maxValue) * height);
        int y2 = rect.bottom - static_cast<int>((m_dataHistory[i].stickPercent / maxValue) * height);
        
        MoveToEx(hdc, x1, y1, nullptr);
        LineTo(hdc, x2, y2);
    }
    
    // Cleanup
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);
    DeleteObject(dataPen);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        MainWindow* window = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        return 0;
    }
    
    MainWindow* window = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (window) {
        return window->HandleMessage(hwnd, msg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        m_shouldExit = true;
        return 0;
        
    case WM_CLOSE:
        Hide();  // Minimize to tray instead of closing
        return 0;
        
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONDBLCLK) {
            ToggleVisibility();
        } else if (lParam == WM_RBUTTONUP) {
            // Show context menu
            POINT pt;
            GetCursorPos(&pt);
            
            HMENU menu = CreatePopupMenu();
            AppendMenu(menu, MF_STRING, 4001, m_visible ? "Hide" : "Show");
            AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(menu, MF_STRING, 4002, "Exit");
            
            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
            
            if (cmd == 4001) {
                ToggleVisibility();
            } else if (cmd == 4002) {
                m_shouldExit = true;
                PostQuitMessage(0);
            }
            
            DestroyMenu(menu);
        }
        return 0;
        
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 3001:  // Apply button
            ApplySettings();
            return 0;
        case 3002:  // Minimize to tray
            Hide();
            return 0;
        case 2006:  // Sensitivity - button
            {
                char sensitivityStr[256];
                GetWindowText(m_sensitivityEdit, sensitivityStr, sizeof(sensitivityStr));
                float sensitivity = static_cast<float>(atof(sensitivityStr));
                sensitivity -= 0.1f;
                if (sensitivity < 0.1f) sensitivity = 0.1f;
                sprintf_s(sensitivityStr, "%.1f", sensitivity);
                SetWindowText(m_sensitivityEdit, sensitivityStr);
            }
            return 0;
        case 2007:  // Sensitivity + button
            {
                char sensitivityStr[256];
                GetWindowText(m_sensitivityEdit, sensitivityStr, sizeof(sensitivityStr));
                float sensitivity = static_cast<float>(atof(sensitivityStr));
                sensitivity += 0.1f;
                if (sensitivity > 3.0f) sensitivity = 3.0f;
                sprintf_s(sensitivityStr, "%.1f", sensitivity);
                SetWindowText(m_sensitivityEdit, sensitivityStr);
            }
            return 0;
        }
        break;
        
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (dis->CtlID == 1001) {  // Graph area
            DrawInputGraph(dis->hDC, dis->rcItem);
        } else if (dis->CtlID == 1002) {  // Stick area
            DrawStickPosition(dis->hDC, dis->rcItem);
        }
        return TRUE;
    }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void MainWindow::SetComponents(RawInputHandler* input, ViGEmController* controller,
                               InputProcessor* processor, ConfigManager* config) {
    m_inputHandler = input;
    m_controller = controller;
    m_processor = processor;
    m_configManager = config;
    
    LoadSettings();
}

void MainWindow::LoadSettings() {
    if (!m_configManager) return;
    
    // Make sure controls exist before trying to use them
    if (!m_sensitivityEdit || !m_rate50Radio || !m_invertYCheck || 
        !m_lockXCheck || !m_adaptiveModeCheck) {
        return;  // Controls not created yet
    }
    
    const auto config = m_configManager->GetConfig();  // Thread-safe copy
    
    // Set sensitivity edit box
    char sensitivityStr[16];
    sprintf_s(sensitivityStr, "%.1f", config.sensitivity);
    SetWindowText(m_sensitivityEdit, sensitivityStr);
    
    // Set update rate radio buttons
    SendMessage(m_rate30Radio, BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessage(m_rate50Radio, BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessage(m_rate60Radio, BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessage(m_rate100Radio, BM_SETCHECK, BST_UNCHECKED, 0);
    
    if (config.updateIntervalMs >= 33) 
        SendMessage(m_rate30Radio, BM_SETCHECK, BST_CHECKED, 0);      // 30Hz
    else if (config.updateIntervalMs >= 20) 
        SendMessage(m_rate50Radio, BM_SETCHECK, BST_CHECKED, 0);      // 50Hz
    else if (config.updateIntervalMs >= 16) 
        SendMessage(m_rate60Radio, BM_SETCHECK, BST_CHECKED, 0);      // 60Hz
    else 
        SendMessage(m_rate100Radio, BM_SETCHECK, BST_CHECKED, 0);     // 100Hz
    
    // Set checkboxes
    SendMessage(m_invertYCheck, BM_SETCHECK, config.invertY ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(m_lockXCheck, BM_SETCHECK, config.lockX ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(m_adaptiveModeCheck, BM_SETCHECK, config.adaptiveMode ? BST_CHECKED : BST_UNCHECKED, 0);
}

void MainWindow::ApplySettings() {
    if (!m_configManager || !m_processor) return;
    
    auto config = m_configManager->GetConfig();  // Thread-safe copy
    
    // Get sensitivity from edit box
    char sensitivityStr[256];
    GetWindowText(m_sensitivityEdit, sensitivityStr, sizeof(sensitivityStr));
    float sensitivity = static_cast<float>(atof(sensitivityStr));
    // Clamp to valid range
    if (sensitivity < 0.1f) sensitivity = 0.1f;
    if (sensitivity > 3.0f) sensitivity = 3.0f;
    config.sensitivity = sensitivity;
    
    // Get update rate from radio buttons
    if (SendMessage(m_rate30Radio, BM_GETCHECK, 0, 0) == BST_CHECKED)
        config.updateIntervalMs = 33;  // 30Hz
    else if (SendMessage(m_rate50Radio, BM_GETCHECK, 0, 0) == BST_CHECKED)
        config.updateIntervalMs = 20;  // 50Hz
    else if (SendMessage(m_rate60Radio, BM_GETCHECK, 0, 0) == BST_CHECKED)
        config.updateIntervalMs = 16;  // 60Hz
    else if (SendMessage(m_rate100Radio, BM_GETCHECK, 0, 0) == BST_CHECKED)
        config.updateIntervalMs = 10;  // 100Hz
    
    // Get checkboxes
    config.invertY = SendMessage(m_invertYCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    config.lockX = SendMessage(m_lockXCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    config.adaptiveMode = SendMessage(m_adaptiveModeCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    
    // Apply to processor
    m_processor->SetConfig(config.toProcessingConfig());
    
    // Update config manager and save to file
    m_configManager->SetConfig(config);  // Thread-safe update
    m_configManager->Save();
    
    MessageBox(m_hwnd, "Settings applied and saved!", "Success", MB_OK | MB_ICONINFORMATION);
}

} // namespace Mouse2VR