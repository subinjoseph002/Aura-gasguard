@echo off
title ESP32 CP2102 Driver Installer
color 0A
echo ======================================================================
echo          ESP32 CP2102 USB-to-UART Bridge Driver Installer
echo ======================================================================
echo.
echo Detecting administrative privileges and installing driver...
echo (If a User Account Control prompt appears, please click YES)
echo.

set "INF_PATH=%~dp0drivers\cp2102\pololu-cp2102-windows\slabvcp.inf"
set "EXE_PATH=%~dp0drivers\cp2102\pololu-cp2102-windows\pololu-cp2102-setup-x64.exe"

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "Start-Process powershell -Verb RunAs -Wait -ArgumentList '-NoProfile -ExecutionPolicy Bypass -Command Write-Host \"\"\"Installing CP2102 Driver...\"\"\" -ForegroundColor Cyan; pnputil.exe /add-driver \"\"\"%INF_PATH%\"\"\" /install; Start-Sleep -Seconds 2'"

echo.
echo Checking if COM port is now recognized...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$dev = Get-PnpDevice | Where-Object { $_.FriendlyName -like '*CP210*' -or $_.InstanceId -like '*VID_10C4&PID_EA60*' }; if ($dev.Status -eq 'OK') { Write-Host 'SUCCESS: Driver installed and ESP32 is recognized!' -ForegroundColor Green; Get-PnpDevice -Class Ports | Select-Object FriendlyName, Status | Format-Table } else { Write-Host 'Driver package added. If not recognized yet, launching wizard setup...' -ForegroundColor Yellow; Start-Process '%EXE_PATH%' -Verb RunAs }"

echo.
echo ======================================================================
echo Installation process completed!
echo If your ESP32 is plugged in, unplug and reconnect the USB cable.
echo ======================================================================
pause
