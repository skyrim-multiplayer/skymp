@echo off
setlocal

:: ============================================================
::  Restarts all SkyMP services: stops game server, backend and
::  nginx, then starts them back up in dependency order.
::  Shortcut-friendly: elevates itself, shows status, then closes.
:: ============================================================

net session >nul 2>&1
if errorlevel 1 (
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

set "NSSM=C:\tools\nssm\nssm.exe"
if not exist "%NSSM%" set "NSSM=nssm"

echo === Stopping SkyMP services ===
for %%S in (SkympGameServer SkympBackend SkympNginx) do (
    echo -- %%S
    "%NSSM%" stop %%S 2>&1
)
taskkill /f /im nginx.exe >nul 2>&1

echo.
echo Waiting 3 seconds...
timeout /t 3 /nobreak >nul

echo.
echo === Starting SkyMP services ===
for %%S in (SkympNginx SkympBackend SkympGameServer) do (
    echo -- %%S
    "%NSSM%" start %%S 2>&1
)

echo.
echo === Status ===
for %%S in (SkympNginx SkympBackend SkympGameServer) do (
    <nul set /p="%%S: "
    "%NSSM%" status %%S 2>&1
)
echo.
echo Listening ports (expect 443 web, 4000 api, 4002 dashboard, 7777 game, 7778 relay):
netstat -an | findstr /c:":443 " /c:":4000 " /c:":4002 " /c:":7777 " /c:":7778 " | findstr /c:"LISTENING" /c:"*:*"
echo.
echo Closing in 15 seconds (or press a key)...
timeout /t 15 >nul
