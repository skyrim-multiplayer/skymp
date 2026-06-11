@echo off
setlocal

:: ============================================================
::  SkyRP reverse-proxy installer. Double-click from the repo root.
::    1. Installs nginx to C:\nginx
::    2. Installs win-acme (Let's Encrypt client) to C:\tools\win-acme
::    3. Starts nginx on port 80 and obtains certificates for
::       api. and dashboard.skyrimroleplay.co.uk (auto-renewing)
::    4. Switches nginx to the full HTTPS proxy config
::       (api -> localhost:4000, dashboard -> localhost:4002)
::    5. Registers nginx as the SkyrpNginx Windows service
::  PREREQUISITE: DNS A records for both subdomains must point at
::  this machine, and ports 80/443 must be reachable from outside.
::  Safe to re-run; existing certificates are kept.
:: ============================================================

set "REPO_DIR=%~dp0"
if "%REPO_DIR:~-1%"=="\" set "REPO_DIR=%REPO_DIR:~0,-1%"
set "NGINX_DIR=C:\nginx"
set "NGINX_VER=1.28.0"
set "WACS_DIR=C:\tools\win-acme"
set "WACS_VER=2.2.9.1701"
set "NSSM_DIR=C:\tools\nssm"
set "EMAIL=smitty9542@gmail.com"
set "DOMAINS=api.skyrimroleplay.co.uk,dashboard.skyrimroleplay.co.uk"
set "CERT_NAME=api.skyrimroleplay.co.uk"
set "SERVICE=SkyrpNginx"

:: ---- Re-launch elevated if not running as Administrator ----
net session >nul 2>&1
if errorlevel 1 (
    echo Requesting administrator rights...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

echo.
echo === SkyRP nginx + win-acme setup ===
echo.

if not exist "%REPO_DIR%\deploy\nginx\nginx-full.conf" (
    echo [ERROR] deploy\nginx configs not found next to this script.
    echo Run this bat from the root of the skyrp repo.
    pause
    exit /b 1
)

:: ---- 1. nginx ----
if exist "%NGINX_DIR%\nginx.exe" (
    echo nginx already present at %NGINX_DIR%
) else (
    echo Downloading nginx %NGINX_VER%...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://nginx.org/download/nginx-%NGINX_VER%.zip' -OutFile \"$env:TEMP\nginx.zip\"; Expand-Archive -Path \"$env:TEMP\nginx.zip\" -DestinationPath \"$env:TEMP\nginx_extract\" -Force"
    if not exist "%TEMP%\nginx_extract\nginx-%NGINX_VER%\nginx.exe" (
        echo [ERROR] nginx download failed. Get https://nginx.org/download/nginx-%NGINX_VER%.zip
        echo manually and extract its contents to %NGINX_DIR%, then re-run.
        pause
        exit /b 1
    )
    xcopy /e /i /y "%TEMP%\nginx_extract\nginx-%NGINX_VER%" "%NGINX_DIR%" >nul
    echo nginx installed to %NGINX_DIR%
)
if not exist "%NGINX_DIR%\certs" mkdir "%NGINX_DIR%\certs"
if not exist "%NGINX_DIR%\html" mkdir "%NGINX_DIR%\html"

:: ---- 2. win-acme ----
if exist "%WACS_DIR%\wacs.exe" (
    echo win-acme already present at %WACS_DIR%
) else (
    echo Downloading win-acme %WACS_VER%...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://github.com/win-acme/win-acme/releases/download/v%WACS_VER%/win-acme.v%WACS_VER%.x64.pluggable.zip' -OutFile \"$env:TEMP\wacs.zip\"; Expand-Archive -Path \"$env:TEMP\wacs.zip\" -DestinationPath '%WACS_DIR%' -Force"
    if not exist "%WACS_DIR%\wacs.exe" (
        echo [ERROR] win-acme download failed. Get it from
        echo https://github.com/win-acme/win-acme/releases and extract to %WACS_DIR%
        pause
        exit /b 1
    )
    echo win-acme installed to %WACS_DIR%
)

:: ---- NSSM (shared with the other setup scripts) ----
set "NSSM="
where nssm >nul 2>&1
if not errorlevel 1 set "NSSM=nssm"
if not defined NSSM if exist "%NSSM_DIR%\nssm.exe" set "NSSM=%NSSM_DIR%\nssm.exe"
if not defined NSSM (
    echo Downloading NSSM 2.24...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://nssm.cc/release/nssm-2.24.zip' -OutFile \"$env:TEMP\nssm.zip\"; Expand-Archive -Path \"$env:TEMP\nssm.zip\" -DestinationPath \"$env:TEMP\nssm_extract\" -Force"
    mkdir "%NSSM_DIR%" 2>nul
    copy /y "%TEMP%\nssm_extract\nssm-2.24\win64\nssm.exe" "%NSSM_DIR%\nssm.exe" >nul
    set "NSSM=%NSSM_DIR%\nssm.exe"
)

:: ---- Firewall ----
netsh advfirewall firewall delete rule name="SkyRP Web HTTP 80" >nul 2>&1
netsh advfirewall firewall add  rule name="SkyRP Web HTTP 80" dir=in action=allow protocol=TCP localport=80 >nul
netsh advfirewall firewall delete rule name="SkyRP Web HTTPS 443" >nul 2>&1
netsh advfirewall firewall add  rule name="SkyRP Web HTTPS 443" dir=in action=allow protocol=TCP localport=443 >nul

:: ---- 3. Certificates ----
if exist "%NGINX_DIR%\certs\%CERT_NAME%-key.pem" (
    echo Certificates already present - skipping issuance.
    goto :full_config
)

echo Installing phase-1 config (HTTP only, for ACME validation)...
copy /y "%REPO_DIR%\deploy\nginx\nginx-http.conf" "%NGINX_DIR%\conf\nginx.conf" >nul

:: make sure no stray nginx is running, then register + start the service
"%NSSM%" stop %SERVICE% >nul 2>&1
"%NSSM%" remove %SERVICE% confirm >nul 2>&1
taskkill /f /im nginx.exe >nul 2>&1
"%NSSM%" install %SERVICE% "%NGINX_DIR%\nginx.exe"
"%NSSM%" set %SERVICE% AppDirectory "%NGINX_DIR%"
"%NSSM%" set %SERVICE% Start SERVICE_AUTO_START
"%NSSM%" start %SERVICE%
timeout /t 3 /nobreak >nul

echo Requesting certificates for %DOMAINS% ...
echo (Requires the DNS A records to point at this machine.)
"%WACS_DIR%\wacs.exe" --source manual --host "%DOMAINS%" --validation filesystem --webroot "%NGINX_DIR%\html" --store pemfiles --pemfilespath "%NGINX_DIR%\certs" --accepttermsofservice --emailaddress %EMAIL%
if not exist "%NGINX_DIR%\certs\%CERT_NAME%-key.pem" (
    echo.
    echo [ERROR] Certificate issuance failed - see win-acme output above.
    echo Common causes: DNS not pointing at this VPS, or port 80 blocked
    echo by the hosting provider. Fix and re-run this script.
    pause
    exit /b 1
)
echo Certificates stored in %NGINX_DIR%\certs (auto-renewal task registered).

:full_config
:: ---- 4. Full HTTPS config ----
echo Installing full proxy config...
copy /y "%REPO_DIR%\deploy\nginx\nginx-full.conf" "%NGINX_DIR%\conf\nginx.conf" >nul
"%NGINX_DIR%\nginx.exe" -p "%NGINX_DIR%" -t
if errorlevel 1 (
    echo [ERROR] nginx config test failed - see message above.
    pause
    exit /b 1
)

:: ---- 5. (Re)start the service so the new config is live ----
"%NSSM%" status %SERVICE% >nul 2>&1
if errorlevel 1 (
    "%NSSM%" install %SERVICE% "%NGINX_DIR%\nginx.exe"
    "%NSSM%" set %SERVICE% AppDirectory "%NGINX_DIR%"
    "%NSSM%" set %SERVICE% Start SERVICE_AUTO_START
)
"%NSSM%" stop %SERVICE% >nul 2>&1
taskkill /f /im nginx.exe >nul 2>&1
"%NSSM%" start %SERVICE%
timeout /t 3 /nobreak >nul

echo.
echo === Status ===
"%NSSM%" status %SERVICE%
netstat -an | findstr ":443" | findstr LISTENING >nul
if errorlevel 1 (
    echo [WARNING] Nothing is listening on 443. Check %NGINX_DIR%\logs\error.log
) else (
    echo nginx is listening on 443.
)
echo.
echo Test from this machine:
echo   curl https://api.skyrimroleplay.co.uk/api/status
echo (The backend service must be running for it to return JSON.)
echo.
pause