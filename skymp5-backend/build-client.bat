@echo off
setlocal

:: ============================================================
::  Builds the client files bucket that the launcher downloads.
::    build\dist\client\Data  (from CI artifact or local build)
::      -> build\client-files\root\        (unpacked, served at /files/root)
::      -> build\client-files\skymp-client.zip  (served at /api/files/zip)
::  Safe to re-run any time. The running backend picks up the new
::  files immediately - no service restart needed.
:: ============================================================

set "BACKEND_DIR=%~dp0"
if "%BACKEND_DIR:~-1%"=="\" set "BACKEND_DIR=%BACKEND_DIR:~0,-1%"
set "REPO_DIR=%BACKEND_DIR%\.."

where node >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Node.js was not found on PATH. Install it from https://nodejs.org
    pause
    exit /b 1
)

if not exist "%REPO_DIR%\build\dist\client\Data" (
    echo [ERROR] No client build found at build\dist\client\Data
    echo Download the "dist" artifact from the latest green GitHub Actions run
    echo and unzip it into the repo's build\ folder, then re-run this script.
    pause
    exit /b 1
)

if not exist "%BACKEND_DIR%\node_modules" (
    echo Installing dependencies...
    pushd "%BACKEND_DIR%"
    call npm install
    popd
)

echo Building client files bucket...
pushd "%BACKEND_DIR%"
call npm run build-client
if errorlevel 1 (
    popd
    echo [ERROR] build-client failed - see output above.
    pause
    exit /b 1
)
popd

:: Clean up the old output location, if it still exists from earlier versions
if exist "%BACKEND_DIR%\public\files\root" (
    echo Removing old output at public\files\root ...
    rmdir /s /q "%BACKEND_DIR%\public\files\root"
)
if exist "%BACKEND_DIR%\public\files" del /q "%BACKEND_DIR%\public\files\*.zip" 2>nul

echo.
echo === Result ===
if exist "%REPO_DIR%\build\client-files\skymp-client.zip" (
    for %%F in ("%REPO_DIR%\build\client-files\skymp-client.zip") do echo Zip:     %%~fF  (%%~zF bytes^)
) else (
    echo [WARNING] Expected zip not found at build\client-files\skymp-client.zip
)
if exist "%BACKEND_DIR%\data\files-version.json" (
    echo Version:
    type "%BACKEND_DIR%\data\files-version.json"
)
echo.
pause