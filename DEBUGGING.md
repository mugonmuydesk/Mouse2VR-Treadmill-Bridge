# Debugging Guide for Mouse2VR-Treadmill-Bridge

## Folder Structure (Windows)

### Development Setup
```
C:\Dev\                                 # All development projects
├── Mouse2VR-Treadmill-Bridge\         # Git clone location
│   ├── src\                           # Source code
│   ├── build\                         # CMake build output
│   └── logs\                          # Debug logs
├── Mouse2Joystick4VR\                 # Other VR project
└── dev-context\                       # Your private context repo

C:\Dev\Releases\                       # Downloaded builds for testing
├── Mouse2VR\                          
│   ├── v0.1.0-20250901-abc1234\      # Properly versioned folders
│   │   ├── Mouse2VR.exe              # Console application
│   │   ├── Mouse2VR_WebView.exe      # WebView2 GUI
│   │   └── ViGEmClient.dll           # Required dependency
│   └── v0.2.0-20250903-def5678\      # Another version
└── Mouse2Joystick4VR\
    └── ...
```

### Setting Up Development Folders
```powershell
# Create development structure
New-Item -ItemType Directory -Path "C:\Dev" -Force
New-Item -ItemType Directory -Path "C:\Dev\Releases\Mouse2VR" -Force

# Clone repositories to C:\Dev
cd C:\Dev
git clone https://github.com/mugonmuydesk/Mouse2VR-Treadmill-Bridge.git
git clone https://github.com/mugonmuydesk/dev-context.git

# Build location will be C:\Dev\Mouse2VR-Treadmill-Bridge\build
```

## Quick Start Debugging

### 1. Console App (Best for logic debugging)
- Open VS Code in `C:\Dev\Mouse2VR-Treadmill-Bridge`
- Press F5 with "Debug Console App" selected
- Full debugging support with breakpoints
- Output directly in terminal

### 2. Win32 UI (Production version)
- Press F5 with "Debug Win32 UI" selected
- Has system tray support
- Check `logs\debug.log` for detailed logging

## VS Code Debugging

### Available Debug Configurations

1. **Debug Console App** - Full debugging with breakpoints
2. **Debug WebView2** - Debug GUI with Chrome DevTools
3. **Attach to Running Process** - Attach to any running instance

### Task Commands (Ctrl+Shift+P → "Tasks: Run Task")

- **Build Console Debug** - Build console app
- **Build WebView2 Debug** - Build WebView2 GUI
- **Build All** - Build console and WebView2
- **View Logs** - Tail debug.log in real-time
- **Clear Logs** - Delete all log files
- **Configure CMake** - Initial CMake setup
- **Clean Build** - Clean all build artifacts

## Logging System

All components use unified logging to `logs\debug.log`:

```cpp
// Include the logger
#include "common/Logger.h"

// Initialize once at startup
Mouse2VR::Logger::Instance().Initialize("logs/debug.log");

// Use logging macros
LOG_DEBUG("RawInput", "Mouse delta: " + std::to_string(deltaY));
LOG_INFO("Main", "Application started");
LOG_WARNING("ViGEm", "Controller reconnecting...");
LOG_ERROR("Config", "Failed to load settings");

// Log with data
LOG_INFO_DATA("Performance", "Update rate", "Hz", updateRate);

// Performance timing
{
    SCOPED_TIMER("ProcessInput");
    // Code to time
}
```

### Viewing Logs

**Real-time in VS Code:**
1. Ctrl+Shift+P → "Tasks: Run Task"
2. Select "View Logs"
3. Opens dedicated terminal tailing logs

**Manual:**
- File location: `C:\Dev\Mouse2VR-Treadmill-Bridge\logs\debug.log`
- Use any text editor or `Get-Content -Tail 50 -Wait` in PowerShell

## Common Issues & Solutions

### Build Issues

**CMake can't find dependencies:**
```bash
git submodule update --init --recursive
```

**WinUI3 build fails:**
```bash
cd src\winui
nuget restore packages.config
```

**ViGEmBus not found:**
- Install from: https://github.com/ViGEm/ViGEmBus/releases
- Restart after installation

### Runtime Issues

**No mouse input detected:**
- Run as Administrator
- Check Raw Input permissions
- Look for "RawInput" errors in debug.log

**Controller not working:**
- Check ViGEmBus service: `sc query ViGEmBus`
- Look for "ViGEm" errors in debug.log
- Try reinstalling ViGEmBus driver

**WinUI3 window doesn't appear:**
- Check Windows Event Viewer → Applications
- Look for Windows App SDK errors
- Ensure Windows App SDK runtime is installed

### Debug Output Locations

| Build Type | Console Output | Debug Output | Log File |
|------------|---------------|--------------|----------|
| Console App | ✅ Terminal | ✅ Debug Console | ✅ debug.log |
| Win32 UI | ❌ Hidden | ✅ Debug Console | ✅ debug.log |
| WinUI3 | ❌ Hidden | ❌ Swallowed | ✅ debug.log |

## Performance Profiling

Enable detailed timing in CMake:
```cmake
add_compile_definitions(ENABLE_PROFILING)
```

This logs timing for:
- Mouse input processing
- Controller update rate
- UI refresh rate
- Individual function performance

## Testing Workflow

### Local Testing
1. Build in VS Code (F5 or Tasks)
2. Test in `C:\Dev\Mouse2VR-Treadmill-Bridge\build\bin\Debug\`
3. Check logs in `logs\debug.log`

### GitHub Actions Artifacts
1. Push to GitHub
2. Wait for Actions to complete
3. Download artifacts to `C:\Dev\Releases\Mouse2VR\vX.X.X\`
4. Test downloaded build
5. Keep working build versions for comparison

### Version Management
```powershell
# Download and extract to versioned folder
Expand-Archive -Path "Mouse2VR-artifacts.zip" -DestinationPath "C:\Dev\Releases\Mouse2VR\v0.1.1\"

# Create "latest" symlink (Admin required)
New-Item -ItemType SymbolicLink -Path "C:\Dev\Releases\Mouse2VR\latest" -Target "C:\Dev\Releases\Mouse2VR\v0.1.1"
```

## Tips for Working with Claude Code

1. **Always provide logs** when reporting issues
2. **Specify which UI** (Console/Win32/WinUI3) when requesting changes
3. **Use Console app first** for testing new logic
4. **Share build output** if compilation fails
5. **Test incrementally** - don't wait for perfect code

## Advanced Debugging

### Memory Leaks
Use Visual Studio Diagnostic Tools or Application Verifier

### CPU Profiling
1. Build with Release + Debug Info
2. Use Visual Studio Performance Profiler
3. Or use Very Sleepy (lightweight profiler)

### Raw Input Debugging
```cpp
LOG_DEBUG("RawInput", "Device: " + std::to_string(raw->header.hDevice));
LOG_DEBUG("RawInput", "Flags: " + std::to_string(raw->data.mouse.usFlags));
LOG_DEBUG("RawInput", "Delta X: " + std::to_string(raw->data.mouse.lLastX));
LOG_DEBUG("RawInput", "Delta Y: " + std::to_string(raw->data.mouse.lLastY));
```

### ViGEm Controller State
```cpp
LOG_INFO("ViGEm", "Controller connected: " + std::to_string(isConnected));
LOG_DEBUG("ViGEm", "Stick X: " + std::to_string(state.sThumbLX));
LOG_DEBUG("ViGEm", "Stick Y: " + std::to_string(state.sThumbLY));
```