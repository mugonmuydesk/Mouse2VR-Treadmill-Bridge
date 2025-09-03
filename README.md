# Mouse2VR Treadmill Bridge 🏃‍♂️🎮

Convert your DIY treadmill's mouse sensor input into smooth VR locomotion!

**✅ v2.4.0 Update: Core functionality fully restored! The app now properly captures mouse input and controls the virtual gamepad.**

## 🎯 What This Does

Mouse2VR bridges the gap between physical treadmill movement and virtual reality locomotion. It captures raw mouse movement from a sensor attached to your treadmill belt and translates it into Xbox controller stick input that VR games understand.

**Key Features:**
- ✅ Raw Input API - No screen edge limitations
- ✅ Direct ViGEm integration - Creates virtual Xbox 360 controller
- ✅ Zero smoothing - Your treadmill provides real physics
- ✅ High performance - 100Hz update rate
- ✅ Multiple interfaces - Console and WebView2 GUI
- ✅ Modular architecture - Core library with pluggable UI frontends
- ✅ Fixed mouse direction mapping - Forward movement works correctly
- ✅ Standard Windows behavior - Normal minimize/maximize/close buttons
- ✅ Optional system tray - Available but not forced

## 🔧 Physical Setup

1. **Mount a USB mouse** to your treadmill frame
2. **Position the sensor** to read the belt movement
3. **Connect mouse** to your PC
4. **Run Mouse2VR** to start translating movement
5. **Configure your VR game** to use Xbox controller for locomotion

```
Treadmill Belt → Mouse Sensor → Mouse2VR → Virtual Xbox Controller → VR Game
                                    ↑
                              (You are here)
```

## 📦 Installation

### Prerequisites
- Windows 10/11
- [ViGEmBus Driver](https://github.com/ViGEm/ViGEmBus/releases) (required for virtual controller)
- Visual C++ Redistributables 2019+
- WebView2 Runtime (usually pre-installed on Windows 11)

### ⚠️ Windows Security False Positive
Windows Defender may flag Mouse2VR_WebView.exe as suspicious. This is a **false positive** because:
- The executable is unsigned (code signing certificates cost $300+/year)
- It contains virtual gamepad drivers that trigger antivirus heuristics
- It's a new/rare file that Windows hasn't seen before

**To fix this:**
1. Click "More info" → "Run anyway" when Windows blocks it
2. Or add an exclusion: Windows Security → Virus & threat protection → Exclusions → Add folder `C:\Dev\Releases\Mouse2VR\`
3. The source code is fully open and built transparently on GitHub Actions

### Quick Start
1. Download the latest release from [Releases](https://github.com/mugonmuydesk/Mouse2VR-Treadmill-Bridge/releases)
2. Install ViGEmBus driver if not already installed
3. Extract Mouse2VR.exe
4. Run Mouse2VR.exe
5. Start walking!

## 🎮 Usage

### Basic Operation
```bash
Mouse2VR.exe
```

The application will:
1. Initialize Raw Input to capture mouse movement
2. Create a virtual Xbox 360 controller
3. Begin translating mouse Y-axis movement to left stick Y-axis
4. Display real-time speed and stick deflection

### Controls
- **Walk Forward** → Character moves forward
- **Walk Backward** → Character moves backward  
- **Stop** → Character stops
- **Ctrl+C** → Exit application

## 🛠️ Building from Source

### Requirements
- Windows 10/11
- CMake 3.20+
- Visual Studio 2019+ or MinGW-w64
- Git

### Build Steps
```bash
# Clone repository
git clone --recursive https://github.com/mugonmuydesk/Mouse2VR-Treadmill-Bridge.git
cd Mouse2VR-Treadmill-Bridge

# Create build directory
mkdir build
cd build

# Configure
cmake .. -DBUILD_CONSOLE=ON -DBUILD_WEBVIEW=ON

# Build
cmake --build . --config Release

# Run console version (for testing)
./bin/Release/Mouse2VR.exe

# Or run WebView2 GUI version
./bin/Release/Mouse2VR_WebView.exe
```

## 🗺️ Roadmap

### v1.0 - MVP ✅ Complete
- [x] Raw Input mouse capture
- [x] ViGEm virtual controller
- [x] Basic console interface
- [x] Fixed sensitivity
- [x] Config file support (ConfigManager implemented)

### v2.0 - Enhanced UI (In Progress - Current v2.4.0)
- [x] Modular core library architecture
- [x] WebView2 GUI implementation with optional system tray
- [x] Modern HTML/CSS/JS interface
- [x] Debug logging capabilities
- [x] Fixed mouse direction mapping
- [x] Standard Windows application behavior
- [x] **Core functionality restored** - Mouse input now properly connected to virtual controller
- [x] Real-time speed display from actual sensor data
- [ ] Live input visualization
- [ ] Settings panel (configuration methods implemented, UI pending)
- [ ] Calibration wizard

### v3.0 - Advanced Features
- [ ] Per-game profiles
- [ ] SteamVR integration
- [ ] Community profile sharing
- [ ] Multiple input device support
- [ ] IMU sensor support

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests.

### Development Setup
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📝 Technical Details

### Architecture
- **Core Library**: `Mouse2VRCore` - Modular design with separated components
- **Input Layer**: Windows Raw Input API for unrestricted mouse capture (`RawInputHandler`)
- **Processing Layer**: Configurable scaling and axis mapping (`InputProcessor`)
- **Configuration**: JSON-based settings management (`ConfigManager`)
- **Output Layer**: ViGEmBus for virtual Xbox 360 controller (`ViGEmController`)
- **UI Options**: Console, Win32 GUI, and WinUI 3 frontends
- **Update Rate**: 100Hz (10ms intervals)

### Why Raw Input?
Traditional mouse input is limited by screen boundaries. When the cursor hits the edge, movement stops. Raw Input provides continuous delta values regardless of cursor position, perfect for treadmill applications.

## 🐛 Troubleshooting

### "Failed to initialize virtual controller"
- Install [ViGEmBus driver](https://github.com/ViGEm/ViGEmBus/releases)
- Restart your computer after installation

### "No movement detected"
- Check mouse is connected and working
- Try a different USB port
- Ensure mouse sensor can read treadmill belt

### "Movement is reversed"
- Fixed in latest version - forward mouse movement now correctly moves character forward
- Axis inversion available through configuration if needed

## 📄 License

This project is licensed under the GPL-3.0 License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [ViGEmBus](https://github.com/ViGEm/ViGEmBus) - Virtual Gamepad Emulation Framework
- Original Mouse2Joystick project for inspiration
- The VR community for continuous support

## 📬 Contact

Project Link: [https://github.com/mugonmuydesk/Mouse2VR-Treadmill-Bridge](https://github.com/mugonmuydesk/Mouse2VR-Treadmill-Bridge)

---

**Made with ❤️ for the VR treadmill community**
