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

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

namespace Mouse2VR {

MainWindow* MainWindow::s_instance = nullptr;

MainWindow::MainWindow() {
    s_instance = this;
    
    // Initialize common controls
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);
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
        return false;
    }
    
    // Create window
    m_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        "Mouse2VRMainWindow",
        "Mouse2VR Treadmill Bridge v2.1",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr,
        nullptr,
        hInstance,
        this
    );
    
    if (!m_hwnd) {
        return false;
    }
    
    CreateControls();
    CreateTrayIcon();
    
    return true;
}

void MainWindow::CreateControls() {
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
    
    // Sensitivity slider
    CreateWindow("STATIC", "Sensitivity:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, 370, 100, 20,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    m_sensitivitySlider = CreateWindow(TRACKBAR_CLASS, "",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_TOOLTIPS,
        140, 370, 200, 30,
        m_hwnd, (HMENU)2001, m_hInstance, nullptr);
    SendMessage(m_sensitivitySlider, TBM_SETRANGE, TRUE, MAKELONG(10, 300));
    SendMessage(m_sensitivitySlider, TBM_SETPOS, TRUE, 100);
    
    m_sensitivityLabel = CreateWindow("STATIC", "1.0",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        350, 370, 50, 20,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    // Update rate combo
    CreateWindow("STATIC", "Update Rate:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, 410, 100, 20,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    m_updateRateCombo = CreateWindow("COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        140, 410, 120, 100,
        m_hwnd, (HMENU)2002, m_hInstance, nullptr);
    
    SendMessage(m_updateRateCombo, CB_ADDSTRING, 0, (LPARAM)"30 Hz");
    SendMessage(m_updateRateCombo, CB_ADDSTRING, 0, (LPARAM)"50 Hz");
    SendMessage(m_updateRateCombo, CB_ADDSTRING, 0, (LPARAM)"60 Hz");
    SendMessage(m_updateRateCombo, CB_ADDSTRING, 0, (LPARAM)"100 Hz");
    SendMessage(m_updateRateCombo, CB_SETCURSEL, 1, 0);  // Default 50 Hz
    
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
    Show();
    
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        
        if (m_shouldExit) {
            break;
        }
    }
    
    return static_cast<int>(msg.wParam);
}

void MainWindow::UpdateStatus(const MouseDelta& delta, float speed, float stickPercent, float updateRate) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // Update current values
    m_currentSpeed = speed;
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
        }
        break;
        
    case WM_HSCROLL:
        if ((HWND)lParam == m_sensitivitySlider) {
            int pos = SendMessage(m_sensitivitySlider, TBM_GETPOS, 0, 0);
            float sensitivity = pos / 100.0f;
            
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << sensitivity;
            SetWindowText(m_sensitivityLabel, ss.str().c_str());
        }
        return 0;
        
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
    
    const auto& config = m_configManager->GetConfig();
    
    // Set sensitivity slider
    SendMessage(m_sensitivitySlider, TBM_SETPOS, TRUE, static_cast<LPARAM>(config.sensitivity * 100));
    
    // Set update rate combo
    int rateIndex = 1;  // Default 50Hz
    if (config.updateIntervalMs >= 33) rateIndex = 0;      // 30Hz
    else if (config.updateIntervalMs >= 20) rateIndex = 1; // 50Hz
    else if (config.updateIntervalMs >= 16) rateIndex = 2; // 60Hz
    else rateIndex = 3;                                    // 100Hz
    SendMessage(m_updateRateCombo, CB_SETCURSEL, rateIndex, 0);
    
    // Set checkboxes
    SendMessage(m_invertYCheck, BM_SETCHECK, config.invertY ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(m_lockXCheck, BM_SETCHECK, config.lockX ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(m_adaptiveModeCheck, BM_SETCHECK, config.adaptiveMode ? BST_CHECKED : BST_UNCHECKED, 0);
}

void MainWindow::ApplySettings() {
    if (!m_configManager || !m_processor) return;
    
    auto& config = m_configManager->GetConfig();
    
    // Get sensitivity
    int pos = SendMessage(m_sensitivitySlider, TBM_GETPOS, 0, 0);
    config.sensitivity = pos / 100.0f;
    
    // Get update rate
    int rateIndex = SendMessage(m_updateRateCombo, CB_GETCURSEL, 0, 0);
    switch (rateIndex) {
    case 0: config.updateIntervalMs = 33; break;  // 30Hz
    case 1: config.updateIntervalMs = 20; break;  // 50Hz
    case 2: config.updateIntervalMs = 16; break;  // 60Hz
    case 3: config.updateIntervalMs = 10; break;  // 100Hz
    }
    
    // Get checkboxes
    config.invertY = SendMessage(m_invertYCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    config.lockX = SendMessage(m_lockXCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    config.adaptiveMode = SendMessage(m_adaptiveModeCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    
    // Apply to processor
    m_processor->SetConfig(config.toProcessingConfig());
    
    // Save to file
    m_configManager->Save();
    
    MessageBox(m_hwnd, "Settings applied and saved!", "Success", MB_OK | MB_ICONINFORMATION);
}

} // namespace Mouse2VR