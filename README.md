# Mouse2VR Treadmill Bridge 🏃‍♂️🎮

Transform any treadmill into a VR locomotion device using a simple USB mouse!

**Latest: v2.8.3** - DPI calibration, performance optimizations, and true portability

## 🎯 What It Does

Mouse2VR captures movement from a mouse sensor attached to your treadmill belt and translates it into virtual Xbox controller input for smooth, natural VR locomotion. Walk on your treadmill, move in VR - it's that simple.

## 🚀 Key Features

### Core Functionality
- **Raw Input API** - Direct mouse capture with no screen boundaries
- **Virtual Xbox Controller** - ViGEm creates a virtual gamepad that any game recognizes
- **Real Physics** - No artificial smoothing; your actual movement drives the game
- **Adjustable Performance** - Target 25/45/60 Hz update rates

### Modern UI (v2.7+)
- **Fluent Design Interface** - Clean, modern Windows 11 style
- **Live Performance Metrics** - See actual vs target update rates
- **Dual-Trace Speed Graph** - Monitor both treadmill and game speed
- **Toggle Switch Control** - Simple on/off for virtual controller
- **Sensitivity Slider** - Visual feedback with 1.0 reference notch

### New in v2.8+
- **DPI Calibration** - Presets for 400, 800, 1000, 1200, 1600, 3200 DPI mice
- **Accurate Speed Calculation** - Real physical speed in m/s based on mouse DPI
- **5-Second Movement Test** - Diagnostics button with detailed logging
- **True Portability** - Config and logs stored relative to exe location
- **Performance Optimizations** - Async logging, high-res timers, stable 45 Hz

## 📦 Installation

### Prerequisites
1. **Windows 10/11** (64-bit)
2. **[ViGEmBus Driver](https://github.com/ViGEm/ViGEmBus/releases)** - Required for virtual controller
3. **[Visual C++ Runtime 2019+](https://aka.ms/vs/17/release/vc_redist.x64.exe)** - If not already installed
4. **WebView2 Runtime** - Pre-installed on Windows 11, [download for Windows 10](https://go.microsoft.com/fwlink/p/?LinkId=2124703)

### Download & Run
1. Download the latest release from [Releases](https://github.com/mugonmuydesk/Mouse2VR-Treadmill-Bridge/releases)
2. Extract to any folder
3. Run `Mouse2VR_WebView.exe` for the GUI version
4. Or run `Mouse2VR.exe` for console/debugging

### ⚠️ Windows Security Notice
Windows Defender may flag the app as unrecognized. This is normal for unsigned software. Click "More info" → "Run anyway" to proceed. The app is [open source](https://github.com/mugonmuydesk/Mouse2VR-Treadmill-Bridge) and safe.

## 🔧 Physical Setup

### What You Need
- USB mouse (any DPI, optical recommended)
- Way to mount mouse to treadmill frame
- Mouse sensor positioned to read belt movement

### Setup Steps
1. **Mount the mouse** securely to your treadmill frame
2. **Position sensor** 1-5mm above the belt for optimal tracking
3. **Ensure good lighting** - sensor needs to see belt texture
4. **Connect to PC** via USB
5. **Configure your VR game** to accept Xbox controller input for movement

```
Treadmill Belt → Mouse Sensor → Mouse2VR → Virtual Controller → VR Game
                     ↓              ↑              ↓
                  Movement      You Are Here    Locomotion
```

## 🎮 Usage Guide

### Quick Start
1. Launch Mouse2VR_WebView.exe
2. Toggle "Enable Virtual Controller" ON (default)
3. Start walking on treadmill
4. See your speed in the app
5. Launch your VR game and configure it for gamepad locomotion

### Settings Explained

**Sensitivity (0.1 - 3.0)**  
Multiplies mouse input. Default 1.0 works for most setups.
- Lower = need more belt movement for same game speed
- Higher = less belt movement needed

**Target Update Rate**  
How often to process input (Hz). Higher = more responsive.
- 25 Hz - Low latency, minimal CPU usage
- 45 Hz - Balanced performance
- 60 Hz - Smoothest movement

**Axis Options**
- **Invert Y** - Reverses forward/backward if needed
- **Lock X** - Disables side-to-side (treadmill = forward/back only)
- **Adaptive Mode** - (Future) Dynamic sensitivity based on speed

## 📊 Understanding Mouse Input

### How Mouse DPI Affects Speed

The app reads mouse movement in "counts" (also called mickeys). Your mouse DPI determines how many counts equal real-world distance:

- **800 DPI mouse** = 800 counts per inch = ~31,500 counts per meter
- **1600 DPI mouse** = 1600 counts per inch = ~63,000 counts per meter
- **400 DPI mouse** = 400 counts per inch = ~15,750 counts per meter

The app now includes DPI calibration settings. Select your mouse DPI from the settings panel (default: 1000 DPI) for accurate speed measurements in m/s. Use the 5-second test feature to verify your settings.

### Performance Metrics

**Target vs Actual Update Rate**  
With v2.8.3 performance optimizations:
- Target 25 Hz → Actual 24-25 Hz
- Target 45 Hz → Actual 44-45 Hz (stable)
- Target 60 Hz → Actual 58-60 Hz

The app now uses high-resolution timers (1ms precision) and QueryPerformanceCounter for accurate timing. UI updates at 5 Hz to reduce overhead while maintaining responsiveness.

## 🛠️ Troubleshooting

### Mouse Not Detected
- Run as Administrator
- Check mouse is plugged in before launching
- Try the console version to see debug output

### No Movement in Game
- Ensure ViGEmBus driver is installed
- Check game accepts Xbox controller input
- Verify "Enable Virtual Controller" is ON
- Look for green "Running" status

### Erratic Movement
- Clean mouse sensor lens
- Check belt has visible texture (not pure black)
- Adjust mounting distance (1-5mm from belt)
- Try different sensitivity settings

### Performance Issues
- Lower target update rate to 25 Hz
- Close other USB polling software
- Check CPU usage in Task Manager

## 🔨 Building from Source

### Requirements
- Visual Studio 2022 or Build Tools
- CMake 3.20+
- Git

### Build Steps

**Note:** This project uses GitHub Actions for all builds. Push to GitHub and download artifacts from the Actions tab.

For local development only:
```bash
git clone https://github.com/mugonmuydesk/Mouse2VR-Treadmill-Bridge.git
cd Mouse2VR-Treadmill-Bridge
cmake -B build
cmake --build build --config Release
```

## 📄 Technical Details

### Architecture
- **Core Library** - Platform-agnostic input processing
- **Win32 Host** - Native Windows application
- **WebView2 UI** - Modern HTML/JS interface
- **ViGEm Integration** - Virtual gamepad creation

### Data Flow
1. Raw Input API captures unfiltered mouse deltas
2. InputProcessor scales and applies settings
3. ViGEmController updates virtual Xbox stick
4. Game reads controller as normal gamepad input

## 🤝 Contributing

Contributions welcome! Please check existing issues first.

### Areas for Improvement
- DPI-based calibration system
- Speed profiles for different games
- Curved speed-to-stick mapping
- SteamVR overlay integration

## 🧪 Testing Status

### Automated Testing
A comprehensive automated test suite has been implemented to validate that all runtime settings affect behavior correctly:

- **DPI Settings Validation** - Tests 400, 800, 1000, 1200, 1600, 3200 DPI
- **Sensitivity Scaling** - Tests 0.5x, 1.0x, 1.5x, 2.0x sensitivity  
- **Update Rate Tests** - Validates 25 Hz, 45 Hz, 60 Hz processing frequencies
- **Axis Options** - Tests Y-axis inversion and X-axis lock
- **Virtual Controller Toggle** - Tests enable/disable behavior
- **Runtime Setting Changes** - Validates settings can change during operation
- **WebView Rate Validation** - Ensures WebView polling matches target Hz
- **Cross-Setting Persistence** - Tests multiple settings don't revert to defaults

**Current Status**: Tests compile successfully but require ViGEm driver to run. CI environment lacks driver support, so tests must be run locally on development machines with ViGEm installed.

### Enhanced Logging (v2.8.5+)
Every log entry now includes complete settings snapshot:
```
[2025-09-04 10:45:23] [INFO] [Core] Message [DPI:1000|Sens:1.0|Hz:45|InvY:0|LockX:1|Run:1|ActHz:44|Spd:0.25|Stk:4.1%]
```

This provides moment-by-moment tracking of all settings changes, making it easy to verify settings propagation and debug issues.

## 📜 License

MIT License - See [LICENSE](LICENSE) file for details

## 🙏 Acknowledgments

- [ViGEm](https://vigem.org/) - Virtual Gamepad Emulation Framework
- [WebView2](https://developer.microsoft.com/microsoft-edge/webview2/) - Modern Web Platform for Windows
- The VR fitness community for inspiration

---

*Transform your fitness routine into VR adventures!* 🏃‍♂️🎮