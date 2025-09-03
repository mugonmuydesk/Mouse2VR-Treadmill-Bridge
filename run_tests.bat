@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo Mouse2VR Test Runner
echo ==========================================
echo.

REM Check if Visual Studio is available
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Visual Studio 2022 not found
    exit /b 1
)

REM Create build directory if it doesn't exist
if not exist build mkdir build
cd build

REM Configure with CMake
echo [1/4] Configuring CMake...
cmake -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=ON .. >cmake_config.log 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed
    type cmake_config.log
    exit /b 1
)
echo SUCCESS: CMake configured

REM Build tests
echo [2/4] Building tests...
cmake --build . --config Debug --target Mouse2VR_Tests >build.log 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    type build.log
    exit /b 1
)
echo SUCCESS: Tests built

REM Run tests
echo [3/4] Running tests...
echo.
bin\Debug\Mouse2VR_Tests.exe --gtest_output=xml:test_results.xml --gtest_color=yes
set TEST_RESULT=%errorlevel%

REM Generate summary
echo.
echo [4/4] Generating test summary...
if %TEST_RESULT% equ 0 (
    echo ALL TESTS PASSED
) else (
    echo SOME TESTS FAILED
)

REM Save results to file
echo Test Results Summary > test_summary.txt
echo ==================== >> test_summary.txt
echo Date: %date% %time% >> test_summary.txt
echo Exit Code: %TEST_RESULT% >> test_summary.txt
if exist test_results.xml (
    echo Full results saved to: build\test_results.xml >> test_summary.txt
)

cd ..
exit /b %TEST_RESULT%