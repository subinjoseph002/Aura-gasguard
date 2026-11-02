@echo off
title Aura GasGuard - Docker Container Launcher
cd /d "%~dp0"

echo =====================================================================
echo   Aura GasGuard - Containerized IoT Backend and SQLite Database
echo =====================================================================
echo.

:: Ensure Docker bin directory is in PATH
set "PATH=%PATH%;%LOCALAPPDATA%\Programs\DockerDesktop\resources\bin"

echo [1] Checking Docker CLI...
docker --version
if errorlevel 1 goto DOCKER_NOT_FOUND

echo.
echo [2] Checking if Docker Engine daemon is running...
docker info >nul 2>&1
if not errorlevel 1 goto DOCKER_RUNNING

echo.
echo [*] Docker Desktop Engine is not running yet.
echo [*] Launching Docker Desktop application...
start "" "%LOCALAPPDATA%\Programs\DockerDesktop\Docker Desktop.exe"
echo [*] Waiting for Docker Engine to start...
echo [*] NOTE: If a popup appears in Docker Desktop, please click 'Accept' / 'Continue'.
echo.

set attempt=0

:WAIT_LOOP
set /a attempt=attempt+1
if "%attempt%"=="25" goto DOCKER_TIMEOUT

echo     Waiting for Docker Engine to initialize [attempt %attempt%/25]...
ping -n 4 127.0.0.1 >nul
docker info >nul 2>&1
if errorlevel 1 goto WAIT_LOOP

:DOCKER_RUNNING
echo [*] Docker Engine is ONLINE!
echo.
echo [3] Building and starting containerized backend via Docker Compose...
docker-compose up -d --build
if not errorlevel 1 goto COMPOSE_SUCCESS

echo.
echo [*] Trying alternative 'docker compose' command...
docker compose up -d --build
if not errorlevel 1 goto COMPOSE_SUCCESS

goto COMPOSE_ERROR

:COMPOSE_SUCCESS
echo.
echo =====================================================================
echo   [SUCCESS] Aura GasGuard Backend Container is RUNNING!
echo =====================================================================
echo   - Container Name : gasguard-iot-backend
echo   - REST API Port  : http://localhost:5000
echo   - Dashboard URL  : http://localhost:5000
echo   - Database File  : ./backend/data/sensor_database.db (Persisted)
echo =====================================================================
echo.
echo Opening dashboard in your default browser...
start "" "http://localhost:5000"

echo.
echo Current running containers:
docker ps --filter "name=gasguard-iot-backend"

echo.
echo Backend is running in background. You can close this window at any time.
pause
exit /b 0

:DOCKER_NOT_FOUND
echo.
echo [ERROR] Docker CLI not found.
echo Please verify that Docker Desktop is installed.
pause
exit /b 1

:DOCKER_TIMEOUT
echo.
echo [NOTE] Docker Engine is taking longer than expected to start up.
echo Please check your screen or system tray (near clock) to see if Docker Desktop
echo is asking to Accept Terms or finish initialization.
echo Once Docker is running (whale icon steady), double-click this file again!
echo.
echo Alternatively, you can run the backend natively using: run_backend.bat
pause
exit /b 1

:COMPOSE_ERROR
echo.
echo [ERROR] Failed to start container with Docker Compose.
echo You can run the backend natively without Docker using: run_backend.bat
pause
exit /b 1
