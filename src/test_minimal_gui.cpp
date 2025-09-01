// Minimal test to isolate GUI crash
#include <windows.h>
#include <cstdio>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Test 1: Can we even show a message box?
    MessageBox(nullptr, "Test 1: Message box works", "Debug", MB_OK);
    
    // Test 2: Can we register a window class?
    WNDCLASS wc = {};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "TestClass";
    
    if (!RegisterClass(&wc)) {
        MessageBox(nullptr, "Failed to register class", "Error", MB_OK);
        return 1;
    }
    
    MessageBox(nullptr, "Test 2: Window class registered", "Debug", MB_OK);
    
    // Test 3: Can we create a window?
    HWND hwnd = CreateWindow("TestClass", "Test", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        nullptr, nullptr, hInstance, nullptr);
    
    if (!hwnd) {
        MessageBox(nullptr, "Failed to create window", "Error", MB_OK);
        return 1;
    }
    
    ShowWindow(hwnd, SW_SHOW);
    MessageBox(nullptr, "Test 3: Window created and shown!", "Success", MB_OK);
    
    // Don't run message loop, just exit
    return 0;
}