#!/usr/bin/env python3
"""
Extract embedded HTML/JS/CSS from WebViewWindow.cpp into separate files for development.
This makes it easier to edit the UI with proper syntax highlighting and tooling.
"""

import re
import os
import sys
from pathlib import Path

def extract_html_from_cpp(cpp_file_path, output_dir):
    """Extract HTML content from C++ raw string literals."""
    
    # Read the C++ file
    with open(cpp_file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Find the GetEmbeddedHTML function
    func_pattern = r'std::wstring\s+WebViewWindow::GetEmbeddedHTML\(\)\s*\{(.*?)\n\}'
    func_match = re.search(func_pattern, content, re.DOTALL)
    
    if not func_match:
        print("Error: Could not find GetEmbeddedHTML function")
        return False
    
    func_content = func_match.group(1)
    
    # Extract all raw string literals
    # Pattern matches LR"HTML(...content...)HTML"
    raw_string_pattern = r'LR"HTML\((.*?)\)HTML"'
    html_parts = re.findall(raw_string_pattern, func_content, re.DOTALL)
    
    if not html_parts:
        print("Error: No HTML content found in function")
        return False
    
    # Combine all parts
    full_html = ''.join(html_parts)
    
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Write the complete HTML file
    output_file = os.path.join(output_dir, 'index.html')
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(full_html)
    
    print(f"Successfully extracted HTML to: {output_file}")
    
    # Also extract CSS and JS into separate files for better organization
    extract_assets(full_html, output_dir)
    
    return True

def extract_assets(html_content, output_dir):
    """Extract CSS and JavaScript into separate files."""
    
    # Extract all <style> content
    style_pattern = r'<style>(.*?)</style>'
    styles = re.findall(style_pattern, html_content, re.DOTALL)
    
    if styles:
        css_file = os.path.join(output_dir, 'styles.css')
        with open(css_file, 'w', encoding='utf-8') as f:
            f.write('/* Extracted CSS from Mouse2VR WebView */\n\n')
            for style in styles:
                f.write(style.strip())
                f.write('\n\n')
        print(f"Extracted CSS to: {css_file}")
    
    # Extract all <script> content (excluding inline event handlers)
    script_pattern = r'<script>(.*?)</script>'
    scripts = re.findall(script_pattern, html_content, re.DOTALL)
    
    if scripts:
        js_file = os.path.join(output_dir, 'app.js')
        with open(js_file, 'w', encoding='utf-8') as f:
            f.write('// Extracted JavaScript from Mouse2VR WebView\n\n')
            for script in scripts:
                f.write(script.strip())
                f.write('\n\n')
        print(f"Extracted JavaScript to: {js_file}")
    
    # Create a development version with external references
    create_dev_html(html_content, output_dir)

def create_dev_html(html_content, output_dir):
    """Create a development HTML file with external CSS/JS references."""
    
    # Remove style tags
    html_dev = re.sub(r'<style>.*?</style>', '<link rel="stylesheet" href="styles.css">', 
                      html_content, flags=re.DOTALL, count=1)
    
    # Remove remaining style tags
    html_dev = re.sub(r'<style>.*?</style>', '', html_dev, flags=re.DOTALL)
    
    # Remove script tags and add external reference
    html_dev = re.sub(r'<script>.*?</script>', '', html_dev, flags=re.DOTALL)
    
    # Add script reference before closing body
    html_dev = html_dev.replace('</body>', '    <script src="app.js"></script>\n</body>')
    
    dev_file = os.path.join(output_dir, 'index_dev.html')
    with open(dev_file, 'w', encoding='utf-8') as f:
        f.write(html_dev)
    
    print(f"Created development HTML: {dev_file}")

def main():
    # Determine paths
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    cpp_file = project_root / 'src' / 'webview' / 'WebViewWindow.cpp'
    output_dir = project_root / 'src' / 'webview' / 'ui'
    
    if not cpp_file.exists():
        print(f"Error: C++ file not found: {cpp_file}")
        sys.exit(1)
    
    print(f"Extracting HTML from: {cpp_file}")
    print(f"Output directory: {output_dir}")
    
    if extract_html_from_cpp(cpp_file, output_dir):
        print("\nExtraction complete!")
        print("\nFiles created:")
        print("  - index.html       : Complete HTML as embedded in C++")
        print("  - index_dev.html   : Development version with external CSS/JS")
        print("  - styles.css       : Extracted CSS styles")
        print("  - app.js          : Extracted JavaScript code")
        print("\nYou can now edit these files directly for development.")
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()