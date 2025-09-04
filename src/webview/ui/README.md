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

This generates/updates `WebViewWindow_HTML.h` with your changes.

### 4. How the HTML System Works
**IMPORTANT**: The application **always** uses the external HTML from `WebViewWindow_HTML.h`. 
- There is **no embedded HTML** in WebViewWindow.cpp anymore
- The `#define USE_EXTERNAL_HTML` is always enabled
- All UI changes must be made in the `src/webview/ui/` files

### 5. Rebuild the project
After updating the header file, push to GitHub and let Actions build, or build locally.

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