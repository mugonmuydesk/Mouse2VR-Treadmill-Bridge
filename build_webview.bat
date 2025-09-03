@echo off
setlocal

echo Building Mouse2VR WebView2...
echo.

REM Setup VS environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" >nul 2>&1
if %errorlevel% neq 0 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" >nul 2>&1
)

REM Build WebView2 target
msbuild build\src\webview\Mouse2VR_WebView.vcxproj /p:Configuration=Debug /p:Platform=x64

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful! Check build\bin\Debug\Mouse2VR_WebView.exe
pause