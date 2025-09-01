#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>
#include <atomic>
#include <memory>
#include <deque>
#include <mutex>

namespace Mouse2VR {

// Forward declarations
class RawInputHandler;
class ViGEmController;
class InputProcessor;
class ConfigManager;
struct MouseDelta;

class MainWindow {
public:
    MainWindow();
    ~MainWindow();
    
    // Initialize and create window
    bool Initialize(HINSTANCE hInstance);
    
    // Run message loop
    int Run();
    
    // Update display with new data
    void UpdateStatus(const MouseDelta& delta, float speed, float stickPercent, float updateRate);
    
    // Set components for interaction
    void SetComponents(RawInputHandler* input, ViGEmController* controller, 
                      InputProcessor* processor, ConfigManager* config);
    
    // Show/hide window
    void Show();
    void Hide();
    void ToggleVisibility();
    
    // Check if should exit
    bool ShouldExit() const { return m_shouldExit; }
    
private:
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // UI Creation
    void CreateControls();
    void CreateTrayIcon();
    void RemoveTrayIcon();
    
    // UI Updates
    void UpdateInputGraph();
    void DrawStickPosition(HDC hdc, RECT rect);
    void DrawInputGraph(HDC hdc, RECT rect);
    
    // Settings
    void ApplySettings();
    void LoadSettings();
    void SaveSettings();
    
    // Window state
    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    static MainWindow* s_instance;
    std::atomic<bool> m_shouldExit{false};
    bool m_visible = true;
    
    // Tray icon
    static constexpr UINT WM_TRAYICON = WM_USER + 1;
    static constexpr UINT ID_TRAYICON = 1;
    
    // Controls
    HWND m_statusText = nullptr;
    HWND m_sensitivitySlider = nullptr;
    HWND m_sensitivityLabel = nullptr;
    HWND m_updateRateCombo = nullptr;
    HWND m_invertYCheck = nullptr;
    HWND m_lockXCheck = nullptr;
    HWND m_adaptiveModeCheck = nullptr;
    HWND m_applyButton = nullptr;
    HWND m_graphArea = nullptr;
    HWND m_stickArea = nullptr;
    
    // Components
    RawInputHandler* m_inputHandler = nullptr;
    ViGEmController* m_controller = nullptr;
    InputProcessor* m_processor = nullptr;
    ConfigManager* m_configManager = nullptr;
    
    // Data for visualization
    struct DataPoint {
        float deltaY;
        float speed;
        float stickPercent;
    };
    
    std::deque<DataPoint> m_dataHistory;
    std::mutex m_dataMutex;
    static constexpr size_t MAX_HISTORY = 100;
    
    // Current values
    float m_currentSpeed = 0.0f;
    float m_currentStickX = 0.0f;
    float m_currentStickY = 0.0f;
    float m_currentUpdateRate = 0.0f;
};

} // namespace Mouse2VR