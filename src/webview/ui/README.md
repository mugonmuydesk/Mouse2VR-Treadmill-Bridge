# Mouse2VR WebView UI Development

This directory contains the extracted HTML, CSS, and JavaScript files for the Mouse2VR WebView interface.

## Files

- `index.html` - Complete HTML file as embedded in the C++ binary
- `index_dev.html` - Development version with external CSS/JS references
- `styles.css` - All CSS styles extracted from the HTML
- `app.js` - All JavaScript code extracted from the HTML

## Development Workflow

### 1. Initial Setup (already done)
```bash
# Extract HTML from the C++ file
python scripts/extract_html.py
```

### 2. Development Process

#### Option A: Edit the complete file
Edit `index.html` directly with all inline styles and scripts.

#### Option B: Edit separate files (recommended)
1. Edit `styles.css` for styling changes
2. Edit `app.js` for JavaScript functionality
3. Edit `index_dev.html` for HTML structure changes
4. Test locally by opening `index_dev.html` in a browser

### 3. Build for C++
After making changes, run:
```bash
# Windows
scripts\update_html.bat

# Or directly with Python
python scripts/inline_html.py
```

This generates `WebViewWindow_HTML.h` with your changes.

### 4. Enable in C++
In `src/webview/WebViewWindow.cpp`, uncomment the line:
```cpp
#define USE_EXTERNAL_HTML
```

### 5. Rebuild the project
Push to GitHub and let Actions build, or build locally.

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
   - The `.cpp` file still contains the original embedded HTML for backward compatibility
   - The generated `WebViewWindow_HTML.h` should be committed when ready for production
   - The UI files in this directory are the source of truth when `USE_EXTERNAL_HTML` is defined

4. **Switching Back**: To use the original embedded HTML, simply comment out:
   ```cpp
   // #define USE_EXTERNAL_HTML
   ```

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

WebViewWindow_HTML.h        -->  Included by WebViewWindow.cpp
                            -->  Used when USE_EXTERNAL_HTML is defined
```