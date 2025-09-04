@echo off
setlocal enabledelayedexpansion

:: SkyMP Windows Build Script
:: Configures MSVC environment and builds the project with proper error handling

if "%1"=="--help" (
    echo Usage: build_server.bat [--help] [--clean]
    echo.
    echo Options:
    echo   --help    Show this help message
    echo   --clean   Clean build directory before building
    echo.
    echo This script sets up the Visual Studio build environment and builds SkyMP.
    echo Requires Visual Studio 2022 Build Tools and Windows SDK.
    exit /b 0
)

echo SkyMP Windows Build Script
echo ==========================

:: Clean build directory if requested
if "%1"=="--clean" (
    echo Cleaning build directory...
    if exist build rmdir /s /q build
    echo Clean completed.
    echo.
)

:: Find and setup Visual Studio environment
echo Setting up Visual Studio environment...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VSINSTALLDIR=%%i"
    if exist "!VSINSTALLDIR!\VC\Auxiliary\Build\vcvars64.bat" (
        call "!VSINSTALLDIR!\VC\Auxiliary\Build\vcvars64.bat" >nul
        if !ERRORLEVEL! neq 0 (
            echo ERROR: Failed to setup Visual Studio environment
            exit /b 1
        )
    ) else (
        echo ERROR: Visual Studio 2022 Build Tools not found
        echo Please install Visual Studio 2022 Build Tools with C++ workload
        exit /b 1
    )
) else (
    echo ERROR: Visual Studio Installer not found
    echo Please install Visual Studio 2022 Build Tools
    exit /b 1
)

:: Create build directory
echo Creating build directory...
if not exist build mkdir build
cd build
if !ERRORLEVEL! neq 0 (
    echo ERROR: Failed to create or enter build directory
    exit /b 1
)

:: Set compiler environment variables
echo Setting compiler environment variables...
set VCPKG_DISABLE_METRICS=1
set CMAKE_C_COMPILER=cl.exe
set CMAKE_CXX_COMPILER=cl.exe
set CC=cl.exe
set CXX=cl.exe

:: Configure CMake
echo Configuring CMake...
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_UNIT_TESTS=OFF -DBUILD_SCRIPTS=OFF -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -G "Visual Studio 17 2022" -A x64

if !ERRORLEVEL! neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    echo Common solutions:
    echo 1. Ensure Visual Studio 2022 Build Tools are installed
    echo 2. Ensure Windows SDK is installed
    echo 3. Check that vcpkg dependencies are available
    echo.
    exit /b 1
)

:: Build project
echo.
echo Building project...
cmake --build . --config Release

if !ERRORLEVEL! neq 0 (
    echo.
    echo ERROR: Build failed!
    echo Check the output above for specific error messages.
    exit /b 1
) else (
    echo.
    echo SUCCESS: Build completed successfully!
    echo Binary location: build\dist\server\
)