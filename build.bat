@echo off
setlocal EnableExtensions EnableDelayedExpansion
title SkyMP Build Tool

set "ROOT_DIR=%~dp0"
set "ROOT_DIR=%ROOT_DIR:~0,-1%"
set "BUILD_DIR=%ROOT_DIR%\build"
set "BUILD_CONFIG=Release"
set "CMAKE_BUILD_UNIT_TESTS=OFF"
set "CMAKE_OFFLINE_MODE=ON"
set "CMAKE_BUILD_CLIENT=ON"
set "CMAKE_BUILD_SKYRIM_PLATFORM=ON"
set "CMAKE_BUILD_SCRIPTS=ON"
set "CMAKE_BUILD_GAMEMODE=ON"
set "CMAKE_BUILD_FRONT=OFF"
set "CMAKE_BUILD_NODEJS=OFF"

if "%~1"=="" goto MENU
if /i "%~1"=="help" goto HELP
if /i "%~1"=="-h" goto HELP
if /i "%~1"=="--help" goto HELP

for %%T in (%*) do (
  call :DISPATCH "%%~T"
  if errorlevel 1 exit /b !errorlevel!
)

echo.
echo [OK] Requested build target(s) completed.
exit /b 0

:MENU
cls
echo ================================================================================
echo   SkyMP Build Tool
echo ================================================================================
echo.
echo   [1]  All TypeScript / JavaScript bundles
echo   [2]  Server only
echo   [3]  Client only
echo   [4]  Client + Server
echo   [5]  Front UI only
echo   [6]  Chat module
echo   [7]  Gamemode
echo   [8]  Patcher workspace
echo   [9]  Apply chat patches
echo   [N]  Native / CMake build
echo   [H]  Help / command-line targets
echo   [0]  Exit
echo.
set /p "CHOICE=Choice: "

if "%CHOICE%"=="1" (call :DISPATCH ts-all & goto MENU_DONE)
if "%CHOICE%"=="2" (call :DISPATCH server & goto MENU_DONE)
if "%CHOICE%"=="3" (call :DISPATCH client & goto MENU_DONE)
if "%CHOICE%"=="4" (call :DISPATCH client-server & goto MENU_DONE)
if "%CHOICE%"=="5" (call :DISPATCH front & goto MENU_DONE)
if "%CHOICE%"=="6" (call :DISPATCH chat & goto MENU_DONE)
if "%CHOICE%"=="7" (call :DISPATCH gamemode & goto MENU_DONE)
if "%CHOICE%"=="8" (call :DISPATCH patcher & goto MENU_DONE)
if "%CHOICE%"=="9" (call :DISPATCH patches & goto MENU_DONE)
if /i "%CHOICE%"=="N" (call :DISPATCH native & goto MENU_DONE)
if /i "%CHOICE%"=="H" goto HELP_MENU
if "%CHOICE%"=="0" exit /b 0
goto MENU

:MENU_DONE
echo.
if errorlevel 1 (
  echo [ERROR] Build failed.
) else (
  echo [OK] Build completed.
)
echo.
pause
goto MENU

:HELP_MENU
call :PRINT_HELP
pause
goto MENU

:HELP
call :PRINT_HELP
exit /b 0

:PRINT_HELP
echo.
echo Usage:
echo   build.bat TARGET [TARGET ...]
echo.
echo Common targets:
echo   ts-all, all-ts       Build all TS/JS projects below, but do not apply patches
echo   server               Build skymp5-server TypeScript backend bundle
echo   client               Build skymp5-client Skyrim Platform plugin bundle
echo   client-server        Build client and server
echo   front, ui            Build skymp5-front browser UI bundle
echo   chat                 Type-check chat, deploy it to gamemode source, build gamemode
echo   gamemode             Build build/dist/server/gamemode.js
echo   patcher              Build skymp5-patcher core and CLI
echo   patches              Build patcher and apply skymp5-chat client patches
echo   native, cmake        Run a default CMake configure + build
echo.
echo Extra targets:
echo   chat-check           Type-check skymp5-chat only
echo   chat-deploy          Deploy skymp5-chat server chat file to gamemode source only
echo   patch-client         Apply skymp5-chat client patches only
echo.
echo Examples:
echo   build.bat chat
echo   build.bat client-server
echo   build.bat server gamemode
echo   build.bat ts-all
echo   build.bat patches
echo.
exit /b 0

:DISPATCH
set "TARGET=%~1"

if /i "%TARGET%"=="ts-all" goto DO_TSA
if /i "%TARGET%"=="all-ts" goto DO_TSA
if /i "%TARGET%"=="all" goto DO_TSA
if /i "%TARGET%"=="server" goto DO_SRV
if /i "%TARGET%"=="server-ts" goto DO_SRV
if /i "%TARGET%"=="client" goto DO_CLI
if /i "%TARGET%"=="client-ts" goto DO_CLI
if /i "%TARGET%"=="client-server" goto DO_CS
if /i "%TARGET%"=="server-client" goto DO_CS
if /i "%TARGET%"=="front" goto DO_FE
if /i "%TARGET%"=="ui" goto DO_FE
if /i "%TARGET%"=="frontend" goto DO_FE
if /i "%TARGET%"=="chat" goto DO_CHAT
if /i "%TARGET%"=="chat-check" goto DO_CHKC
if /i "%TARGET%"=="chat-deploy" goto DO_CHD
if /i "%TARGET%"=="gamemode" goto DO_GM
if /i "%TARGET%"=="gamemode-ts" goto DO_GM
if /i "%TARGET%"=="patcher" goto DO_PB
if /i "%TARGET%"=="patcher-build" goto DO_PB
if /i "%TARGET%"=="patches" goto DO_PAT
if /i "%TARGET%"=="patch-client" goto DO_PCLI
if /i "%TARGET%"=="native" goto DO_NAT
if /i "%TARGET%"=="cmake" goto DO_NAT

echo [ERROR] Unknown build target: %TARGET%
echo        Run build.bat help for available targets.
exit /b 1

:DO_TSA
call :DO_SRV || exit /b 1
call :DO_CLI || exit /b 1
call :DO_FE || exit /b 1
call :DO_GM || exit /b 1
call :DO_PB || exit /b 1
call :DO_CHAT || exit /b 1
exit /b 0

:DO_SRV
call :RUN_CMD "skymp5-server" "%ROOT_DIR%\skymp5-server" "npm.cmd run build-ts"
exit /b %errorlevel%

:DO_CLI
call :RUN_CMD "skymp5-client" "%ROOT_DIR%\skymp5-client" "npm.cmd run build"
exit /b %errorlevel%

:DO_CS
call :DO_CLI || exit /b 1
call :DO_SRV || exit /b 1
exit /b 0

:DO_FE
call :RUN_CMD "skymp5-front" "%ROOT_DIR%\skymp5-front" "npm.cmd run build"
exit /b %errorlevel%

:DO_GM
call :RUN_CMD "frostfall-gamemode" "%ROOT_DIR%\frostfall-gamemode" "npm.cmd run build"
exit /b %errorlevel%

:DO_CHAT
call :DO_CHKC || exit /b 1
call :DO_CHD || exit /b 1
call :DO_GM || exit /b 1
exit /b 0

:DO_CHKC
call :RUN_CMD "skymp5-chat check" "%ROOT_DIR%\skymp5-chat" "npm.cmd run check"
exit /b %errorlevel%

:DO_CHD
call :RUN_CMD "skymp5-chat deploy:server" "%ROOT_DIR%\skymp5-chat" "npm.cmd run deploy:server"
exit /b %errorlevel%

:DO_PB
call :RUN_CMD "skymp5-patcher" "%ROOT_DIR%\skymp5-patcher" "npm.cmd run build"
exit /b %errorlevel%

:DO_PAT
call :DO_PB || exit /b 1
call :DO_PCLI || exit /b 1
exit /b 0

:DO_PCLI
call :RUN_CMD "skymp5-chat patch:client" "%ROOT_DIR%\skymp5-chat" "npm.cmd run patch:client"
exit /b %errorlevel%

:DO_NAT
call :RUN_NATIVE
exit /b %errorlevel%

:RUN_CMD
set "STEP_NAME=%~1"
set "STEP_DIR=%~2"
set "STEP_CMD=%~3"

echo.
echo ================================================================================
echo [BUILD] %STEP_NAME%
echo [DIR]   %STEP_DIR%
echo [CMD]   %STEP_CMD%
echo ================================================================================

if not exist "%STEP_DIR%" (
  echo [ERROR] Directory not found: %STEP_DIR%
  exit /b 1
)

pushd "%STEP_DIR%" || exit /b 1
call %STEP_CMD%
set "STEP_RESULT=%errorlevel%"
popd

if not "%STEP_RESULT%"=="0" (
  echo [ERROR] %STEP_NAME% failed with exit code %STEP_RESULT%.
  exit /b %STEP_RESULT%
)

echo [OK] %STEP_NAME%
exit /b 0

:RUN_NATIVE
where cmake >nul 2>&1
if errorlevel 1 (
  echo [ERROR] cmake not found in PATH.
  exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo.
echo ================================================================================
echo [BUILD] Native / CMake
echo [DIR]   %BUILD_DIR%
echo ================================================================================
echo.

cmake "%ROOT_DIR%" ^
  -B "%BUILD_DIR%" ^
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
  -DBUILD_UNIT_TESTS=%CMAKE_BUILD_UNIT_TESTS% ^
  -DOFFLINE_MODE=%CMAKE_OFFLINE_MODE% ^
  -DBUILD_CLIENT=%CMAKE_BUILD_CLIENT% ^
  -DBUILD_SKYRIM_PLATFORM=%CMAKE_BUILD_SKYRIM_PLATFORM% ^
  -DBUILD_SCRIPTS=%CMAKE_BUILD_SCRIPTS% ^
  -DBUILD_GAMEMODE=%CMAKE_BUILD_GAMEMODE% ^
  -DBUILD_FRONT=%CMAKE_BUILD_FRONT% ^
  -DBUILD_NODEJS=%CMAKE_BUILD_NODEJS%
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config %BUILD_CONFIG%
if errorlevel 1 exit /b 1

echo [OK] Native / CMake
exit /b 0
