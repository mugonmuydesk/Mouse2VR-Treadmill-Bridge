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
option(BUILD_WIN32_GUI "Build Win32 GUI application" ON)
option(BUILD_WINUI "Build WinUI 3 GUI application" OFF)

# Add subdirectories
add_subdirectory(external)

# Find packages
find_package(Threads REQUIRED)

# Core library
add_library(Mouse2VRCore STATIC
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
    WIN32_LEAN_AND_MEAN
    NOMINMAX
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

# Win32 GUI application (legacy - to be replaced by WinUI)
if(BUILD_WIN32_GUI)
    add_executable(Mouse2VR_GUI WIN32
        src/main_gui.cpp
        src/ui/MainWindow.cpp
        src/Mouse2VR_GUI.manifest
    )
    
    target_include_directories(Mouse2VR_GUI PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/ViGEmClient/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include
    )
    
    target_link_libraries(Mouse2VR_GUI PRIVATE
        Mouse2VRCore
        comctl32
        shell32
        user32
        gdi32
    )
    
    target_compile_definitions(Mouse2VR_GUI PRIVATE
        _WINDOWS
    )
    
    # Copy ViGEmClient DLL to output directory
    add_custom_command(TARGET Mouse2VR_GUI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:ViGEmClient>
            $<TARGET_FILE_DIR:Mouse2VR_GUI>
    )
endif()

# WinUI 3 application (new)
if(BUILD_WINUI)
    message(STATUS "Building WinUI 3 application")
    add_subdirectory(src/winui)
endif()

# Install rules
if(BUILD_CONSOLE)
    install(TARGETS Mouse2VR
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config.json 
    DESTINATION bin 
    OPTIONAL
)