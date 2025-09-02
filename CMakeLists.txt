cmake_minimum_required(VERSION 3.20)
project(Mouse2VR VERSION 2.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform check
if(NOT WIN32)
    message(FATAL_ERROR "Mouse2VR only supports Windows")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(BUILD_CONSOLE "Build console application" ON)
option(BUILD_WINUI "Build WinUI 3 GUI application" OFF)
option(BUILD_WEBVIEW "Build WebView2 application" ON)

# Add subdirectories
add_subdirectory(external)

# Find packages
find_package(Threads REQUIRED)

# Core library
add_library(Mouse2VRCore STATIC
    src/core/Mouse2VRCore.cpp
    src/core/Logger.cpp
    src/core/RawInputHandler.cpp
    src/core/ViGEmController.cpp
    src/core/InputProcessor.cpp
    src/core/ConfigManager.cpp
)

target_include_directories(Mouse2VRCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/ViGEmClient/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include
)

target_link_libraries(Mouse2VRCore PUBLIC
    ViGEmClient
    Threads::Threads
    setupapi
    winmm
)

target_compile_definitions(Mouse2VRCore PUBLIC
    _CRT_SECURE_NO_WARNINGS
)

# Console application
if(BUILD_CONSOLE)
    add_executable(Mouse2VR src/console/main.cpp)
    
    target_link_libraries(Mouse2VR PRIVATE
        Mouse2VRCore
    )
    
    target_compile_definitions(Mouse2VR PRIVATE
        _CONSOLE
    )
    
    # Copy ViGEmClient DLL to output directory
    add_custom_command(TARGET Mouse2VR POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:ViGEmClient>
            $<TARGET_FILE_DIR:Mouse2VR>
    )
endif()

# WinUI 3 application
if(BUILD_WINUI)
    add_subdirectory(src/winui)
endif()

# WebView2 application
if(BUILD_WEBVIEW)
    # Download WebView2 if not present
    include(FetchContent)
    FetchContent_Declare(
        webview2
        URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2210.55
        URL_HASH SHA256=c0945ddfe38272f10f24673a1cb07b54b018f614eec85d3e830628369039ad3d
    )
    FetchContent_MakeAvailable(webview2)
    
    add_executable(Mouse2VR_WebView WIN32
        src/webview/main.cpp
        src/webview/SystemTray.cpp
        src/webview/WebViewWindow.cpp
    )
    
    target_include_directories(Mouse2VR_WebView PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/webview
        ${webview2_SOURCE_DIR}/build/native/include
    )
    
    target_link_libraries(Mouse2VR_WebView PRIVATE
        Mouse2VRCore
        ${webview2_SOURCE_DIR}/build/native/x64/WebView2LoaderStatic.lib
        shell32
        ole32
        oleaut32
        uuid
        shlwapi
    )
    
    target_compile_definitions(Mouse2VR_WebView PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    
    # Copy ViGEmClient DLL to output directory
    add_custom_command(TARGET Mouse2VR_WebView POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:ViGEmClient>
            $<TARGET_FILE_DIR:Mouse2VR_WebView>
    )
endif()

# Installation
install(TARGETS Mouse2VRCore
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

if(BUILD_CONSOLE)
    install(TARGETS Mouse2VR
        RUNTIME DESTINATION bin
    )
endif()

# Copy runtime dependencies
install(FILES $<TARGET_FILE:ViGEmClient>
    DESTINATION bin
)