#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <microsoft.ui.xaml.window.h>
#include <shobjidl_core.h>

// Core includes
#include "../../include/core/RawInputHandler.h"
#include "../../include/core/ViGEmController.h"
#include "../../include/core/InputProcessor.h"
#include "../../include/core/ConfigManager.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::Mouse2VR::implementation
{
    // Core bridge to integrate with existing Mouse2VR logic
    struct MainWindow::CoreBridge
    {
        std::unique_ptr<::Mouse2VR::RawInputHandler> inputHandler;
        std::unique_ptr<::Mouse2VR::ViGEmController> controller;
        std::unique_ptr<::Mouse2VR::InputProcessor> processor;
        std::unique_ptr<::Mouse2VR::ConfigManager> configManager;
        
        std::thread workerThread;
        std::atomic<bool> running{false};
        
        float currentSpeed = 0.0f;
        float currentStickY = 0.0f;
        float updateRate = 0.0f;
        
        bool Initialize(HWND hwnd)
        {
            configManager = std::make_unique<::Mouse2VR::ConfigManager>("config.json");
            configManager->Load();
            
            inputHandler = std::make_unique<::Mouse2VR::RawInputHandler>();
            if (!inputHandler->Initialize(hwnd))
                return false;
            
            controller = std::make_unique<::Mouse2VR::ViGEmController>();
            if (!controller->Initialize())
                return false;
            
            processor = std::make_unique<::Mouse2VR::InputProcessor>();
            processor->SetConfig(configManager->GetConfig().toProcessingConfig());
            
            return true;
        }
        
        void Start()
        {
            running = true;
            workerThread = std::thread([this]()
            {
                auto lastUpdate = std::chrono::steady_clock::now();
                int updateCount = 0;
                auto metricsResetTime = std::chrono::steady_clock::now();
                
                while (running)
                {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration<float>(now - lastUpdate).count();
                    
                    // Get mouse deltas
                    auto delta = inputHandler->GetAndResetDeltas();
                    
                    // Process input
                    float stickX, stickY;
                    processor->ProcessDelta(delta, elapsed, stickX, stickY);
                    
                    // Update controller
                    controller->SetLeftStick(0.0f, stickY);
                    controller->Update();
                    
                    // Update metrics
                    currentSpeed = processor->GetSpeedMetersPerSecond();
                    currentStickY = stickY;
                    
                    updateCount++;
                    auto metricsDuration = std::chrono::duration<float>(now - metricsResetTime).count();
                    if (metricsDuration >= 1.0f)
                    {
                        updateRate = updateCount / metricsDuration;
                        updateCount = 0;
                        metricsResetTime = now;
                    }
                    
                    lastUpdate = now;
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
            });
        }
        
        void Stop()
        {
            running = false;
            if (workerThread.joinable())
                workerThread.join();
        }
        
        void ApplySettings(float sensitivity, int updateRateMs, bool invertY, bool lockX, bool adaptive)
        {
            auto config = configManager->GetConfig();
            config.sensitivity = sensitivity;
            config.updateIntervalMs = updateRateMs;
            config.invertY = invertY;
            config.lockX = lockX;
            config.adaptiveMode = adaptive;
            
            configManager->SetConfig(config);
            processor->SetConfig(config.toProcessingConfig());
        }
    };

    MainWindow::MainWindow()
    {
        InitializeComponent();
        InitializeCore();
        
        // Setup update timer for UI
        m_updateTimer = DispatcherTimer();
        m_updateTimer.Interval(std::chrono::milliseconds(100));
        m_updateTimer.Tick([this](auto&&, auto&&) { UpdateStatus(); });
        m_updateTimer.Start();
    }

    MainWindow::~MainWindow()
    {
        if (m_coreBridge)
        {
            m_coreBridge->Stop();
        }
    }

    void MainWindow::InitializeCore()
    {
        // Get the native window handle using WinUI 3 approach
        HWND hwnd = nullptr;
        auto windowNative = this->as<IWindowNative>();
        windowNative->get_WindowHandle(&hwnd);
        
        m_coreBridge = std::make_unique<CoreBridge>();
        if (m_coreBridge->Initialize(hwnd))
        {
            m_coreBridge->Start();
            StatusText().Text(L"Status: Running");
        }
        else
        {
            StatusText().Text(L"Status: Failed to initialize");
        }
    }

    void MainWindow::UpdateStatus()
    {
        if (!m_coreBridge)
            return;
        
        // Update status text
        wchar_t buffer[256];
        swprintf_s(buffer, L"Speed: %.2f m/s", m_coreBridge->currentSpeed);
        SpeedText().Text(buffer);
        
        swprintf_s(buffer, L"Stick: %.0f%%", abs(m_coreBridge->currentStickY) * 100.0f);
        StickText().Text(buffer);
        
        swprintf_s(buffer, L"Update Rate: %.0f Hz", m_coreBridge->updateRate);
        UpdateRateText().Text(buffer);
        
        // Update visualizations
        DrawStickPosition(0.0f, m_coreBridge->currentStickY);
    }

    void MainWindow::OnApplySettings(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_coreBridge)
            return;
        
        float sensitivity = static_cast<float>(SensitivitySlider().Value());
        
        // Get selected update rate
        int updateRateMs = 20; // Default 50Hz
        auto selectedIndex = UpdateRateRadio().SelectedIndex();
        switch (selectedIndex)
        {
            case 0: updateRateMs = 33; break; // 30Hz
            case 1: updateRateMs = 20; break; // 50Hz
            case 2: updateRateMs = 16; break; // 60Hz
            case 3: updateRateMs = 10; break; // 100Hz
        }
        
        bool invertY = InvertYCheck().IsChecked().Value();
        bool lockX = LockXCheck().IsChecked().Value();
        bool adaptive = AdaptiveCheck().IsChecked().Value();
        
        m_coreBridge->ApplySettings(sensitivity, updateRateMs, invertY, lockX, adaptive);
    }

    void MainWindow::DrawStickPosition(float x, float y)
    {
        // Simple visualization - will enhance later
        // For now, just clear the canvas
        StickCanvas().Children().Clear();
        
        // Draw center dot
        auto ellipse = Shapes::Ellipse();
        ellipse.Width(10);
        ellipse.Height(10);
        ellipse.Fill(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 255, 0, 0)));
        
        // Position based on stick values
        double canvasWidth = StickCanvas().ActualWidth();
        double canvasHeight = StickCanvas().ActualHeight();
        
        if (canvasWidth > 0 && canvasHeight > 0)
        {
            double posX = (canvasWidth / 2.0) + (x * canvasWidth / 2.0);
            double posY = (canvasHeight / 2.0) - (y * canvasHeight / 2.0);
            
            Canvas::SetLeft(ellipse, posX - 5);
            Canvas::SetTop(ellipse, posY - 5);
            
            StickCanvas().Children().Append(ellipse);
        }
    }

    void MainWindow::DrawInputGraph()
    {
        // TODO: Implement graph drawing
        // For now, leave empty
    }
}