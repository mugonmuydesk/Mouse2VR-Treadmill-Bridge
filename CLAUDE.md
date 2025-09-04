# Claude Code Development Notes

## Current Version: v2.11.0-config-sync

### Recent Updates
- **Config-UI Synchronization**: Config file is now single source of truth
- **Rate Separation**: Independent backend target rate and UI refresh rate controls
- **HTML/JS Extraction**: Separated HTML/CSS/JS to external files for easier development
- **Direct Rate Control**: Removed hidden rate mappings (25→36, 45→70, 60→94 Hz)
- **Startup Sync**: UI now requests config on startup for proper initialization

### Key Architecture Changes

#### HTML/JS Development Workflow  
- **Single source of truth**: All UI code in `src/webview/ui/`
- **No embedded HTML**: WebViewWindow.cpp always uses WebViewWindow_HTML.h
- **Build-time generation**: Run `scripts/inline_html.py` to update header from source files
- **MSVC string limit handling**: Automatic chunking for large HTML strings

#### Config Synchronization Flow
1. Backend loads config on startup
2. UI requests config via `getConfig()` message
3. Backend sends current state via `applyConfigToUI()`
4. All UI changes persist to config file immediately
5. Config file remains single source of truth

### Testing Commands
```bash
# Build tests
"/mnt/c/Program Files/CMake/bin/cmake.exe" -B build_test -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=ON
"/mnt/c/Program Files/CMake/bin/cmake.exe" --build build_test --config Debug --target Mouse2VR_Tests

# Run tests
./build_test/bin/Debug/Mouse2VR_Tests.exe
```

### Build Commands
```bash
# Release build
"/mnt/c/Program Files/CMake/bin/cmake.exe" -B build -G "Visual Studio 17 2022" -A x64
"/mnt/c/Program Files/CMake/bin/cmake.exe" --build build --config Release
```

### Development Scripts
- `scripts/extract_html.py` - Extracts embedded HTML to separate files
- `scripts/inline_html.py` - Rebuilds WebViewWindow_HTML.h from external files

### Performance Metrics
- Backend processing: Configurable 25-60 Hz
- UI refresh rate: Independent 1/5/20 Hz
- Actual vs Target rate display for monitoring
- Real-time speed query counter for diagnostics

### Known Issues
- Tests may fail on GitHub Actions due to WebView2 dependencies
- UI refresh rate not persisted (TODO: add to config)

### File Structure
```
src/webview/
├── WebViewWindow.cpp       # Main WebView logic (no embedded HTML)
├── WebViewWindow_HTML.h    # Generated header - always used
└── ui/
    ├── index.html          # Production HTML source
    ├── index_dev.html      # Development HTML for browser testing
    ├── styles.css          # CSS styles source
    └── app.js              # JavaScript source
```