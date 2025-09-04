@echo off
REM Update HTML - Inline the HTML/CSS/JS into C++ header for compilation
REM Run this after editing files in src/webview/ui/

echo Updating HTML for C++ compilation...
python scripts\inline_html.py

if %errorlevel% neq 0 (
    echo Error: Failed to inline HTML
    pause
    exit /b 1
)

echo.
echo HTML successfully updated!
echo You can now rebuild the project.
pause