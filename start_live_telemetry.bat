@echo off
title Aura GasGuard - Live Hardware Telemetry Streamer
color 0b
echo ======================================================================
echo   AURA GASGUARD - LIVE PHYSICAL HARDWARE TELEMETRY STREAMER
echo ======================================================================
echo.
echo [*] Streaming real-time sensor data from ESP32 (COM5) to Docker Backend...
echo [*] Database target: SQLite (backend/data/sensor_database.db)
echo [*] Launching Real-Time Dashboard at http://localhost:5000 ...
echo.

start "" "http://localhost:5000"

python -u serial_bridge.py
pause

