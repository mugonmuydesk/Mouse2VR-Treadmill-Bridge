# Mouse2VR WebView UI Development

This directory contains the HTML, CSS, and JavaScript files for the Mouse2VR WebView interface.

## Files

- `index.html` - Main HTML file with inline styles and scripts
- `index_dev.html` - Development version with external CSS/JS references
- `styles.css` - All CSS styles (used in development)
- `app.js` - All JavaScript code (used in development)

## New External Resource Architecture

The application now loads UI files directly from the filesystem instead of embedding them as string literals. This eliminates the MSVC C2026 string literal size limit and enables live UI editing during development.

### How It Works

1. **Production Mode** (default):
   - UI files are copied to `resources/ui/` folder during build
   - WebView2 loads from `file:///path/to/executable/resources/ui/index.html`
   - Files are deployed alongside the executable

2. **Development Mode** (when `#define DEV_UI` is enabled in WebViewWindow.cpp):
   - UI files are loaded directly from `src/webview/ui/`
   - Changes to HTML/CSS/JS are reflected immediately on refresh
   - No rebuild required for UI changes

### File Deployment Structure
```
Mouse2VR_WebView.exe
└── resources/
    └── ui/
        ├── index.html
        ├── styles.css
        └── app.js
```

## Development Workflow

### 1. Enable Development Mode
Edit `src/webview/WebViewWindow.cpp` and uncomment:
```cpp
#define DEV_UI  // Uncomment for development
```

### 2. Edit UI Files
- Edit `index.html` for complete UI with inline styles/scripts
- OR edit `index_dev.html`, `styles.css`, and `app.js` separately

### 3. Test Changes
- With DEV_UI enabled, just restart the application
- Changes are loaded from source files directly
- Use browser DevTools (F12) in WebView2 for debugging

### 4. Production Build
1. Comment out `#define DEV_UI` in WebViewWindow.cpp
2. Ensure `index.html` has all styles and scripts inline
3. Build the project - CMake will copy files to resources folder
4. Test that the executable works with the bundled resources

## Testing Changes Locally

You can test UI changes without rebuilding the C++ application:

1. Open `index_dev.html` in a web browser
2. Note: The `window.chrome.webview` API won't be available, so C++ integration won't work
3. You can mock the API for testing:

```javascript
// Add to app.js for browser testing
if (!window.chrome?.webview) {
    window.chrome = {
        webview: {
            postMessage: (msg) => console.log('Message to C++:', msg)
        }
    };
    window.mouse2vr = {
        // Mock functions for testing
        setSensitivity: (v) => console.log('Set sensitivity:', v),
        setUpdateRate: (v) => console.log('Set update rate:', v),
        // etc.
    };
}
```

## Important Notes

1. **MSVC String Limit**: If the HTML grows beyond ~60KB, the inline script will automatically split it into multiple chunks.

2. **Encoding**: All files use UTF-8 encoding. Ensure your editor preserves this.

3. **Version Control**: 
   - The UI source files in `src/webview/ui/` are the source of truth
   - The generated `WebViewWindow_HTML.h` should be committed after changes
   - There is no embedded HTML in the .cpp file anymore

## File Relationships

```
scripts/extract_html.py     -->  Extracts from WebViewWindow.cpp
                            -->  Creates ui/*.html, ui/*.css, ui/*.js

ui/index_dev.html           -->  Development file with external refs
ui/styles.css              -->  CSS styles
ui/app.js                  -->  JavaScript code
                            |
                            v
scripts/inline_html.py      -->  Combines files
                            -->  Generates WebViewWindow_HTML.h

WebViewWindow_HTML.h        -->  Always used by WebViewWindow.cpp
                            -->  Generated from the ui/ source files
```