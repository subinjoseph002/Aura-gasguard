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
import threading

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

BACKEND_URL = os.environ.get("BACKEND_URL", "https://aura-gasguard.onrender.com/api/sensor-data")
DEFAULT_BAUD = 115200

TELEMETRY_PATTERN = re.compile(
    r"\[LIVE\]\s+Gas:\s+([\d\.]+)%\s+\|\s+Raw ADC:\s+(\d+)\s+\|\s+Limit:\s+(\d+)%\s+\|\s+Status:\s+(\w+)"
)

def find_esp32_port():
    """SERIAL_PORT env var wins; otherwise prefer known ESP32 USB-UART bridge chips."""
    forced = os.environ.get("SERIAL_PORT")
    if forced:
        return forced
    ports = list(serial.tools.list_ports.comports())
    for keywords in (("cp210", "ch340", "ch910", "ftdi"), ("usb", "uart")):
        for p in ports:
            text = f"{p.description or ''} {p.manufacturer or ''}".lower()
            if any(k in text for k in keywords):
                return p.device
    return ports[0].device if ports else "COM5"

LAST_ERROR = ""

def post_to_backend(payload):
    """POST one reading. Returns (ok, backend_threshold or None)."""
    global LAST_ERROR
    if USE_REQUESTS:
        try:
            resp = HTTP_SESSION.post(BACKEND_URL, json=payload, timeout=10.0)
            ok = resp.status_code in (200, 201)
            LAST_ERROR = "" if ok else f"HTTP {resp.status_code}"
            return ok, (resp.json().get("threshold") if ok else None)
        except Exception as e:
            LAST_ERROR = type(e).__name__
            return False, None
    else:
        try:
            req = urllib.request.Request(
                BACKEND_URL,
                data=json.dumps(payload).encode("utf-8"),
                headers={"Content-Type": "application/json"}
            )
            with urllib.request.urlopen(req, timeout=10.0) as resp:
                ok = resp.status in (200, 201)
                return ok, (json.loads(resp.read()).get("threshold") if ok else None)
        except Exception as e:
            LAST_ERROR = type(e).__name__
            return False, None

def sender_loop(state):
    """Uploads readings on its own thread so a slow network never stalls the serial reader.
    Only the NEWEST reading is kept; stale ones are dropped (status changes are never dropped)."""
    count = 0
    last_pushed_threshold = None
    while not state["stop"]:
        with state["cond"]:
            while not state["pending"] and not state["stop"]:
                state["cond"].wait(0.5)
            batch, state["pending"] = state["pending"], []
            dropped, state["dropped"] = state["dropped"], 0
        for payload in batch:
            started = time.time()
            ok, backend_threshold = post_to_backend({k: v for k, v in payload.items() if not k.startswith("_")})
            count += 1
            device_threshold = payload["threshold"]
            ser = state["ser"]
            # Cloud dashboard owns the alarm setpoint: push changes down to the ESP32 over USB.
            if (isinstance(backend_threshold, int) and backend_threshold != device_threshold
                    and backend_threshold != last_pushed_threshold and ser is not None):
                last_pushed_threshold = backend_threshold
                try:
                    ser.write(f"T{backend_threshold}\n".encode())
                    print(f"[CONFIG] Pushed threshold {backend_threshold}% to ESP32", flush=True)
                except Exception:
                    pass
            status = payload["status"]
            icon = {"DANGER": "🔴", "WARNING": "🟡", "FAULT": "⚪"}.get(status, "🟢")
            sync = "OK" if ok else "RETRY (" + LAST_ERROR + ")"
            skipped = f" | skipped {dropped} stale" if dropped else ""
            print(f"[{count:04d}] {icon} Gas: {payload['gas_percent']:5.1f}% | ADC: {payload['raw_value']:4d} | "
                  f"Status: {status:7s} | Cloud Sync: {sync} ({(time.time() - started) * 1000:.0f} ms){skipped}", flush=True)
            dropped = 0


def main():
    port = find_esp32_port()
    print("=" * 70, flush=True)
    print("  AURA GASGUARD — HIGH-PERFORMANCE CLOUD & LOCAL SERIAL BRIDGE", flush=True)
    print("=" * 70, flush=True)
    print(f"  [>] Serial Port: {port} @ {DEFAULT_BAUD} baud", flush=True)
    print(f"  [>] Ingestion Target: {BACKEND_URL}", flush=True)
    print("  [>] Press Ctrl+C to stop streaming.", flush=True)
    print("=" * 70, flush=True)

    state = {"cond": threading.Condition(), "pending": [], "dropped": 0, "stop": False, "ser": None}
    threading.Thread(target=sender_loop, args=(state,), daemon=True).start()

    ser = None
    warn_printed = False
    last_status = None

    while True:
        try:
            if ser is None or not ser.is_open:
                try:
                    port = find_esp32_port()
                    ser = serial.Serial(port, DEFAULT_BAUD, timeout=0.3)
                    ser.reset_input_buffer()          # never replay readings buffered while we were away
                    state["ser"] = ser
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
                status = match.group(4).upper()
                payload = {
                    "device_id": "ESP32-GAS-01",
                    "sensor_type": "MQ-2",
                    "raw_value": int(match.group(2)),
                    "gas_percent": gas_pct,
                    "ppm_estimated": int((gas_pct / 100.0) * 1200 + 15),
                    "status": status,
                    "buzzer_state": status == "DANGER",
                    "threshold": int(match.group(3))
                }
                with state["cond"]:
                    # Keep status transitions (alarm on/off) and only the newest ordinary reading.
                    kept = [p for p in state["pending"] if p.get("_keep")]
                    state["dropped"] += len(state["pending"]) - len(kept)
                    payload["_keep"] = status != last_status
                    state["pending"] = kept + [payload]
                    state["cond"].notify()
                last_status = status

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
                state["ser"] = None
            time.sleep(1.0)

    state["stop"] = True
    if ser and ser.is_open:
        ser.close()

if __name__ == "__main__":
    main()
