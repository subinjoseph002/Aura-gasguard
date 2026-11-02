"""
=====================================================================================
Aura GasGuard - Live Hardware Serial Bridge (ESP32 USB -> Containerized Backend)
=====================================================================================
Reads real-time physical telemetry streamed over USB Serial (COM5) by the ESP32,
parses the sensor readings, and relays them to the Docker REST API (:5000).
Stores all real-world data into the persistent SQLite database.
=====================================================================================
"""

import sys
import time
import re
import os
import urllib.request
import json

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[ERROR] pyserial is required. Install via: pip install pyserial")
    sys.exit(1)

BACKEND_URL = os.environ.get("BACKEND_URL", "http://localhost:5000/api/sensor-data")
DEFAULT_BAUD = 115200

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

def find_esp32_port():
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        desc = (p.description or "").lower()
        if "cp210" in desc or "ch340" in desc or "usb" in desc or "uart" in desc:
            return p.device
    if ports:
        return ports[0].device
    return "COM5"

def post_to_backend(payload):
    try:
        req = urllib.request.Request(
            BACKEND_URL,
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"}
        )
        with urllib.request.urlopen(req, timeout=0.6) as resp:
            return resp.status in (200, 201)
    except Exception as e:
        return False

def main():
    port = find_esp32_port()
    print("=" * 70)
    print("  AURA GASGUARD — ULTRA-LOW LATENCY SERIAL BRIDGE")
    print("=" * 70)
    print(f"  [>] Serial Port: {port} @ {DEFAULT_BAUD} baud")
    print(f"  [>] Ingestion Target: {BACKEND_URL}")
    print("  [>] Press Ctrl+C to stop streaming.")
    print("=" * 70)

    ser = None
    count = 0
    pattern = re.compile(
        r"\[LIVE\]\s+Gas:\s+([\d\.]+)%\s+\|\s+Raw ADC:\s+(\d+)\s+\|\s+Limit:\s+(\d+)%\s+\|\s+Status:\s+(\w+)"
    )

    while True:
        try:
            if ser is None or not ser.is_open:
                try:
                    port = find_esp32_port()
                    ser = serial.Serial(port, DEFAULT_BAUD, timeout=0.3)
                    print(f"[CONNECTED] Opened {port} @ {DEFAULT_BAUD} baud.")
                    time.sleep(0.5)
                except Exception as e:
                    time.sleep(1.0)
                    continue

            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if not line:
                continue

            match = pattern.search(line)
            if match:
                gas_pct = float(match.group(1))
                raw_adc = int(match.group(2))
                threshold = int(match.group(3))
                status = match.group(4).upper()
                ppm = int((gas_pct / 100.0) * 1200 + 15)

                payload = {
                    "device_id": "ESP32-GAS-01",
                    "sensor_type": "MQ-2",
                    "raw_value": raw_adc,
                    "gas_percent": gas_pct,
                    "ppm_estimated": ppm,
                    "status": status,
                    "buzzer_state": status == "DANGER",
                    "threshold": threshold
                }

                ok = post_to_backend(payload)
                count += 1
                status_icon = "🔴" if status == "DANGER" else ("🟡" if status == "WARNING" else "🟢")
                print(f"[{count:04d}] {status_icon} Gas: {gas_pct:5.1f}% | ADC: {raw_adc:4d} | Status: {status:7s} | Saved: {'YES' if ok else 'FAIL'}")

            elif "[WARMUP]" in line:
                print(f"  {line}")

        except KeyboardInterrupt:
            print("\n[INFO] Stopped by user.")
            break
        except Exception as ex:
            print(f"[WARN] Read error: {ex}")
            if ser:
                try:
                    ser.close()
                except Exception:
                    pass
                ser = None
            time.sleep(1.0)

    if ser and ser.is_open:
        ser.close()

if __name__ == "__main__":
    main()
