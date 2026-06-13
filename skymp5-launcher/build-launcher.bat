@echo off
setlocal

:: ============================================================
::  Builds the SkyRP Launcher Windows installer.
::  Output: <repo>\build\launcher\SkyRP Launcher Setup <version>.exe
:: ============================================================

set "LAUNCHER_DIR=%~dp0"
if "%LAUNCHER_DIR:~-1%"=="\" set "LAUNCHER_DIR=%LAUNCHER_DIR:~0,-1%"
set "OUT_DIR=%LAUNCHER_DIR%\..\build\launcher"

where node >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Node.js was not found on PATH. Install it from https://nodejs.org
    pause
    exit /b 1
)

echo Installing dependencies...
pushd "%LAUNCHER_DIR%"
call npm install
if errorlevel 1 (
    popd
    echo [ERROR] npm install failed - see output above.
    pause
    exit /b 1
)

:: Don't let electron-builder auto-pick a code-signing certificate from the
:: Windows cert store. If that cert is expired/invalid, signing fails and
:: ABORTS the build right after packaging Electron. With this off the build
:: completes (unsigned). To actually sign, set CSC_LINK + CSC_KEY_PASSWORD
:: to your .pfx certificate before running this script.
if not defined CSC_LINK set "CSC_IDENTITY_AUTO_DISCOVERY=false"

echo Building Windows installer...
:: Full electron-builder output is captured to build-launcher.log for support.
call npm run build:win > "%LAUNCHER_DIR%\build-launcher.log" 2>&1
if errorlevel 1 (
    echo [ERROR] electron-builder failed. Last 30 log lines:
    powershell -NoProfile -Command "Get-Content -Tail 30 '%LAUNCHER_DIR%\build-launcher.log'"
    echo.
    echo Full log: %LAUNCHER_DIR%\build-launcher.log
    popd
    pause
    exit /b 1
)
popd

echo.
echo === Result ===
set "FOUND="
for %%F in ("%OUT_DIR%\*.exe") do (
    echo Installer: %%~fF  (%%~zF bytes^)
    set "FOUND=1"
)
if not defined FOUND (
    echo [WARNING] No installer .exe found in %OUT_DIR%
    pause
    exit /b 1
)
echo.
echo Opening output folder...
start "" explorer "%OUT_DIR%"
pause