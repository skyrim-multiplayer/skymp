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

echo Building Windows installer...
call npm run build:win
if errorlevel 1 (
    popd
    echo [ERROR] electron-builder failed - see output above.
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