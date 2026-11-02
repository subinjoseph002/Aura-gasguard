@echo off
title Aura GasGuard - IoT Backend and Database Server
cd /d "%~dp0\backend"

echo =====================================================================
echo   Aura GasGuard - IoT REST Backend and SQLite Database Server
echo =====================================================================
echo.
echo [1] Checking Python environment...
python --version
if errorlevel 1 (
    echo.
    echo [ERROR] Python is not found in your PATH.
    pause
    exit /b 1
)

echo.
echo [2] Initializing SQLite Database (data/sensor_database.db)...
python -c "import database; database.init_db(); print('[OK] Database ready.')"

echo.
echo [3] Opening Real-Time Dashboard at http://localhost:5000...
start "" "http://localhost:5000"

echo.
echo [4] Starting Backend REST API Server on port 5000...
echo (Press Ctrl+C to stop the server)
echo =====================================================================
python app.py
pause
