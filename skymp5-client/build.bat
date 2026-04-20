@echo off
cd /d "%~dp0"

echo Building skymp5-client...
npm run build
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo Copying to Frostfall-Client...
copy /Y "..\build\dist\client\Data\Platform\Plugins\skymp5-client.js" "..\..\Frostfall-Client\Data\Platform\Plugins\skymp5-client.js"
if errorlevel 1 (
    echo Copy failed.
    exit /b 1
)

echo Done.
