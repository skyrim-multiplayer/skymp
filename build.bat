@echo off
setlocal enabledelayedexpansion
title SkyMP Build Tool

:: ── Defaults ──────────────────────────────────────────────────────────────────
set BUILD_CONFIG=Release
set SKYRIM_DIR=
set BUILD_UNIT_TESTS=OFF
set OFFLINE_MODE=ON
set BUILD_CLIENT=ON
set BUILD_SKYRIM_PLATFORM=ON
set BUILD_SCRIPTS=ON
set BUILD_GAMEMODE=OFF
set BUILD_FRONT=OFF
set BUILD_NODEJS=OFF
set SKYRIM_VR=OFF
set PREPARE_NEXUS_ARCHIVES=OFF
set INSTALL_CLIENT_DIST=OFF
set NO_CLEAN_AFTER_BUILD=OFF
set DOWNLOAD_SKYRIM_DATA=OFF
set UNIT_DATA_DIR=
set CPPCOV_PATH=
set PARALLEL_JOBS=
set SKIP_CONFIGURE=0
set CLEAN_BUILD=0
set USE_NINJA=OFF

:MAIN_MENU
cls
echo ================================================================================
echo   SkyMP Build Tool
echo ================================================================================
echo.
echo   [1]  Quick Presets
echo   [2]  Configure Options
echo   [3]  Set Skyrim Directory   : !SKYRIM_DIR!
echo   [4]  Build Config           : !BUILD_CONFIG!
echo   [5]  Parallel Jobs          : !PARALLEL_JOBS!
echo   [6]  Toggle: Skip CMake configure (use existing cache)  [!SKIP_CONFIGURE!]
echo   [7]  Toggle: Clean build directory before build         [!CLEAN_BUILD!]
echo   [8]  Show current settings
echo   [9]  Run Build
echo   [C]  Clean node_modules (all subdirectories)
echo   [V]  Clean vcpkg (git clean -xfd)
echo   [E]  Print environment
echo   [0]  Exit
echo.
set /p CHOICE="  Choice: "

if "!CHOICE!"=="1" goto PRESETS
if "!CHOICE!"=="2" goto OPTIONS_MENU
if "!CHOICE!"=="3" goto SET_SKYRIM_DIR
if "!CHOICE!"=="4" goto TOGGLE_CONFIG
if "!CHOICE!"=="5" goto SET_JOBS
if "!CHOICE!"=="6" goto TOGGLE_SKIP_CONFIGURE
if "!CHOICE!"=="7" goto TOGGLE_CLEAN
if "!CHOICE!"=="8" goto SHOW_SETTINGS
if "!CHOICE!"=="9" goto RUN_BUILD
if /i "!CHOICE!"=="C" goto CLEAN_NODE
if /i "!CHOICE!"=="V" goto CLEAN_VCPKG
if /i "!CHOICE!"=="E" goto PRINT_ENV
if "!CHOICE!"=="0" exit /b 0
goto MAIN_MENU

:: ── Presets ───────────────────────────────────────────────────────────────────
:PRESETS
cls
echo ================================================================================
echo   Quick Presets
echo ================================================================================
echo.
echo   [1]  Server Only    (no client, no scripts, no tests)
echo   [2]  Client Only    (no server-side, no tests)
echo   [3]  Full Build     (everything except Nexus archives)
echo   [4]  Dev / Fast     (no tests, no scripts, no front, no gamemode)
echo   [5]  Tests          (unit tests enabled)
echo   [B]  Back
echo.
set /p PRESET="  Choice: "

if /i "!PRESET!"=="1" (
    set BUILD_CLIENT=OFF
    set BUILD_SKYRIM_PLATFORM=OFF
    set BUILD_SCRIPTS=OFF
    set BUILD_GAMEMODE=OFF
    set BUILD_FRONT=OFF
    set BUILD_UNIT_TESTS=OFF
    echo   [Preset] Server Only applied.
    timeout /t 1 >nul
    goto MAIN_MENU
)
if /i "!PRESET!"=="2" (
    set BUILD_CLIENT=ON
    set BUILD_SKYRIM_PLATFORM=ON
    set BUILD_SCRIPTS=ON
    set BUILD_GAMEMODE=OFF
    set BUILD_FRONT=OFF
    set BUILD_UNIT_TESTS=OFF
    echo   [Preset] Client Only applied.
    timeout /t 1 >nul
    goto MAIN_MENU
)
if /i "!PRESET!"=="3" (
    set BUILD_CLIENT=ON
    set BUILD_SKYRIM_PLATFORM=ON
    set BUILD_SCRIPTS=ON
    set BUILD_GAMEMODE=ON
    set BUILD_FRONT=ON
    set BUILD_UNIT_TESTS=ON
    echo   [Preset] Full Build applied.
    timeout /t 1 >nul
    goto MAIN_MENU
)
if /i "!PRESET!"=="4" (
    set BUILD_CLIENT=ON
    set BUILD_SKYRIM_PLATFORM=ON
    set BUILD_SCRIPTS=OFF
    set BUILD_GAMEMODE=OFF
    set BUILD_FRONT=OFF
    set BUILD_UNIT_TESTS=OFF
    echo   [Preset] Dev / Fast applied.
    timeout /t 1 >nul
    goto MAIN_MENU
)
if /i "!PRESET!"=="5" (
    set BUILD_UNIT_TESTS=ON
    echo   [Preset] Tests enabled.
    timeout /t 1 >nul
    goto MAIN_MENU
)
if /i "!PRESET!"=="B" goto MAIN_MENU
goto PRESETS

:: ── Options Menu ──────────────────────────────────────────────────────────────
:OPTIONS_MENU
cls
echo ================================================================================
echo   Configure Options  (ON/OFF toggles)
echo ================================================================================
echo.
echo   [1]  BUILD_UNIT_TESTS        : !BUILD_UNIT_TESTS!
echo   [2]  OFFLINE_MODE            : !OFFLINE_MODE!
echo   [3]  BUILD_CLIENT            : !BUILD_CLIENT!
echo   [4]  BUILD_SKYRIM_PLATFORM   : !BUILD_SKYRIM_PLATFORM!
echo   [5]  BUILD_SCRIPTS           : !BUILD_SCRIPTS!
echo   [6]  BUILD_GAMEMODE          : !BUILD_GAMEMODE!
echo   [U]  USE_NINJA               : !USE_NINJA!
echo   [7]  BUILD_FRONT             : !BUILD_FRONT!
echo   [8]  BUILD_NODEJS            : !BUILD_NODEJS!
echo   [9]  SKYRIM_VR               : !SKYRIM_VR!
echo   [A]  PREPARE_NEXUS_ARCHIVES  : !PREPARE_NEXUS_ARCHIVES!
echo   [C]  INSTALL_CLIENT_DIST     : !INSTALL_CLIENT_DIST!
echo   [D]  NO_CLEAN_AFTER_BUILD    : !NO_CLEAN_AFTER_BUILD!
echo   [E]  DOWNLOAD_SKYRIM_DATA    : !DOWNLOAD_SKYRIM_DATA!
echo   [F]  UNIT_DATA_DIR           : !UNIT_DATA_DIR!
echo   [G]  CPPCOV_PATH             : !CPPCOV_PATH!
echo   [B]  Back
echo.
set /p OPT="  Toggle/set option: "

if "!OPT!"=="1" call :TOGGLE BUILD_UNIT_TESTS & goto OPTIONS_MENU
if "!OPT!"=="2" call :TOGGLE OFFLINE_MODE & goto OPTIONS_MENU
if "!OPT!"=="3" call :TOGGLE BUILD_CLIENT & goto OPTIONS_MENU
if "!OPT!"=="4" call :TOGGLE BUILD_SKYRIM_PLATFORM & goto OPTIONS_MENU
if "!OPT!"=="5" call :TOGGLE BUILD_SCRIPTS & goto OPTIONS_MENU
if "!OPT!"=="6" call :TOGGLE BUILD_GAMEMODE & goto OPTIONS_MENU
if "!OPT!"=="7" call :TOGGLE BUILD_FRONT & goto OPTIONS_MENU
if "!OPT!"=="8" call :TOGGLE BUILD_NODEJS & goto OPTIONS_MENU
if "!OPT!"=="9" call :TOGGLE SKYRIM_VR & goto OPTIONS_MENU
if /i "!OPT!"=="A" call :TOGGLE PREPARE_NEXUS_ARCHIVES & goto OPTIONS_MENU
if /i "!OPT!"=="C" call :TOGGLE INSTALL_CLIENT_DIST & goto OPTIONS_MENU
if /i "!OPT!"=="D" call :TOGGLE NO_CLEAN_AFTER_BUILD & goto OPTIONS_MENU
if /i "!OPT!"=="E" call :TOGGLE DOWNLOAD_SKYRIM_DATA & goto OPTIONS_MENU
if /i "!OPT!"=="F" goto SET_UNIT_DATA_DIR
if /i "!OPT!"=="G" goto SET_CPPCOV_PATH
if /i "!OPT!"=="U" call :TOGGLE USE_NINJA & goto OPTIONS_MENU
if /i "!OPT!"=="B" goto MAIN_MENU
goto OPTIONS_MENU

:SET_UNIT_DATA_DIR
set /p UNIT_DATA_DIR="  UNIT_DATA_DIR path (leave blank to clear): "
goto OPTIONS_MENU

:SET_CPPCOV_PATH
set /p CPPCOV_PATH="  CPPCOV_PATH (leave blank to clear): "
goto OPTIONS_MENU

:: ── Skyrim Dir ────────────────────────────────────────────────────────────────
:SET_SKYRIM_DIR
cls
echo   Current: !SKYRIM_DIR!
echo   Leave blank to clear / skip Skyrim dir.
set /p SKYRIM_DIR="  Skyrim SE path: "
if "!SKYRIM_DIR!" NEQ "" (
    if not exist "!SKYRIM_DIR!\SkyrimSE.exe" (
        echo   [WARN] SkyrimSE.exe not found at that path. Clearing.
        set SKYRIM_DIR=
        pause
    )
)
goto MAIN_MENU

:: ── Build Config ──────────────────────────────────────────────────────────────
:TOGGLE_CONFIG
if "!BUILD_CONFIG!"=="Release" (set BUILD_CONFIG=Debug) else (set BUILD_CONFIG=Release)
goto MAIN_MENU

:: ── Parallel Jobs ─────────────────────────────────────────────────────────────
:SET_JOBS
set /p PARALLEL_JOBS="  Number of parallel jobs (leave blank for default): "
goto MAIN_MENU

:: ── Skip Configure ────────────────────────────────────────────────────────────
:TOGGLE_SKIP_CONFIGURE
if "!SKIP_CONFIGURE!"=="0" (set SKIP_CONFIGURE=1) else (set SKIP_CONFIGURE=0)
goto MAIN_MENU

:: ── Clean ─────────────────────────────────────────────────────────────────────
:TOGGLE_CLEAN
if "!CLEAN_BUILD!"=="0" (set CLEAN_BUILD=1) else (set CLEAN_BUILD=0)
goto MAIN_MENU

:: ── Show Settings ─────────────────────────────────────────────────────────────
:SHOW_SETTINGS
cls
echo ================================================================================
echo   Current Build Settings
echo ================================================================================
echo.
echo   Build config           : !BUILD_CONFIG!
echo   Parallel jobs          : !PARALLEL_JOBS!
echo   Skip CMake configure   : !SKIP_CONFIGURE!
echo   Clean before build     : !CLEAN_BUILD!
echo.
echo   SKYRIM_DIR             : !SKYRIM_DIR!
echo   UNIT_DATA_DIR          : !UNIT_DATA_DIR!
echo   CPPCOV_PATH            : !CPPCOV_PATH!
echo.
echo   BUILD_CLIENT           : !BUILD_CLIENT!
echo   BUILD_SKYRIM_PLATFORM  : !BUILD_SKYRIM_PLATFORM!
echo   BUILD_SCRIPTS          : !BUILD_SCRIPTS!
echo   BUILD_GAMEMODE         : !BUILD_GAMEMODE!
echo   BUILD_FRONT            : !BUILD_FRONT!
echo   BUILD_NODEJS           : !BUILD_NODEJS!
echo   BUILD_UNIT_TESTS       : !BUILD_UNIT_TESTS!
echo   OFFLINE_MODE           : !OFFLINE_MODE!
echo   SKYRIM_VR              : !SKYRIM_VR!
echo   PREPARE_NEXUS_ARCHIVES : !PREPARE_NEXUS_ARCHIVES!
echo   INSTALL_CLIENT_DIST    : !INSTALL_CLIENT_DIST!
echo   NO_CLEAN_AFTER_BUILD   : !NO_CLEAN_AFTER_BUILD!
echo   DOWNLOAD_SKYRIM_DATA   : !DOWNLOAD_SKYRIM_DATA!
echo.
pause
goto MAIN_MENU

:: ── Run Build ─────────────────────────────────────────────────────────────────
:RUN_BUILD
cls
echo ================================================================================
echo   Running Build
echo ================================================================================
echo.

:: Check cmake
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake not found in PATH.
    pause & goto MAIN_MENU
)

:: Root dir is where this batch lives
set ROOT_DIR=%~dp0
set ROOT_DIR=!ROOT_DIR:~0,-1!
set BUILD_DIR=!ROOT_DIR!\build

:: Optionally clean
if "!CLEAN_BUILD!"=="1" (
    echo [CLEAN] Removing build directory...
    if exist "!BUILD_DIR!" rmdir /s /q "!BUILD_DIR!"
)

:: Create build dir
if not exist "!BUILD_DIR!" mkdir "!BUILD_DIR!"

:: CMake configure
if "!SKIP_CONFIGURE!"=="0" (
    echo [1/2] Configuring with CMake...
    echo.

    set CMAKE_ARGS=-DBUILD_UNIT_TESTS=!BUILD_UNIT_TESTS!
    set CMAKE_ARGS=!CMAKE_ARGS! -DOFFLINE_MODE=!OFFLINE_MODE!
    set CMAKE_ARGS=!CMAKE_ARGS! -DBUILD_CLIENT=!BUILD_CLIENT!
    set CMAKE_ARGS=!CMAKE_ARGS! -DBUILD_SKYRIM_PLATFORM=!BUILD_SKYRIM_PLATFORM!
    set CMAKE_ARGS=!CMAKE_ARGS! -DBUILD_SCRIPTS=!BUILD_SCRIPTS!
    set CMAKE_ARGS=!CMAKE_ARGS! -DBUILD_GAMEMODE=!BUILD_GAMEMODE!
    set CMAKE_ARGS=!CMAKE_ARGS! -DBUILD_FRONT=!BUILD_FRONT!
    set CMAKE_ARGS=!CMAKE_ARGS! -DBUILD_NODEJS=!BUILD_NODEJS!
    set CMAKE_ARGS=!CMAKE_ARGS! -DSKYRIM_VR=!SKYRIM_VR!
    set CMAKE_ARGS=!CMAKE_ARGS! -DPREPARE_NEXUS_ARCHIVES=!PREPARE_NEXUS_ARCHIVES!
    set CMAKE_ARGS=!CMAKE_ARGS! -DINSTALL_CLIENT_DIST=!INSTALL_CLIENT_DIST!
    set CMAKE_ARGS=!CMAKE_ARGS! -DNO_CLEAN_AFTER_BUILD=!NO_CLEAN_AFTER_BUILD!
    set CMAKE_ARGS=!CMAKE_ARGS! -DDOWNLOAD_SKYRIM_DATA=!DOWNLOAD_SKYRIM_DATA!

    if "!SKYRIM_DIR!" NEQ "" (
        set CMAKE_ARGS=!CMAKE_ARGS! -DSKYRIM_DIR="!SKYRIM_DIR!"
    )
    if "!UNIT_DATA_DIR!" NEQ "" (
        set CMAKE_ARGS=!CMAKE_ARGS! -DUNIT_DATA_DIR="!UNIT_DATA_DIR!"
    )
    if "!CPPCOV_PATH!" NEQ "" (
        set CMAKE_ARGS=!CMAKE_ARGS! -DCPPCOV_PATH="!CPPCOV_PATH!"
    )

    set GENERATOR_ARGS=
    if "!USE_NINJA!"=="ON" (
        where ninja >nul 2>&1
        if errorlevel 1 (
            echo [WARN] ninja not found in PATH, falling back to default generator.
        ) else (
            set GENERATOR_ARGS=-G "Ninja"
        )
    )

    echo   cmake "!ROOT_DIR!" !GENERATOR_ARGS! !CMAKE_ARGS! -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    echo.
    cmake "!ROOT_DIR!" !GENERATOR_ARGS! !CMAKE_ARGS! -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B "!BUILD_DIR!"
    if errorlevel 1 (
        echo.
        echo [ERROR] CMake configuration failed.
        pause & goto MAIN_MENU
    )
    echo.
) else (
    echo [SKIP] CMake configure skipped ^(using existing cache^).
    echo.
)

:: Build
echo [2/2] Building ^(!BUILD_CONFIG!^)...
echo.

set BUILD_CMD=cmake --build "!BUILD_DIR!" --config !BUILD_CONFIG!
if "!PARALLEL_JOBS!" NEQ "" (
    set BUILD_CMD=!BUILD_CMD! --parallel !PARALLEL_JOBS!
)

echo   !BUILD_CMD!
echo.
!BUILD_CMD!

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed.
    pause & goto MAIN_MENU
)

echo.
echo ================================================================================
echo   Build completed successfully  ^(!BUILD_CONFIG!^)
echo   Output: !BUILD_DIR!\dist
echo ================================================================================
echo.
pause
goto MAIN_MENU

:: ── Clean node_modules ────────────────────────────────────────────────────────
:CLEAN_NODE
cls
echo ================================================================================
echo   Clean node_modules
echo ================================================================================
echo.
echo   This will DELETE all node_modules directories under:
echo   !ROOT_DIR!
echo.
echo   Press Ctrl-C to abort, or
set /p CONFIRM="  press Enter to proceed: "
echo.
echo   Searching for node_modules...
for /d /r "!ROOT_DIR!" %%D in (node_modules) do (
    if exist "%%D" (
        echo   Removing: %%D
        rmdir /s /q "%%D"
    )
)
echo.
echo   Done.
pause
goto MAIN_MENU

:: ── Clean vcpkg ───────────────────────────────────────────────────────────────
:CLEAN_VCPKG
cls
echo ================================================================================
echo   Clean vcpkg  (git clean -xfd)
echo ================================================================================
echo.
set VCPKG_DIR=!ROOT_DIR!\vcpkg
if not exist "!VCPKG_DIR!" (
    echo   [ERROR] vcpkg directory not found: !VCPKG_DIR!
    pause & goto MAIN_MENU
)
echo   Directory: !VCPKG_DIR!
echo.
echo   Dry run preview:
git -C "!VCPKG_DIR!" clean -xfd --dry-run
echo.
echo   Press Ctrl-C to abort, or
set /p CONFIRM="  press Enter to proceed with git clean -xfd: "
git -C "!VCPKG_DIR!" clean -xfd
echo.
pause
goto MAIN_MENU

:: ── Print environment ─────────────────────────────────────────────────────────
:PRINT_ENV
cls
echo ================================================================================
echo   Environment Variables
echo ================================================================================
echo.
set
echo.
pause
goto MAIN_MENU

:: ── Helper: toggle ON/OFF ──────────────────────────────────────────────────────
:TOGGLE
if "!%1!"=="ON" (set %1=OFF) else (set %1=ON)
exit /b 0
