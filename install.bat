@echo off
setlocal enabledelayedexpansion
title SkyMP Server Installer

echo ============================================
echo  SkyMP Server Installer fuer Windows
echo ============================================
echo.

:: ── Voraussetzungen pruefen ──────────────────

where git >nul 2>&1
if errorlevel 1 (
    echo [FEHLER] Git nicht gefunden. Bitte installieren: https://git-scm.com
    pause & exit /b 1
)

where cmake >nul 2>&1
if errorlevel 1 (
    echo [FEHLER] CMake nicht gefunden. Bitte installieren: https://cmake.org/download
    pause & exit /b 1
)

where node >nul 2>&1
if errorlevel 1 (
    echo [FEHLER] Node.js nicht gefunden. Bitte installieren: https://nodejs.org
    pause & exit /b 1
)

where yarn >nul 2>&1
if errorlevel 1 (
    echo [INFO] Yarn nicht gefunden, wird installiert...
    npm install -g yarn
)

echo [OK] Alle Voraussetzungen gefunden.
echo.

:: ── Installationsverzeichnis ─────────────────

set INSTALL_DIR=E:\Github\skymp
echo Installationsverzeichnis: %INSTALL_DIR%
echo.

:: ── Skyrim-Pfad abfragen ─────────────────────

set DEFAULT_SKYRIM=C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition
set /p SKYRIM_DIR="Skyrim SE Pfad (Enter fuer Standard: %DEFAULT_SKYRIM%): "
if "!SKYRIM_DIR!"=="" set SKYRIM_DIR=%DEFAULT_SKYRIM%

if not exist "!SKYRIM_DIR!\SkyrimSE.exe" (
    echo [WARNUNG] SkyrimSE.exe nicht gefunden in: !SKYRIM_DIR!
    echo           Build wird ohne Skyrim-Pfad fortgesetzt.
    set SKYRIM_DIR=
)

echo.

:: ── In Fork-Verzeichnis wechseln ─────────────

echo [1/4] Wechsle in Fork-Verzeichnis...
if not exist "%INSTALL_DIR%" (
    echo [FEHLER] Verzeichnis nicht gefunden: %INSTALL_DIR%
    pause & exit /b 1
)

cd /d "%INSTALL_DIR%"

echo [1/4] Initialisiere Submodule...
git submodule init
git submodule update
if errorlevel 1 ( echo [FEHLER] Submodule fehlgeschlagen. & pause & exit /b 1 )

echo.

:: ── CMake konfigurieren ───────────────────────

echo [2/4] Konfiguriere CMake...
if not exist build mkdir build
cd build

if not "!SKYRIM_DIR!"=="" (
    cmake .. -DSKYRIM_DIR="!SKYRIM_DIR!" -DBUILD_UNIT_TESTS=OFF -DOFFLINE_MODE=ON
) else (
    cmake .. -DBUILD_UNIT_TESTS=OFF -DOFFLINE_MODE=ON
)

if errorlevel 1 ( echo [FEHLER] CMake Konfiguration fehlgeschlagen. & pause & exit /b 1 )

echo.

:: ── Build ────────────────────────────────────

echo [3/4] Baue Projekt (das kann 30-60 Minuten dauern)...
cmake --build . --config Release
if errorlevel 1 ( echo [FEHLER] Build fehlgeschlagen. & pause & exit /b 1 )

echo.

:: ── Server-Config erstellen ───────────────────

echo [4/4] Erstelle Standard-Server-Config...

set SERVER_DIR=%INSTALL_DIR%\build\dist\server

if not exist "%SERVER_DIR%\server-settings.json" (
    (
        echo {
        echo   "port": 7777,
        echo   "maxPlayers": 50,
        echo   "name": "Mein SkyMP Server",
        echo   "dataDir": "./data",
        echo   "gamemodePath": "./gamemode.js",
        echo   "savePath": "./save"
        echo }
    ) > "%SERVER_DIR%\server-settings.json"
    echo [OK] server-settings.json erstellt.
) else (
    echo [INFO] server-settings.json existiert bereits, unveraendert.
)

:: ── Start-Script erstellen ────────────────────

(
    echo @echo off
    echo title SkyMP Server
    echo cd /d "%SERVER_DIR%"
    echo node dist_back\skymp5-server.js
    echo pause
) > "%INSTALL_DIR%\start_server.bat"

echo.
echo ============================================
echo  Installation abgeschlossen!
echo ============================================
echo.
echo  Server-Verzeichnis : %SERVER_DIR%
echo  Config-Datei       : %SERVER_DIR%\server-settings.json
echo  Server starten     : %INSTALL_DIR%\start_server.bat
echo.
echo  Client-Dateien fuer Skyrim liegen in:
echo  %INSTALL_DIR%\build\dist\client\
echo  (Inhalt in deinen Skyrim-Ordner kopieren)
echo.
pause