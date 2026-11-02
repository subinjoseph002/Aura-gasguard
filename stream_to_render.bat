@echo off
title Aura GasGuard - Live Cloud Hardware Telemetry Streamer
color 0a
echo ======================================================================
echo   AURA GASGUARD - STREAMING LIVE ESP32 SENSOR DATA TO RENDER CLOUD
echo ======================================================================
echo.
echo [*] Live Cloud Target: https://aura-gasguard.onrender.com/api/sensor-data
echo [*] Opening Live Cloud Dashboard in your browser...
echo.

start https://aura-gasguard.onrender.com

set PYTHONUNBUFFERED=1
set BACKEND_URL=https://aura-gasguard.onrender.com/api/sensor-data

python -u serial_bridge.py
if errorlevel 1 (
    echo.
    echo [ERROR] Python execution exited with an error. Check if pyserial or requests is installed.
)
pause
