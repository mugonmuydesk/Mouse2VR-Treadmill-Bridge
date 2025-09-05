#include "WebViewWindow.h"
#include "Bridge.h"
#include "core/Mouse2VRCore.h"
#include "common/Logger.h"
#include "core/PathUtils.h"
#include <sstream>
#include <filesystem>
#include <Shlwapi.h>

// Development mode: Load UI directly from source files
// Production mode: Load UI from resources folder
// Uncomment the following line for development
// #define DEV_UI

using namespace Microsoft::WRL;

// Utility to build a path to the bundled runtime
std::wstring GetWebView2FixedRuntimePath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    // Remove filename
    PathRemoveFileSpecW(exePath);

    // Append "WebView2Runtime" (relative folder copied by CMake)
    wchar_t runtimePath[MAX_PATH];
    PathCombineW(runtimePath, exePath, L"WebView2Runtime");

    return std::wstring(runtimePath);
}

// Check if Fixed Runtime is available
bool IsWebView2FixedRuntimeAvailable() {
    std::wstring runtimePath = GetWebView2FixedRuntimePath();
    
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

WebViewWindow::WebViewWindow() 
    : m_parentWindow(nullptr)
    , m_core(nullptr) {
}

WebViewWindow::~WebViewWindow() {
    if (m_controller) {
        m_controller->Close();
    }
}

bool WebViewWindow::Initialize(HWND parentWindow, Mouse2VR::Mouse2VRCore* core) {
    m_parentWindow = parentWindow;
    m_core = core;
    
    LOG_INFO("WebView", "Initializing WebView2...");
    
    // Create WebView2 environment
    HRESULT hr = CreateWebView2Environment();
    if (FAILED(hr)) {
        LOG_ERROR("WebView", "Failed to create WebView2 environment: " + std::to_string(hr));
        return false;
    }
    
    return true;
}

HRESULT WebViewWindow::CreateWebView2Environment() {
    // Determine which runtime to use
    std::wstring browserExecutableFolder;
    const wchar_t* browserPath = nullptr;
    
    if (IsWebView2FixedRuntimeAvailable()) {
        // Use Fixed Runtime (preferred)
        browserExecutableFolder = GetWebView2FixedRuntimePath();
        browserPath = browserExecutableFolder.c_str();
        LOG_INFO("WebView", "Using WebView2 Fixed Runtime at: " + 
                 std::string(browserExecutableFolder.begin(), browserExecutableFolder.end()));
    } else {
        // Fallback to Evergreen Runtime (system-installed)
        browserPath = nullptr;
        LOG_INFO("WebView", "Using WebView2 Evergreen Runtime (system-installed)");
    }
    
    // Create WebView2 environment
    return CreateCoreWebView2EnvironmentWithOptions(
        browserPath,  // Use Fixed Runtime if available, otherwise default
        nullptr,      // Use default user data folder
        nullptr,      // No additional options
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                return OnCreateEnvironmentCompleted(result, environment);
            }
        ).Get()
    );
}

HRESULT WebViewWindow::OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment) {
    if (FAILED(result)) {
        LOG_ERROR("WebView", "Failed to create environment: " + std::to_string(result));
        return result;
    }
    
    m_environment = environment;
    
    // Create WebView2 controller
    m_environment->CreateCoreWebView2Controller(
        m_parentWindow,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                return OnCreateWebViewControllerCompleted(result, controller);
            }
        ).Get()
    );
    
    return S_OK;
}

HRESULT WebViewWindow::OnCreateWebViewControllerCompleted(HRESULT result, ICoreWebView2Controller* controller) {
    if (FAILED(result)) {
        LOG_ERROR("WebView", "Failed to create controller: " + std::to_string(result));
        return result;
    }
    
    m_controller = controller;
    m_controller->get_CoreWebView2(&m_webView);
    
    // Configure WebView settings
    wil::com_ptr<ICoreWebView2Settings> settings;
    m_webView->get_Settings(&settings);
    settings->put_IsScriptEnabled(TRUE);
    settings->put_AreDefaultScriptDialogsEnabled(TRUE);
    settings->put_IsWebMessageEnabled(TRUE);
    settings->put_AreDevToolsEnabled(TRUE);  // Enable DevTools for debugging
    
    // Resize WebView to fit parent window (use full client area)
    RECT bounds;
    GetClientRect(m_parentWindow, &bounds);
    Resize(bounds);
    
    // Register event handlers
    RegisterEventHandlers();
    
    // Setup JavaScript bridge
    SetupJavaScriptBridge();
    
    // Load UI from external files
    LoadUIFromFiles();
    
    // Force window title (in case WebView2 changes it)
    SetWindowTextW(m_parentWindow, L"Mouse2VR Treadmill Bridge");
    
    LOG_INFO("WebView", "WebView2 initialized successfully");
    return S_OK;
}

void WebViewWindow::Resize(RECT bounds) {
    if (m_controller) {
        m_controller->put_Bounds(bounds);
    }
}

HRESULT WebViewWindow::ExecuteScript(const std::wstring& script) {
    if (!m_webView) {
        return E_FAIL;
    }
    
    return m_webView->ExecuteScript(
        script.c_str(),
        Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
            [](HRESULT error, LPCWSTR result) -> HRESULT {
                if (FAILED(error)) {
                    LOG_ERROR("WebView", "Script execution failed");
                }
                return S_OK;
            }
        ).Get()
    );
}

void WebViewWindow::NavigateToFile(const std::wstring& htmlPath) {
    if (m_webView) {
        m_webView->Navigate(htmlPath.c_str());
    }
}

void WebViewWindow::NavigateToString(const std::wstring& htmlContent) {
    if (m_webView) {
        m_webView->NavigateToString(htmlContent.c_str());
    }
}

void WebViewWindow::LoadUIFromFiles() {
    std::wstring htmlPath;
    
#ifdef DEV_UI
    // Development mode: Load from source directory
    // Get the project root directory (assumes standard build structure)
    wchar_t currentDir[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentDir);
    
    // Try to find src/webview/ui relative to current directory
    std::filesystem::path devPath = std::filesystem::path(currentDir);
    
    // Go up directories until we find src/webview/ui
    for (int i = 0; i < 5; i++) {
        std::filesystem::path uiPath = devPath / L"src" / L"webview" / L"ui" / L"index.html";
        if (std::filesystem::exists(uiPath)) {
            htmlPath = Mouse2VR::PathUtils::PathToFileURL(uiPath.wstring());
            LOG_INFO("WebView", "DEV_UI: Loading from source: " + std::string(htmlPath.begin(), htmlPath.end()));
            break;
        }
        devPath = devPath.parent_path();
    }
    
    if (htmlPath.empty()) {
        LOG_ERROR("WebView", "DEV_UI: Could not find src/webview/ui/index.html");
        // Fall back to production mode
    }
#endif
    
    if (htmlPath.empty()) {
        // Production mode: Load from resources folder relative to executable
        std::wstring indexPath = Mouse2VR::PathUtils::GetExecutablePathW(L"resources\\ui\\index.html");
        
        // Check if the file exists
        if (!std::filesystem::exists(indexPath)) {
            LOG_ERROR("WebView", "UI file not found: " + std::string(indexPath.begin(), indexPath.end()));
            // Fallback: try to load from a fallback embedded HTML if needed
            NavigateToString(GetFallbackHTML());
            return;
        }
        
        htmlPath = Mouse2VR::PathUtils::PathToFileURL(indexPath);
        LOG_INFO("WebView", "Loading UI from: " + std::string(htmlPath.begin(), htmlPath.end()));
    }
    
    // Navigate to the file URL
    NavigateToFile(htmlPath);
}

void WebViewWindow::RegisterEventHandlers() {
    // Handle navigation completed
    m_webView->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                BOOL success;
                args->get_IsSuccess(&success);
                
                if (success) {
                    LOG_INFO("WebView", "Navigation completed successfully");
                    // Force window title again after navigation
                    SetWindowTextW(m_parentWindow, L"Mouse2VR Treadmill Bridge");
                    InjectInitialScript();
                    if (m_onDocumentReady) {
                        m_onDocumentReady();
                    }
                } else {
                    LOG_ERROR("WebView", "Navigation failed");
                }
                
                return S_OK;
            }
        ).Get(),
        nullptr
    );
    
    // Handle messages from JavaScript
    m_webView->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                wil::unique_cotaskmem_string message;
                args->TryGetWebMessageAsString(&message);
                
                // Parse and handle message
                std::wstring msg(message.get());
                LOG_DEBUG("WebView", "Received message from JS: " + std::string(msg.begin(), msg.end()));
                
                // Handle different message types
                if (msg.find(L"setSensitivity:") == 0) {
                    double value = std::stod(msg.substr(15));
                    m_core->SetSensitivity(value);
                    LOG_INFO("WebView", "Set sensitivity to: " + std::to_string(value));
                } else if (msg.find(L"setUpdateRate:") == 0) {
                    int value = std::stoi(msg.substr(14));
                    m_core->SetUpdateRate(value);
                    LOG_INFO("WebView", "Set update rate to: " + std::to_string(value) + " Hz");
                } else if (msg.find(L"setInvertY:") == 0) {
                    bool value = (msg.substr(11) == L"true");
                    m_core->SetInvertY(value);
                    LOG_INFO("WebView", "Set invert Y to: " + std::string(value ? "true" : "false"));
                } else if (msg.find(L"setLockX:") == 0) {
                    bool value = (msg.substr(9) == L"true");
                    m_core->SetLockX(value);
                    LOG_INFO("WebView", "Set lock X to: " + std::string(value ? "true" : "false"));
                } else if (msg.find(L"setDPI:") == 0) {
                    int dpi = std::stoi(msg.substr(7));
                    // Calculate counts per meter: DPI * 39.3701 (inches per meter)
                    float countsPerMeter = dpi * 39.3701f;
                    m_core->SetCountsPerMeter(countsPerMeter);
                    LOG_INFO("WebView", "Set DPI to: " + std::to_string(dpi) + " (counts/meter: " + std::to_string(countsPerMeter) + ")");
                } else if (msg == L"startTest") {
                    m_core->StartMovementTest();
                    LOG_INFO("WebView", "Started 5-second movement test");
                } else if (msg == L"getStatus") {
                    bool isRunning = m_core->IsRunning();
                    ExecuteScript(L"updateStatus(" + std::wstring(isRunning ? L"true" : L"false") + L")");
                } else if (msg == L"getSpeed") {
                    auto state = m_core->GetCurrentState();
                    // Treadmill speed is the raw physical speed (m/s)
                    double treadmillSpeed = state.stickY >= 0 ? state.speed : -state.speed;
                    // Game speed based on stick deflection and HL2 max sprint speed (6.1 m/s)
                    double gameSpeed = state.stickY * 6.1; // HL2 max sprint speed at full deflection
                    // Get actual update rate from backend
                    int actualHz = m_core->GetActualUpdateRate();
                    // Send treadmill speed, game speed, stick Y position, and actual update rate
                    std::wstring speedUpdate = L"updateSpeed(" + std::to_wstring(treadmillSpeed) + 
                                              L", " + std::to_wstring(gameSpeed) +
                                              L", " + std::to_wstring(state.stickY) +
                                              L", " + std::to_wstring(actualHz) + L")";
                    ExecuteScript(speedUpdate);
                } else if (msg == L"start") {
                    m_core->Start();
                    LOG_INFO("WebView", "Started Mouse2VR core");
                    // Update UI status
                    ExecuteScript(L"updateStatus(true)");
                } else if (msg == L"stop") {
                    m_core->Stop();
                    LOG_INFO("WebView", "Stopped Mouse2VR core");
                    // Update UI status
                    ExecuteScript(L"updateStatus(false)");
                } else if (msg == L"getConfig") {
                    // Get current configuration from core
                    auto procConfig = m_core->GetProcessorConfig();
                    int targetHz = m_core->GetTargetUpdateRate();
                    bool isRunning = m_core->IsRunning();
                    
                    // Build config JSON string
                    std::wstring configJson = L"{"
                        L"\"dpi\":" + std::to_wstring(procConfig.dpi) + L","
                        L"\"sensitivity\":" + std::to_wstring(procConfig.sensitivity) + L","
                        L"\"updateRateHz\":" + std::to_wstring(targetHz) + L","
                        L"\"uiRateHz\":5," // Default UI rate, TODO: store this properly
                        L"\"invertY\":" + (procConfig.invertY ? L"true" : L"false") + L","
                        L"\"lockX\":" + (procConfig.lockX ? L"true" : L"false") + L","
                        L"\"runEnabled\":" + (isRunning ? L"true" : L"false") +
                        L"}";
                    
                    // Send config to JavaScript
                    ExecuteScript(L"if(window.applyConfigToUI) applyConfigToUI(" + configJson + L")");
                    LOG_INFO("WebView", "Sent config to UI");
                }
                
                return S_OK;
            }
        ).Get(),
        nullptr
    );
}

void WebViewWindow::InjectInitialScript() {
    // Inject JavaScript API for communication
    std::wstring script = LR"JS(
        window.mouse2vr = {
            setSensitivity: function(value) {
                window.chrome.webview.postMessage('setSensitivity:' + value);
            },
            setUpdateRate: function(value) {
                window.chrome.webview.postMessage('setUpdateRate:' + value);
            },
            setInvertY: function(value) {
                window.chrome.webview.postMessage('setInvertY:' + value);
            },
            setLockX: function(value) {
                window.chrome.webview.postMessage('setLockX:' + value);
            },
            setDPI: function(value) {
                window.chrome.webview.postMessage('setDPI:' + value);
            },
            startTest: function() {
                window.chrome.webview.postMessage('startTest');
            },
            getStatus: function() {
                window.chrome.webview.postMessage('getStatus');
            },
            start: function() {
                window.chrome.webview.postMessage('start');
            },
            stop: function() {
                window.chrome.webview.postMessage('stop');
            },
            getSpeed: function() {
                window.chrome.webview.postMessage('getSpeed');
            },
            getConfig: function() {
                window.chrome.webview.postMessage('getConfig');
            }
        };
        
        console.log('Mouse2VR API injected');
    )JS";
    
    ExecuteScript(script);
}

void WebViewWindow::SetupJavaScriptBridge() {
    // This will be expanded with the Bridge class implementation
    // For now, we use simple message passing
}

std::wstring WebViewWindow::GetFallbackHTML() {
    // Minimal fallback HTML if external resources are not found
    return LR"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Mouse2VR - Error</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', system-ui, sans-serif;
            background: #1e1e1e;
            color: #ffffff;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
        }
        .error {
            text-align: center;
            padding: 2rem;
        }
        h1 { color: #f44336; }
        p { color: #888; }
    </style>
</head>
<body>
    <div class="error">
        <h1>UI Resources Not Found</h1>
        <p>Could not load the user interface files.</p>
        <p>Please ensure the 'resources/ui' folder exists in the application directory.</p>
        <p>Path checked: resources\ui\index.html</p>
    </div>
</body>
</html>
)HTML";
}