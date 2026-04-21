cd /d "%~dp0"

echo Building skymp5-front...

npm run build
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)