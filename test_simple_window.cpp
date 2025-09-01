#include <windows.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Show immediate message
    MessageBox(nullptr, "Test program starting", "Debug", MB_OK);
    
    // Register class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "TestWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    if (!RegisterClass(&wc)) {
        MessageBox(nullptr, "Failed to register class", "Error", MB_OK);
        return 1;
    }
    
    // Create window
    HWND hwnd = CreateWindow("TestWindow", "Test", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, hInstance, nullptr);
    
    if (!hwnd) {
        MessageBox(nullptr, "Failed to create window", "Error", MB_OK);
        return 1;
    }
    
    ShowWindow(hwnd, SW_SHOW);
    MessageBox(nullptr, "Window created successfully!", "Success", MB_OK);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}