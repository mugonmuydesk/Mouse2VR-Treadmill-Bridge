cmake_minimum_required(VERSION 3.20)
project(Mouse2VR VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform check
if(NOT WIN32)
    message(FATAL_ERROR "Mouse2VR only supports Windows")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add subdirectories
add_subdirectory(external)

# Find packages
find_package(Threads REQUIRED)

# Collect source files
file(GLOB_RECURSE SOURCES_COMMON
    src/input/*.cpp
    src/output/*.cpp
    src/processing/*.cpp
    src/config/*.cpp
)

file(GLOB_RECURSE SOURCES_GUI
    src/ui/*.cpp
)

file(GLOB_RECURSE HEADERS
    include/*.h
    include/*.hpp
)

# Console version
add_executable(${PROJECT_NAME} 
    src/main.cpp
    ${SOURCES_COMMON}
    ${HEADERS}
)

# GUI version
add_executable(${PROJECT_NAME}_GUI
    WIN32  # Windows subsystem
    src/main_gui.cpp
    ${SOURCES_COMMON}
    ${SOURCES_GUI}
    ${HEADERS}
    src/Mouse2VR_GUI.manifest
)

# Include directories for both versions
foreach(target ${PROJECT_NAME} ${PROJECT_NAME}_GUI)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/ViGEmClient/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include
    )
endforeach()

# Link libraries for console version
target_link_libraries(${PROJECT_NAME} PRIVATE
    Threads::Threads
    ViGEmClient
    setupapi
    winmm
    version
    imm32
    Xinput
)

# Link libraries for GUI version
target_link_libraries(${PROJECT_NAME}_GUI PRIVATE
    Threads::Threads
    ViGEmClient
    setupapi
    winmm
    version
    imm32
    Xinput
    comctl32
    shell32
    user32
    gdi32
)

# Windows specific settings
if(WIN32)
    foreach(target ${PROJECT_NAME} ${PROJECT_NAME}_GUI)
        target_compile_definitions(${target} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
        )
    endforeach()
    
    # Set Windows subsystem
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE  # Console app
    )
    set_target_properties(${PROJECT_NAME}_GUI PROPERTIES
        WIN32_EXECUTABLE TRUE   # GUI app (no console)
    )
endif()

# Copy ViGEmClient DLL to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ViGEmClient>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# Install rules
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/default_config.json 
        DESTINATION bin 
        RENAME m2j_config.json
        OPTIONAL)