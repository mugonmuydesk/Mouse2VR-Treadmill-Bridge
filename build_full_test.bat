@echo off
setlocal

echo Building Full Pipeline Test...
echo.

REM Setup VS environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" >nul 2>&1
if %errorlevel% neq 0 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" >nul 2>&1
)

REM Compile test with core files
cl /nologo /std:c++17 /EHsc ^
   /Iinclude ^
   /DWIN32 /D_WINDOWS /DWIN32_LEAN_AND_MEAN /DNOMINMAX ^
   test_full_pipeline.cpp ^
   src\core\InputProcessor.cpp ^
   /Fe:test_full_pipeline.exe

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful! Running test...
echo.
test_full_pipeline.exe
pause