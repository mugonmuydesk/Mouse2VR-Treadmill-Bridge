#pragma once

#include "MainWindow.g.h"
#include <memory>
#include <thread>
#include <atomic>

namespace winrt::Mouse2VR::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        ~MainWindow();

        void OnApplySettings(Windows::Foundation::IInspectable const& sender, 
                            Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        // Core components (forward declarations)
        struct CoreBridge;
        std::unique_ptr<CoreBridge> m_coreBridge;

        // UI update timer
        Microsoft::UI::Xaml::DispatcherTimer m_updateTimer;
        
        void InitializeCore();
        void UpdateStatus();
        void DrawStickPosition(float x, float y);
        void DrawInputGraph();
    };
}

namespace winrt::Mouse2VR::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}