# UI System Migration Notice

## Important Change (September 2025)

The Mouse2VR project has migrated from embedded HTML string literals to external UI resource files.

### Old System (Deprecated)
- UI was embedded as C++ string literals in `WebViewWindow_HTML.h`
- Required `inline_html.py` script to convert HTML to C++ headers
- Hit MSVC C2026 error (16KB string literal limit)

### New System (Current)
- UI files are deployed as external resources in `resources/ui/` folder
- WebView2 loads directly from `file:///` URLs
- No string literal limitations
- Supports live UI editing in development mode

### Migration Details

#### What Changed:
1. **WebViewWindow.cpp** now loads UI from external files via `LoadUIFromFiles()`
2. **CMakeLists.txt** copies UI files to `resources/ui/` during build
3. **PathUtils** added wide string support for file:/// URLs
4. **DEV_UI mode** allows loading directly from source for development

#### Deprecated Files:
- `scripts/inline_html.py` → `scripts/inline_html_DEPRECATED.py`
- `src/webview/WebViewWindow_HTML.h` (no longer generated or used)

#### New File Structure:
```
build/bin/Release/
├── Mouse2VR_WebView.exe
└── resources/
    └── ui/
        ├── index.html
        ├── styles.css
        └── app.js
```

### For Developers

To enable development mode with live UI editing:
1. Edit `src/webview/WebViewWindow.cpp`
2. Uncomment `#define DEV_UI`
3. UI changes will be reflected without rebuilding

See `src/webview/ui/README.md` for complete development workflow.