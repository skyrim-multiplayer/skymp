@echo off
setlocal

:: ============================================================
::  Regenerates data\modlist.json from the Nexus collection.
::  Run after publishing a new collection revision.
::
::  The backend serves modlist.json fresh on every request, so
::  changes apply immediately - no service restart needed.
:: ============================================================

:: ===== EDIT THESE PER RELEASE =====
set "SLUG=lciswb"
set "REVISION=1"
:: ==================================

set "BACKEND_DIR=%~dp0"
if "%BACKEND_DIR:~-1%"=="\" set "BACKEND_DIR=%BACKEND_DIR:~0,-1%"

where node >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Node.js was not found on PATH.
    pause
    exit /b 1
)

:: API key: use NEXUS_API_KEY if set, otherwise ask once
if defined NEXUS_API_KEY (
    set "KEY=%NEXUS_API_KEY%"
) else (
    set /p KEY="Paste your Nexus API key: "
)
if not defined KEY (
    echo [ERROR] No API key provided.
    pause
    exit /b 1
)

echo.
echo Generating modlist from collection %SLUG% revision %REVISION%...
echo.

pushd "%BACKEND_DIR%"
node scripts\build-mostlist.js %SLUG% %REVISION% %KEY%
set "RESULT=%ERRORLEVEL%"
popd

echo.
if "%RESULT%"=="0" (
    echo Done - the launcher picks the new modlist up immediately.
) else (
    echo [ERROR] Generation failed - see output above.
)
pause
