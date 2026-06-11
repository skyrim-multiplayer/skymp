@echo off
setlocal

:: ============================================================
::  Stops all SkyRP services: game server, backend, nginx
::  (reverse order of start, so players drop before the API does).
::  Shortcut-friendly: elevates itself, shows status, then closes.
:: ============================================================

net session >nul 2>&1
if errorlevel 1 (
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

set "NSSM=C:\tools\nssm\nssm.exe"
if not exist "%NSSM%" set "NSSM=nssm"

echo === Stopping SkyRP services ===
for %%S in (SkyrpGameServer SkyrpBackend SkyrpNginx) do (
    echo.
    echo -- %%S
    "%NSSM%" stop %%S 2>&1
)

:: nginx workers sometimes outlive the service process
taskkill /f /im nginx.exe >nul 2>&1

echo.
echo === Status ===
for %%S in (SkyrpGameServer SkyrpBackend SkyrpNginx) do (
    <nul set /p="%%S: "
    "%NSSM%" status %%S 2>&1
)
echo.
echo NOTE: services are set to auto-start, so they come back after a
echo reboot even when stopped here.
echo.
echo Closing in 15 seconds (or press a key)...
timeout /t 15 >nul
