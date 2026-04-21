@echo off
cd /d "%~dp0"

echo Building skymp5-server...
npm run build-ts
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo Done.
pause
