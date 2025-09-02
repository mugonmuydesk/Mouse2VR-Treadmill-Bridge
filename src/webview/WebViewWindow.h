#pragma once
#include <windows.h>
#include <wrl.h>
#include <wil/com.h>
#include <WebView2.h>
#include <string>
#include <functional>

// Forward declarations
namespace Mouse2VR {
    class Mouse2VRCore;
}

class WebViewWindow {
public:
    WebViewWindow();
    ~WebViewWindow();
    
    bool Initialize(HWND parentWindow, Mouse2VR::Mouse2VRCore* core);
    void Resize(RECT bounds);
    HRESULT ExecuteScript(const std::wstring& script);
    void NavigateToFile(const std::wstring& htmlPath);
    void NavigateToString(const std::wstring& htmlContent);
    
    // Event handlers
    void OnDocumentReady(std::function<void()> callback) { m_onDocumentReady = callback; }
    
private:
    HWND m_parentWindow;
    Mouse2VR::Mouse2VRCore* m_core;
    
    wil::com_ptr<ICoreWebView2Controller> m_controller;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2Environment> m_environment;
    
    std::function<void()> m_onDocumentReady;
    
    HRESULT CreateWebView2Environment();
    HRESULT OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment);
    HRESULT OnCreateWebViewControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
    
    void RegisterEventHandlers();
    void InjectInitialScript();
    void SetupJavaScriptBridge();
    std::wstring GetEmbeddedHTML();
};