project(Mouse2VR_WinUI LANGUAGES CXX)

# Find required packages
find_package(Microsoft.Windows.CppWinRT REQUIRED)
find_package(Microsoft.WindowsAppSDK REQUIRED)

# Generate C++/WinRT headers
cppwinrt_generate_headers(
    Mouse2VR_WinUI_HEADERS
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}
    NAMESPACE Mouse2VR
)

# WinUI 3 executable
add_executable(Mouse2VR_WinUI WIN32
    main.cpp
    pch.cpp
    App.xaml.cpp
    MainWindow.xaml.cpp
    ${Mouse2VR_WinUI_HEADERS}
)

# Add XAML files
target_sources(Mouse2VR_WinUI PRIVATE
    App.xaml
    MainWindow.xaml
    App.idl
    MainWindow.idl
)

# Set properties for XAML compilation
set_property(SOURCE App.xaml PROPERTY VS_XAML_TYPE "ApplicationDefinition")
set_property(SOURCE MainWindow.xaml PROPERTY VS_XAML_TYPE "Page")

# Precompiled header
target_precompile_headers(Mouse2VR_WinUI PRIVATE pch.h)

# Include directories
target_include_directories(Mouse2VR_WinUI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Link libraries
target_link_libraries(Mouse2VR_WinUI PRIVATE
    Mouse2VRCore
    Microsoft::WindowsAppSDK
    Microsoft::Windows::CppWinRT
)

# C++/WinRT properties
set_target_properties(Mouse2VR_WinUI PROPERTIES
    VS_WINRT_COMPONENT TRUE
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Deployment
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(TARGET Mouse2VR_WinUI PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)")
endif()