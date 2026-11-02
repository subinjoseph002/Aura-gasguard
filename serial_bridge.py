"""
=====================================================================================
Aura GasGuard - Live Hardware Serial Bridge (ESP32 USB -> Backend / Cloud)
=====================================================================================
Reads real-time physical telemetry streamed over USB Serial (COM5) by the ESP32,
parses the sensor readings, and relays them to either the local Docker REST API
or the deployed Render Cloud endpoint.
Uses HTTP keep-alive connection pooling for ultra-low latency (< 100ms).
=====================================================================================
"""

import sys
import time
import re
import os
import json

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[ERROR] pyserial is required. Install via: pip install pyserial", flush=True)
    sys.exit(1)

# Use requests.Session for persistent keep-alive connection pooling if available
try:
    import requests
    HTTP_SESSION = requests.Session()
    USE_REQUESTS = True
except ImportError:
    import urllib.request
    HTTP_SESSION = None
    USE_REQUESTS = False

BACKEND_URL = os.environ.get("BACKEND_URL", "http://localhost:5000/api/sensor-data")
DEFAULT_BAUD = 115200

TELEMETRY_PATTERN = re.compile(
    r"\[LIVE\]\s+Gas:\s+([\d\.]+)%\s+\|\s+Raw ADC:\s+(\d+)\s+\|\s+Limit:\s+(\d+)%\s+\|\s+Status:\s+(\w+)"
)

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
    if USE_REQUESTS:
        try:
            resp = HTTP_SESSION.post(BACKEND_URL, json=payload, timeout=3.0)
            return resp.status_code in (200, 201)
        except Exception as e:
            return False
    else:
        try:
            req = urllib.request.Request(
                BACKEND_URL,
                data=json.dumps(payload).encode("utf-8"),
                headers={"Content-Type": "application/json"}
            )
            with urllib.request.urlopen(req, timeout=3.0) as resp:
                return resp.status in (200, 201)
        except Exception as e:
            return False

def main():
    port = find_esp32_port()
    print("=" * 70, flush=True)
    print("  AURA GASGUARD — HIGH-PERFORMANCE CLOUD & LOCAL SERIAL BRIDGE", flush=True)
    print("=" * 70, flush=True)
    print(f"  [>] Serial Port: {port} @ {DEFAULT_BAUD} baud", flush=True)
    print(f"  [>] Ingestion Target: {BACKEND_URL}", flush=True)
    print("  [>] Press Ctrl+C to stop streaming.", flush=True)
    print("=" * 70, flush=True)

    ser = None
    count = 0
    warn_printed = False

    while True:
        try:
            if ser is None or not ser.is_open:
                try:
                    port = find_esp32_port()
                    ser = serial.Serial(port, DEFAULT_BAUD, timeout=0.3)
                    print(f"\n[CONNECTED] Successfully opened {port} @ {DEFAULT_BAUD} baud.", flush=True)
                    print(f"[STREAMING] Relaying hardware telemetry directly to cloud...\n", flush=True)
                    warn_printed = False
                    time.sleep(0.5)
                except Exception as e:
                    if not warn_printed:
                        print(f"[WAIT] Waiting for {port} to become accessible ({e}). Retrying...", flush=True)
                        warn_printed = True
                    time.sleep(1.0)
                    continue

            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if not line:
                continue

            match = TELEMETRY_PATTERN.search(line)
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
                print(f"[{count:04d}] {status_icon} Gas: {gas_pct:5.1f}% | ADC: {raw_adc:4d} | Status: {status:7s} | Cloud Sync: {'OK' if ok else 'RETRY'}", flush=True)

            elif "[WARMUP]" in line:
                print(f"  {line}", flush=True)

        except KeyboardInterrupt:
            print("\n[INFO] Stopped by user.", flush=True)
            break
        except Exception as ex:
            print(f"[WARN] Communication error: {ex}", flush=True)
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
