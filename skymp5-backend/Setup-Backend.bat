@echo off
setlocal

:: ============================================================
::   SkyRP backend one-click installer. Double-click to run.
:: ============================================================

set "BACKEND_DIR=%~dp0"
if "%BACKEND_DIR:~-1%"=="\" set "BACKEND_DIR=%BACKEND_DIR:~0,-1%"
set "LOG_DIR=C:\logs"
set "NSSM_DIR=C:\tools\nssm"
set "SERVICE=SkyrpBackend"

:: Elevate to admin
net session >nul 2>&1
if errorlevel 1 (
    echo Requesting administrator rights...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

echo.
echo === SkyRP backend setup ===
echo Backend folder: %BACKEND_DIR%
echo.

:: Node.js
where node >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Node.js was not found on PATH.
    echo Install the LTS version from https://nodejs.org and re-run this script.
    pause
    exit /b 1
)
for /f "delims=" %%p in ('where node') do (
    set "NODE_EXE=%%p"
    goto :node_found
)
:node_found
for /f "delims=" %%v in ('node --version') do echo Found Node.js %%v at %NODE_EXE%

:: .env
if not exist "%BACKEND_DIR%\.env" (
    if exist "%BACKEND_DIR%\.env.example" (
        copy /y "%BACKEND_DIR%\.env.example" "%BACKEND_DIR%\.env" >nul
        echo.
        echo [ACTION REQUIRED] No .env found - one was just created from .env.example.
        echo Fill in your real values in the editor that opens, save, close,
        echo and run this script again.
        notepad "%BACKEND_DIR%\.env"
        exit /b 1
    )
    echo [ERROR] No .env or .env.example found in %BACKEND_DIR%.
    pause
    exit /b 1
)

:: Dependencies
echo Installing dependencies...
pushd "%BACKEND_DIR%"
call npm install
if errorlevel 1 (
    popd
    echo [ERROR] npm install failed - see output above.
    pause
    exit /b 1
)
popd

:: NSSM
set "NSSM="
where nssm >nul 2>&1
if not errorlevel 1 set "NSSM=nssm"
if not defined NSSM if exist "%NSSM_DIR%\nssm.exe" set "NSSM=%NSSM_DIR%\nssm.exe"
if not defined NSSM (
    echo Downloading NSSM 2.24...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://nssm.cc/release/nssm-2.24.zip' -OutFile \"$env:TEMP\nssm.zip\"; Expand-Archive -Path \"$env:TEMP\nssm.zip\" -DestinationPath \"$env:TEMP\nssm_extract\" -Force"
    if not exist "%TEMP%\nssm_extract\nssm-2.24\win64\nssm.exe" (
        echo [ERROR] NSSM download failed. Download https://nssm.cc/release/nssm-2.24.zip
        echo manually, place win64\nssm.exe at %NSSM_DIR%\nssm.exe and re-run.
        pause
        exit /b 1
    )
    mkdir "%NSSM_DIR%" 2>nul
    copy /y "%TEMP%\nssm_extract\nssm-2.24\win64\nssm.exe" "%NSSM_DIR%\nssm.exe" >nul
    set "NSSM=%NSSM_DIR%\nssm.exe"
)
echo Using NSSM: %NSSM%

:: Service (remove + re-add so re-runs always apply current config)
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"
echo Configuring service: %SERVICE%
"%NSSM%" stop %SERVICE% >nul 2>&1
"%NSSM%" remove %SERVICE% confirm >nul 2>&1
:: also clean up the old service name used by earlier versions of this script
"%NSSM%" stop SkyRP-Backend >nul 2>&1
"%NSSM%" remove SkyRP-Backend confirm >nul 2>&1
"%NSSM%" install %SERVICE% "%NODE_EXE%" "server.js"
"%NSSM%" set %SERVICE% AppDirectory "%BACKEND_DIR%"
"%NSSM%" set %SERVICE% AppStdout "%LOG_DIR%\backend.log"
"%NSSM%" set %SERVICE% AppStderr "%LOG_DIR%\backend-err.log"
"%NSSM%" set %SERVICE% AppRotateFiles 1
"%NSSM%" set %SERVICE% AppRotateBytes 10485760
"%NSSM%" set %SERVICE% Start SERVICE_AUTO_START
"%NSSM%" set %SERVICE% AppThrottle 5000

:: Firewall
netsh advfirewall firewall delete rule name="SkyRP WS Relay TCP 7778" >nul 2>&1
netsh advfirewall firewall add  rule name="SkyRP WS Relay TCP 7778" dir=in action=allow protocol=TCP localport=7778

:: Start
echo Starting %SERVICE%...
"%NSSM%" start %SERVICE%

echo.
echo === Status ===
"%NSSM%" status %SERVICE%
echo Waiting 5 seconds for the API to come up...
timeout /t 5 /nobreak >nul
netstat -an | findstr ":4000" | findstr LISTENING >nul
if errorlevel 1 (
    echo [WARNING] Nothing is listening on port 4000 yet.
    echo Check %LOG_DIR%\backend-err.log for errors.
) else (
    echo Backend API is listening on port 4000.
)
echo.
echo Logs:    %LOG_DIR%\backend.log and backend-err.log
echo Manage:  "%NSSM%" stop %SERVICE%   /   start   /   restart
pause