@echo off
setlocal

cd /d "%~dp0"

> config.js echo module.exports = {
>> config.js echo   outputPath: '../build/dist/client/Data/Platform/UI',
>> config.js echo };

where yarn >nul 2>&1
if %errorlevel%==0 (set "USE_YARN=1") else (set "USE_YARN=")

if not exist node_modules (
  echo Installing front-end dependencies ^(first run, this can take a minute^)...
  if defined USE_YARN ( call yarn install --frozen-lockfile ) else ( call npm install --legacy-peer-deps )
  if errorlevel 1 (
    echo.
    echo Dependency install failed - make sure Node.js and yarn ^(or npm^) are installed and on PATH.
    pause
    exit /b 1
  )
)

echo Building front-end...
if defined USE_YARN ( call yarn build ) else ( call npm run build )
if errorlevel 1 (
  echo.
  echo Front-end build failed.
  pause
  exit /b 1
)

echo.
echo Done. UI written to build\dist\client\Data\Platform\UI
echo Next: run skymp5-backend\build-client.bat to package and publish it.
pause
