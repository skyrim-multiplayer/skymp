@echo off
setlocal

:: ============================================================
::  Starts all SkyRP services: nginx, backend, game server.
::  Shortcut-friendly: elevates itself, shows status, then closes.
:: ============================================================

net session >nul 2>&1
if errorlevel 1 (
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

set "NSSM=C:\tools\nssm\nssm.exe"
if not exist "%NSSM%" set "NSSM=nssm"

echo === Starting SkyRP services ===
for %%S in (SkyrpNginx SkyrpBackend SkyrpGameServer) do (
    echo.
    echo -- %%S
    "%NSSM%" start %%S 2>&1
)

echo.
echo === Status ===
for %%S in (SkyrpNginx SkyrpBackend SkyrpGameServer) do (
    <nul set /p="%%S: "
    "%NSSM%" status %%S 2>&1
)
echo.
echo Listening ports (expect 443 web, 4000 api, 4002 dashboard, 7777 game, 7778 relay):
netstat -an | findstr /c:":443 " /c:":4000 " /c:":4002 " /c:":7777 " /c:":7778 " | findstr /c:"LISTENING" /c:"*:*"
echo.
echo If a service failed: install it first (Setup-Nginx.bat, Setup-Backend.bat,
echo or the game server setup bat in build\dist\server). Logs are in C:\logs.
echo.
echo Closing in 15 seconds (or press a key)...
timeout /t 15 >nul
