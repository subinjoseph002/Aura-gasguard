@echo off
title Aura GasGuard - Live Cloud Hardware Telemetry Streamer
color 0a
echo ======================================================================
echo   AURA GASGUARD - STREAMING LIVE ESP32 SENSOR DATA TO RENDER CLOUD
echo ======================================================================
echo.
echo [*] Streaming real-time sensor data from ESP32 (COM5) to:
echo     https://aura-gasguard.onrender.com/api/sensor-data
echo [*] Opening Live Cloud Dashboard in your browser...
echo.

start "" "https://aura-gasguard.onrender.com"

set BACKEND_URL=https://aura-gasguard.onrender.com/api/sensor-data
python -u serial_bridge.py
pause
