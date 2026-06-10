@echo off
setlocal

:: ======EDIT THESE IF YOUR PATHS DIFFER======
set "SERVER_DIR=C:\Users\Administrator\Desktop\SkyMP\build\dist\server"
set "NODE_EXE=C:\Program Files\nodejs\node.exe"
set "LOG_DIR=C:\Users\Administrator\Desktop\logs"
set "NSSM_DIR=C:\tools\nssm"
set "BACKEND_DIR=C:\Users\Administrator\Desktop\SkyMP\skymp5-backend"
set "BACKEND_ENTRY=server.js"

:: Re-launch elevated if not running as Administrator
net session >nul 2>&1
if errorlevel 1 (
    echo Requesting administrator rights...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

echo.
echo === SkyRP service setup ===
echo.

:: Sanity checks
if not exist "%NODE_EXE%" (
    echo [ERROR] Node.js not found at: %NODE_EXE%
    echo Install Node.js LTS from https://nodejs.org and re-run.
    pause
    exit /b 1
)

if not exist "%SERVER_DIR%\dist_back\skymp5-server.js" (
    echo [ERROR] Game server not found at: %SERVER_DIR%\dist_back\skymp5-server.js
    echo Check the SERVER_DIR variable at the top of this script.
    pause
    exit /b 1
)

if not exist "%SERVER_DIR%\server-settings.json" (
    echo [WARNING] No server-settings.json in %SERVER_DIR%
    echo The server will not start correctly without it.
)

if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

:: Install NSSM if missing
set "NSSM="
where nssm >nul 2>&1
if not errorlevel 1 set "NSSM=nssm"
if not defined NSSM if exist "%NSSM_DIR%\nssm.exe" set "NSSM=%NSSM_DIR%\nssm.exe"

if not defined NSSM (
    echo Downloading NSSM 2.24...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://nssm.cc/release/nssm-2.24.zip' -OutFile \"$env:TEMP\nssm.zip\"; Expand-Archive -Path \"$env:TEMP\nssm.zip\" -DestinationPath \"$env:TEMP\nssm_extract\" -Force"
    if not exist "%TEMP%\nssm_extract\nssm-2.24\win64\nssm.exe" (
        echo [ERROR] NSSM download failed. Download https://nssm.cc/release/nssm-2.24.zip
        echo manually and place win64\nssm.exe into %NSSM_DIR%\nssm.exe, then re-run.
        pause
        exit /b 1
    )
    mkdir "%NSSM_DIR%" 2>nul
    copy /y "%TEMP%\nssm_extract\nssm-2.24\win64\nssm.exe" "%NSSM_DIR%\nssm.exe" >nul
    set "NSSM=%NSSM_DIR%\nssm.exe"
    echo NSSM installed to %NSSM_DIR%\nssm.exe
)

echo Using NSSM: %NSSM%
echo.

:: SkyMP server service
echo Configuring service: SkyrpGameServer
"%NSSM%" stop SkyrpGameServer >nul 2>&1
"%NSSM%" remove SkyrpGameServer confirm >nul 2>&1
"%NSSM%" install SkyrpGameServer "%NODE_EXE%" "dist_back\skymp5-server.js"
"%NSSM%" set SkyrpGameServer AppDirectory "%SERVER_DIR%"
"%NSSM%" set SkyrpGameServer AppStdout "%LOG_DIR%\gameserver.log"
"%NSSM%" set SkyrpGameServer AppStderr "%LOG_DIR%\gameserver-err.log"
"%NSSM%" set SkyrpGameServer AppRotateFiles 1
"%NSSM%" set SkyrpGameServer AppRotateBytes 10485760
"%NSSM%" set SkyrpGameServer Start SERVICE_AUTO_START
"%NSSM%" set SkyrpGameServer AppThrottle 5000

:: Backend service
if defined BACKEND_DIR (
    if exist "%BACKEND_DIR%\%BACKEND_ENTRY%" (
        if not exist "%BACKEND_DIR%\.env" (
            echo [WARNING] No .env file in %BACKEND_DIR% - the backend will not work without it.
        )
        if not exist "%BACKEND_DIR%\node_modules" (
            echo Installing backend dependencies - this may take a minute...
            pushd "%BACKEND_DIR%"
            call npm install
            popd
        )
        echo Configuring service: SkyrpBackend
        "%NSSM%" stop SkyrpBackend >nul 2>&1
        "%NSSM%" remove SkyrpBackend confirm >nul 2>&1
        "%NSSM%" install SkyrpBackend "%NODE_EXE%" "%BACKEND_ENTRY%"
        "%NSSM%" set SkyrpBackend AppDirectory "%BACKEND_DIR%"
        "%NSSM%" set SkyrpBackend AppStdout "%LOG_DIR%\backend.log"
        "%NSSM%" set SkyrpBackend AppStderr "%LOG_DIR%\backend-err.log"
        "%NSSM%" set SkyrpBackend AppRotateFiles 1
        "%NSSM%" set SkyrpBackend AppRotateBytes 10485760
        "%NSSM%" set SkyrpBackend Start SERVICE_AUTO_START
        "%NSSM%" set SkyrpBackend AppThrottle 5000
    ) else (
        echo [WARNING] Backend entry not found: %BACKEND_DIR%\%BACKEND_ENTRY% - skipping backend service.
    )
) else (
    echo Skipping backend service - BACKEND_DIR not set.
)

:: Firewall rules
echo.
echo Configuring firewall rules...
netsh advfirewall firewall delete rule name="SkyRP Game UDP 7777" >nul 2>&1
netsh advfirewall firewall add  rule name="SkyRP Game UDP 7777" dir=in action=allow protocol=UDP localport=7777
netsh advfirewall firewall delete rule name="SkyRP WS Relay TCP 7778" >nul 2>&1
netsh advfirewall firewall add  rule name="SkyRP WS Relay TCP 7778" dir=in action=allow protocol=TCP localport=7778
netsh advfirewall firewall delete rule name="SkyRP Web HTTP 80" >nul 2>&1
netsh advfirewall firewall add  rule name="SkyRP Web HTTP 80" dir=in action=allow protocol=TCP localport=80
netsh advfirewall firewall delete rule name="SkyRP Web HTTPS 443" >nul 2>&1
netsh advfirewall firewall add  rule name="SkyRP Web HTTPS 443" dir=in action=allow protocol=TCP localport=443

:: Start services
echo.
echo Starting SkyrpGameServer...
"%NSSM%" start SkyrpGameServer
if defined BACKEND_DIR if exist "%BACKEND_DIR%\%BACKEND_ENTRY%" (
    echo Starting SkyrpBackend...
    "%NSSM%" start SkyrpBackend
)

::Post summary
echo.
echo === Status ===
"%NSSM%" status SkyrpGameServer
if defined BACKEND_DIR if exist "%BACKEND_DIR%\%BACKEND_ENTRY%" "%NSSM%" status SkyrpBackend
echo.
echo Checking port 7777 (UDP) - a LISTENING/open line below means the server is up:
netstat -an | findstr ":7777"
echo.
echo Logs: %LOG_DIR%\gameserver.log and gameserver-err.log
echo If status is not SERVICE_RUNNING, check the -err.log file.
echo.
pause
