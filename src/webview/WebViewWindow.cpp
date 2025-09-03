#include "WebViewWindow.h"
#include "Bridge.h"
#include "core/Mouse2VRCore.h"
#include "common/Logger.h"
#include <sstream>
#include <filesystem>
#include <Shlwapi.h>

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
    
    // Load the embedded HTML
    NavigateToString(GetEmbeddedHTML());
    
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

void WebViewWindow::RegisterEventHandlers() {
    // Handle navigation completed
    m_webView->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                BOOL success;
                args->get_IsSuccess(&success);
                
                if (success) {
                    LOG_INFO("WebView", "Navigation completed successfully");
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
                } else if (msg == L"getStatus") {
                    bool isRunning = m_core->IsRunning();
                    ExecuteScript(L"updateStatus(" + std::wstring(isRunning ? L"true" : L"false") + L")");
                } else if (msg == L"getSpeed") {
                    auto state = m_core->GetCurrentState();
                    // Calculate signed speed (positive = forward, negative = backward)
                    double signedSpeed = state.stickY >= 0 ? state.speed : -state.speed;
                    // Game speed is stick deflection * max game speed (preserving direction)
                    // This shows the effective movement speed in game after sensitivity
                    double gameSpeed = state.stickY * 3.0; // Scale to max 3 m/s at full stick deflection
                    // Get actual update rate from backend
                    int actualHz = m_core->GetActualUpdateRate();
                    // Send speed, game speed, stick Y position, and actual update rate
                    std::wstring speedUpdate = L"updateSpeed(" + std::to_wstring(signedSpeed) + 
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

std::wstring WebViewWindow::GetEmbeddedHTML() {
    // Build HTML in parts to avoid compiler string size limits
    std::wstring html;
    
    // Part 1: DOCTYPE and head start with Fluent Design
    html += LR"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Mouse2VR Treadmill Bridge</title>
    <style>
        body {
            font-family: "Segoe UI Variable Text", "Segoe UI", sans-serif;
            margin: 0;
            background: #f3f3f3;
            color: #1a1a1a;
            font-size: 14px;
            line-height: 20px;
        }
        
        .app-container {
            max-width: 800px;
            margin: 40px auto;
            padding: 24px;
            background: #ededed;
            border-radius: 8px;
            box-shadow: 0 4px 12px rgba(0,0,0,0.08);
        }
        
        h1 {
            font-size: 28px;
            line-height: 36px;
            margin-bottom: 24px;
            font-weight: 400;
            color: #1a1a1a;
        }
        
        .section-label {
            font-size: 14px;
            font-weight: 500;
            color: #616161;
            margin: 16px 0 8px 4px;
        }
        
        .status-card, .chart-card, .setting-card {
            background: #ffffff;
            border-radius: 8px;
            box-shadow: 0 2px 6px rgba(0,0,0,0.06);
            padding: 12px 16px;
            margin-bottom: 12px;
        }
        
        .status-grid {
            display: flex;
            flex-direction: column;
            gap: 12px;
        }
        
        .status-metric {
            display: flex;
            flex-direction: column;
        }
        
        .status-label {
            font-size: 12px;
            line-height: 16px;
            color: #616161;
            margin-bottom: 2px;
        }
        
        .status-value {
            font-size: 20px;
            line-height: 24px;
            font-weight: 400;
            color: #1a1a1a;
        }
        
        .side-by-side {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 16px;
            margin-bottom: 24px;
            align-items: stretch;
        }
        
        .chart-card {
            display: flex;
            flex-direction: column;
        }
        
        .chart-description {
            font-size: 13px;
            line-height: 18px;
            color: #616161;
            margin-bottom: 12px;
        }
        
        .chart-placeholder {
            flex-grow: 1;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 13px;
            color: #616161;
            border: 1px dashed #ccc;
            border-radius: 6px;
            min-height: 120px;
        }
        
        canvas {
            width: 100% !important;
            height: 100% !important;
        }
        
        .setting-row {
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 8px 0;
        }
        
        .setting-label {
            font-size: 14px;
            line-height: 20px;
            font-weight: 500;
        }
        
        input[type="range"] {
            width: 160px;
        }
        
        .radio-group, .checkbox-group {
            display: flex;
            gap: 12px;
            font-size: 14px;
            color: #1a1a1a;
            align-items: center;
        }
        
        .radio-group label, .checkbox-group label {
            display: flex;
            align-items: center;
            gap: 4px;
        }
        
        input[type="checkbox"], input[type="radio"] {
            accent-color: #0078d4;
            width: 16px;
            height: 16px;
            cursor: pointer;
        }
        
        input[type="radio"] {
            border-radius: 50%;
        }
        
        button {
            background: #0078d4;
            border: none;
            color: #ffffff;
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: background 0.2s ease;
        }
        
        button:hover {
            background: #0067c0;
        }
    </style>
</head>)HTML";
    
    // Part 2: Body content with Fluent Design layout
    html += LR"HTML(<body>
    <div class="app-container">
        <h1>Mouse2VR Treadmill Bridge</h1>
        
        <div class="side-by-side">
            <div>
                <div class="section-label">Performance Metrics</div>
                <div class="status-card">
                    <div class="status-grid">
                        <div class="status-metric">
                            <span class="status-label">Status</span>
                            <span class="status-value" id="statusValue">Stopped</span>
                        </div>
                        <div class="status-metric">
                            <span class="status-label">Speed</span>
                            <span class="status-value" id="speedValue">0.00 m/s</span>
                        </div>
                        <div class="status-metric">
                            <span class="status-label">Stick</span>
                            <span class="status-value" id="stickValue">0%</span>
                        </div>
                        <div class="status-metric">
                            <span class="status-label">Update Rate</span>
                            <span class="status-value" id="updateRateValue">0 Hz</span>
                        </div>
                    </div>
                </div>
            </div>
            
            <div>
                <div class="section-label">Controller Stick Position</div>
                <div class="chart-card">
                    <div class="chart-description">Visualises treadmill movement as left stick deflection.</div>
                    <div class="chart-placeholder">
                        <canvas id="stickCanvas"></canvas>
                    </div>
                </div>
            </div>
        </div>
        
        <div class="section-label">Walking Speed Over Time</div>
        <div class="chart-card">
            <div class="chart-description">Shows treadmill belt speed converted into meters per second.</div>
            <div class="chart-placeholder">
                <canvas id="speedCanvas"></canvas>
            </div>
        </div>
        
        <div class="section-label">Settings</div>
        
        <div class="setting-card">
            <div class="setting-row">
                <span class="setting-label">Sensitivity</span>
                <input type="range" id="sensitivity" min="0.1" max="3.0" step="0.1" value="1.0" onchange="updateSensitivity(this.value)" />
            </div>
        </div>
        
        <div class="setting-card">
            <div class="setting-row">
                <span class="setting-label">Update Rate</span>
                <div class="radio-group">
                    <label><input type="radio" name="rate" value="30" onchange="updateRate(30)"> 30 Hz</label>
                    <label><input type="radio" name="rate" value="50" onchange="updateRate(50)"> 50 Hz</label>
                    <label><input type="radio" name="rate" value="60" checked onchange="updateRate(60)"> 60 Hz</label>
                    <label><input type="radio" name="rate" value="100" onchange="updateRate(100)"> 100 Hz</label>
                </div>
            </div>
        </div>
        
        <div class="setting-card">
            <div class="setting-row">
                <span class="setting-label">Axis Options</span>
                <div class="checkbox-group">
                    <label><input type="checkbox" id="invertY" onchange="updateAxisOptions()"> Invert Y Axis</label>
                    <label><input type="checkbox" id="lockX" checked onchange="updateAxisOptions()"> Lock X Axis</label>
                    <label><input type="checkbox" id="adaptive" onchange="updateAxisOptions()"> Adaptive Mode</label>
                </div>
            </div>
        </div>
        
        <div class="setting-row" style="justify-content: center; margin-top: 16px;">
            <button onclick="toggleRunning()" id="startStopBtn">Start</button>
        </div>
    </div>)HTML";
    
    // Part 3: JavaScript with Fluent Design interactions
    html += LR"HTML(
    <script>
        let isRunning = false;
        let treadmillSpeedHistory = [];
        let gameSpeedHistory = [];
        let lastUpdateTime = Date.now();
        
        // Initialize canvases when page loads
        window.addEventListener('DOMContentLoaded', () => {
            initializeStickCanvas();
            initializeSpeedCanvas();
        });
        
        function toggleRunning() {
            isRunning = !isRunning;
            const btn = document.getElementById('startStopBtn');
            
            if (isRunning) {
                window.mouse2vr.start();
                btn.textContent = 'Stop';
                document.getElementById('statusValue').textContent = 'Running';
                document.getElementById('statusValue').style.color = '#0f7938';
            } else {
                window.mouse2vr.stop();
                btn.textContent = 'Start';
                document.getElementById('statusValue').textContent = 'Stopped';
                document.getElementById('statusValue').style.color = '#c42b1c';
            }
        }
        
        function updateSensitivity(value) {
            if (window.mouse2vr) {
                window.mouse2vr.setSensitivity(parseFloat(value));
            }
        }
        
        function updateRate(value) {
            document.getElementById('updateRateValue').textContent = value + ' Hz';
            if (window.mouse2vr) {
                window.mouse2vr.setUpdateRate(value);
            }
            // Update polling rate to match
            startPolling(value);
        }
        
        function updateAxisOptions() {
            const invertY = document.getElementById('invertY').checked;
            const lockX = document.getElementById('lockX').checked;
            const adaptive = document.getElementById('adaptive').checked;
            
            // Send settings to backend
            if (window.mouse2vr) {
                window.mouse2vr.setInvertY(invertY);
                window.mouse2vr.setLockX(lockX);
                // Adaptive mode not yet implemented in backend
                console.log('Adaptive mode:', adaptive);
            }
        }
        
        // Update speed and stick displays
        function updateSpeed(treadmillSpeed, gameSpeed, stickY, actualHz) {
            // Update speed value (show absolute for display)
            document.getElementById('speedValue').textContent = Math.abs(treadmillSpeed).toFixed(2) + ' m/s';
            
            // Use actual stick Y value if provided
            let stickPercent = 0;
            if (stickY !== undefined) {
                stickPercent = Math.abs(stickY) * 100;
            }
            document.getElementById('stickValue').textContent = Math.round(stickPercent) + '%';
            
            // Use actual Hz from backend if provided, otherwise calculate from JS
            if (actualHz !== undefined && actualHz > 0) {
                document.getElementById('updateRateValue').textContent = actualHz + ' Hz';
            } else {
                // Fallback to calculating from JS timing
                const now = Date.now();
                const hz = 1000 / (now - lastUpdateTime);
                lastUpdateTime = now;
                document.getElementById('updateRateValue').textContent = Math.round(hz) + ' Hz';
            }
            
            // Update visualizations with actual stick position
            if (stickY !== undefined) {
                updateStickVisualization(stickY);
            }
            
            // Add both speeds to history (with sign for direction)
            addSpeedToHistory(treadmillSpeed, gameSpeed);
        }
        
        function initializeStickCanvas() {
            const canvas = document.getElementById('stickCanvas');
            if (!canvas) return;
            
            const ctx = canvas.getContext('2d');
            canvas.width = canvas.offsetWidth;
            canvas.height = canvas.offsetHeight;
            
            drawStickBase(ctx, canvas.width, canvas.height);
        }
        
        function drawStickBase(ctx, w, h) {
            // Draw circle for stick area
            ctx.strokeStyle = '#616161';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.arc(w/2, h/2, Math.min(w, h) * 0.4, 0, Math.PI * 2);
            ctx.stroke();
            
            // Draw center dot
            ctx.fillStyle = '#0078d4';
            ctx.beginPath();
            ctx.arc(w/2, h/2, 5, 0, Math.PI * 2);
            ctx.fill();
        }
        
        function updateStickVisualization(value) {
            const canvas = document.getElementById('stickCanvas');
            if (!canvas) return;
            
            const ctx = canvas.getContext('2d');
            const w = canvas.width;
            const h = canvas.height;
            
            // Clear and redraw base
            ctx.clearRect(0, 0, w, h);
            drawStickBase(ctx, w, h);
            
            // Draw stick position (value ranges from -1 to 1)
            const radius = Math.min(w, h) * 0.4;
            const y = h/2 - (value * radius); // Negative = down, positive = up
            
            ctx.fillStyle = '#0078d4';
            ctx.beginPath();
            ctx.arc(w/2, y, 10, 0, Math.PI * 2);
            ctx.fill();
        }
        )HTML";
        
    // Part 4: Speed graph functions
    html += LR"HTML(
        function initializeSpeedCanvas() {
            const canvas = document.getElementById('speedCanvas');
            if (!canvas) return;
            
            canvas.width = canvas.offsetWidth;
            canvas.height = canvas.offsetHeight;
            
            // Initialize with empty history
            for (let i = 0; i < 50; i++) {
                treadmillSpeedHistory.push(0);
                gameSpeedHistory.push(0);
            }
        }
        
        function addSpeedToHistory(treadmillSpeed, gameSpeed) {
            treadmillSpeedHistory.push(treadmillSpeed);
            gameSpeedHistory.push(gameSpeed);
            
            if (treadmillSpeedHistory.length > 50) {
                treadmillSpeedHistory.shift();
            }
            if (gameSpeedHistory.length > 50) {
                gameSpeedHistory.shift();
            }
            
            drawSpeedGraph();
        }
        
        function drawSpeedGraph() {
            const canvas = document.getElementById('speedCanvas');
            if (!canvas) return;
            
            const ctx = canvas.getContext('2d');
            const w = canvas.width;
            const h = canvas.height;
            
            ctx.clearRect(0, 0, w, h);
            
            // Draw center line (zero speed)
            ctx.strokeStyle = '#d2d2d2';
            ctx.lineWidth = 1;
            ctx.setLineDash([2, 2]);
            ctx.beginPath();
            ctx.moveTo(0, h/2);
            ctx.lineTo(w, h/2);
            ctx.stroke();
            ctx.setLineDash([]);
            
            // Draw treadmill speed (physical)
            ctx.strokeStyle = '#0078d4'; // Microsoft Blue
            ctx.lineWidth = 2;
            ctx.beginPath();
            
            treadmillSpeedHistory.forEach((speed, i) => {
                const x = (i / (treadmillSpeedHistory.length - 1)) * w;
                const y = h/2 - ((speed / 3) * (h/2)); // Scale: 3 m/s = full height
                
                if (i === 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }
            });
            ctx.stroke();
            
            // Draw game speed (after sensitivity)
            ctx.strokeStyle = '#10893e'; // Microsoft Green
            ctx.lineWidth = 2;
            ctx.globalAlpha = 0.8; // Slight transparency
            ctx.beginPath();
            
            gameSpeedHistory.forEach((speed, i) => {
                const x = (i / (gameSpeedHistory.length - 1)) * w;
                const y = h/2 - ((speed / 3) * (h/2)); // Same scale
                
                if (i === 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }
            });
            ctx.stroke();
            ctx.globalAlpha = 1.0;
            
            // Draw legend (Fluent Design style)
            const legendX = w - 140;
            const legendY = 10;
            
            // Semi-transparent background
            ctx.fillStyle = 'rgba(255, 255, 255, 0.9)';
            ctx.fillRect(legendX - 5, legendY - 5, 135, 45);
            
            // Legend items
            ctx.font = '12px "Segoe UI", sans-serif';
            
            // Treadmill speed
            ctx.strokeStyle = '#0078d4';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(legendX, legendY + 8);
            ctx.lineTo(legendX + 20, legendY + 8);
            ctx.stroke();
            
            ctx.fillStyle = '#1a1a1a';
            ctx.fillText('Treadmill', legendX + 25, legendY + 12);
            
            // Game speed
            ctx.strokeStyle = '#10893e';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(legendX, legendY + 25);
            ctx.lineTo(legendX + 20, legendY + 25);
            ctx.stroke();
            
            ctx.fillStyle = '#1a1a1a';
            ctx.fillText('Game (× sensitivity)', legendX + 25, legendY + 29);
        }
        
        // Request speed updates periodically
        let updateInterval = null;
        
        function startPolling(rateHz = 60) {
            // Clear existing interval
            if (updateInterval) {
                clearInterval(updateInterval);
            }
            
            // Calculate interval from Hz (with minimum of 10ms)
            const intervalMs = Math.max(10, Math.floor(1000 / rateHz));
            
            updateInterval = setInterval(() => {
                if (window.mouse2vr && window.mouse2vr.getSpeed) {
                    window.mouse2vr.getSpeed();
                }
            }, intervalMs);
        }
        
        // Start with default 60Hz
        startPolling(60);
        
        // Initialize
        window.addEventListener('DOMContentLoaded', () => {
            console.log('Mouse2VR UI loaded');
            if (window.mouse2vr) {
                window.mouse2vr.getStatus();
            }
        });
    </script>
</body>
</html>)HTML";
    
    return html;
}