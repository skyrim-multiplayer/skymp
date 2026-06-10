@echo off
echo Installing dependencies...
npm install

echo Downloading NSSM...
curl -L https://nssm.cc/release/nssm-2.24.zip -o nssm.zip

echo Extracting NSSM...
powershell -command "Expand-Archive -Path nssm.zip -DestinationPath nssm-extracted -Force"

echo Installing SkyRP Backend as a Windows Service...
nssm-extracted\nssm-2.24\win64\nssm.exe install SkyRP-Backend node "%CD%\server.js"
nssm-extracted\nssm-2.24\win64\nssm.exe set SkyRP-Backend AppDirectory "%CD%"
nssm-extracted\nssm-2.24\win64\nssm.exe set SkyRP-Backend AppEnvironmentExtra "PORT=4000"
nssm-extracted\nssm-2.24\win64\nssm.exe set SkyRP-Backend Start SERVICE_AUTO_START

echo Starting service...
nssm-extracted\nssm-2.24\win64\nssm.exe start SkyRP-Backend

echo.
echo Done! SkyRP-Backend is now running as a Windows Service.
echo You can manage it with:
echo   nssm start SkyRP-Backend
echo   nssm stop SkyRP-Backend
echo   nssm restart SkyRP-Backend
echo   nssm remove SkyRP-Backend
pause