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
file(GLOB_RECURSE SOURCES 
    src/*.cpp
    src/*.c
)

file(GLOB_RECURSE HEADERS
    include/*.h
    include/*.hpp
)

# Create executable
add_executable(${PROJECT_NAME} 
    ${SOURCES} 
    ${HEADERS}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/ViGEmClient/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Threads::Threads
    ViGEmClient
    setupapi
    winmm
    version
    imm32
    Xinput
)

# Windows specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    
    # Set Windows subsystem
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE  # Console app for now, change to TRUE for GUI
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