@echo off
setlocal

rem Edit these to point at your reference install
set "MO2_ROOT=C:\MO2"
set "GAME_ROOT=C:\GOG Games\Skyrim Anniversary Edition"
set "PROFILE=Default"

rem Command-line overrides (optional): arg 1 = MO2 root, arg 2 = game root
if not "%~1"=="" set "MO2_ROOT=%~1"
if not "%~2"=="" set "GAME_ROOT=%~2"

rem Run from this script's folder so scripts\compile-manifest.js resolves.
cd /d "%~dp0"

if not exist "%MO2_ROOT%\mods" (
  echo ERROR: "%MO2_ROOT%" does not look like an MO2 install ^(no mods\ folder^).
  echo Edit MO2_ROOT in this file, or pass the path as the first argument.
  echo.
  pause
  exit /b 1
)

echo Compiling install manifest from "%MO2_ROOT%" ...
echo.

if exist "%GAME_ROOT%\SkyrimSE.exe" (
  node scripts\compile-manifest.js --mo2 "%MO2_ROOT%" --game "%GAME_ROOT%" --profile "%PROFILE%"
) else (
  echo NOTE: no SkyrimSE.exe at "%GAME_ROOT%" - compiling mods only ^(no game-root files^).
  echo.
  node scripts\compile-manifest.js --mo2 "%MO2_ROOT%" --profile "%PROFILE%"
)
set "RC=%ERRORLEVEL%"

echo.
if not "%RC%"=="0" (
  echo Manifest compilation FAILED ^(exit %RC%^).
) else (
  echo Done. data\install-manifest.json and data\modlist.json updated.
  echo Restart the backend to serve the new manifest.
)

echo.
pause
exit /b %RC%
