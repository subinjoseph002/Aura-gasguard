"""
=====================================================================================
Aura GasGuard - Containerized REST API & Backend Server
=====================================================================================
Receives real-time sensor telemetry from ESP32, stores data in SQLite database,
serves REST endpoints, and hosts the real-time visualization dashboard.

Runs natively via `python app.py` or inside Docker container via `docker-compose.yml`.
=====================================================================================
"""

import http.server
import socketserver
import urllib.parse
import json
import os
import sys
import time
from datetime import datetime
from database import (
    init_db, insert_reading, insert_event, get_latest_reading,
    get_recent_readings, get_statistics, get_recent_events,
    get_config, set_config, get_connection, DB_PATH
)

HOST = "0.0.0.0"
PORT = int(os.environ.get("PORT", 5000))
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
STATIC_DIR = os.path.join(BASE_DIR, "static")

# Initialize SQLite database schema
init_db()

# Track previous status for edge-triggered alarm event logging
prev_device_status = {}

class BackendRouter(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Enable Cross-Origin Resource Sharing (CORS)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(204)
        self.end_headers()

    def send_json(self, status_code, payload):
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        self.end_headers()
        self.wfile.write(json.dumps(payload).encode("utf-8"))

    # =================================================================================
    # GET REQUEST ROUTING
    # =================================================================================
    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path.rstrip("/")
        query = urllib.parse.parse_qs(parsed.query)

        # 0. Dedicated Healthcheck Endpoint (for Docker and Render)
        if path in ["/health", "/api/health"]:
            try:
                conn = get_connection()
                conn.execute("SELECT 1").fetchone()
                conn.close()
                self.send_json(200, {
                    "status": "healthy",
                    "service": "Aura GasGuard Backend",
                    "database": "connected",
                    "db_path": DB_PATH,
                    "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                })
            except Exception as e:
                self.send_json(500, {
                    "status": "unhealthy",
                    "error": str(e)
                })
            return

        # 1. Latest Reading API
        if path == "/api/sensor-data/latest":
            latest = get_latest_reading()
            threshold = int(get_config("threshold", 40))
            if latest:
                latest["threshold"] = threshold
                self.send_json(200, latest)
            else:
                self.send_json(200, {
                    "raw_value": None, "gas_percent": None, "ppm_estimated": None,
                    "status": "WAITING_DATA", "buzzer_state": 0,
                    "threshold": threshold,
                    "timestamp": "Awaiting first reading..."
                })
            return

        # 2. Historical Chart Data API
        elif path == "/api/sensor-data/history":
            limit = int(query.get("limit", [60])[0])
            readings = get_recent_readings(min(120, max(10, limit)))
            self.send_json(200, {
                "count": len(readings),
                "threshold": int(get_config("threshold", 40)),
                "readings": readings
            })
            return

        # 3. Aggregated Statistics API
        elif path == "/api/statistics":
            stats = get_statistics()
            self.send_json(200, stats)
            return

        # 4. Audit Log Events API
        elif path == "/api/events":
            events = get_recent_events(30)
            self.send_json(200, events)
            return

        # 5. Export Database to CSV
        elif path == "/api/export/csv":
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM sensor_readings ORDER BY id DESC")
            rows = cursor.fetchall()
            conn.close()

            csv_data = "ID,Timestamp,Device,Sensor,RawValue,GasPercent,PPM,Status,Fan,Buzzer\n"
            for r in rows:
                csv_data += f"{r['id']},{r['timestamp']},{r['device_id']},{r['sensor_type']},{r['raw_value']},{r['gas_percent']},{r['ppm_estimated']},{r['status']},{r['fan_state']},{r['buzzer_state']}\n"

            self.send_response(200)
            self.send_header("Content-Type", "text/csv")
            self.send_header("Content-Disposition", 'attachment; filename="sensor_database_export.csv"')
            self.end_headers()
            self.wfile.write(csv_data.encode("utf-8"))
            return

        # 6. Serve Real-Time Dashboard UI
        elif path in ["", "/", "/index.html"]:
            dashboard_file = os.path.join(STATIC_DIR, "index.html")
            if os.path.exists(dashboard_file):
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.end_headers()
                with open(dashboard_file, "rb") as f:
                    self.wfile.write(f.read())
                return

        # Serve static assets (CSS, JS, images)
        clean_path = parsed.path.lstrip("/")
        target_file = os.path.join(STATIC_DIR, clean_path)
        if os.path.exists(target_file) and os.path.isfile(target_file):
            content_type = "text/plain"
            if target_file.endswith(".html"): content_type = "text/html"
            elif target_file.endswith(".css"): content_type = "text/css"
            elif target_file.endswith(".js"): content_type = "application/javascript"
            elif target_file.endswith(".json"): content_type = "application/json"
            
            self.send_response(200)
            self.send_header("Content-Type", content_type)
            self.end_headers()
            with open(target_file, "rb") as f:
                self.wfile.write(f.read())
            return

        self.send_json(404, {"error": "Route not found", "path": self.path})

    # =================================================================================
    # POST REQUEST ROUTING (Data Ingestion & Controls)
    # =================================================================================
    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path.rstrip("/")
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8") if length > 0 else ""

        payload = {}
        if body:
            try:
                payload = json.loads(body)
            except Exception:
                payload = urllib.parse.parse_qs(body)

        # 1. INGEST TELEMETRY FROM ESP32 (Requirement 2)
        if path == "/api/sensor-data":
            if not payload or not isinstance(payload, dict):
                self.send_json(400, {"error": "Invalid JSON payload"})
                return

            device_id = payload.get("device_id", "ESP32-GAS-01")
            status = payload.get("status", "SAFE")
            gas_pct = float(payload.get("gas_percent", 0.0))
            raw_val = int(payload.get("raw_value", 0))

            # Insert into database
            row_id = insert_reading(payload)

            # Audit log state transitions
            prev_status = prev_device_status.get(device_id, "SAFE")
            if status == "DANGER" and prev_status != "DANGER":
                insert_event("ALARM_TRIGGERED", f"🚨 Gas leak detected by {device_id}: {gas_pct:.1f}% (ADC: {raw_val})", "CRITICAL")
            elif status == "SAFE" and prev_status == "DANGER":
                insert_event("ALARM_CLEARED", f"✅ Gas concentration cleared on {device_id}: {gas_pct:.1f}%", "INFO")
            prev_device_status[device_id] = status

            # Return response to ESP32 with current threshold
            threshold = int(get_config("threshold", 40))

            self.send_json(201, {
                "status": "success",
                "id": row_id,
                "threshold": threshold
            })
            return

        # 2. UPDATE ALARM THRESHOLD
        elif path == "/api/config/threshold":
            val = payload.get("threshold")
            if val is not None:
                val = int(val)
                if 10 <= val <= 95:
                    set_config("threshold", val)
                    insert_event("CONFIG_CHANGE", f"Alarm setpoint updated to {val}%", "INFO")
                    self.send_json(200, {"status": "success", "threshold": val})
                    return
            self.send_json(400, {"error": "Invalid threshold value (10-95)"})
            return

        # 3. EMERGENCY SYSTEM TEST DRILL
        elif path == "/api/test-alarm":
            insert_event("TEST_DRILL", "🚨 4-Second Emergency System Drill initiated from dashboard", "WARNING")
            self.send_json(200, {"status": "testing", "duration_seconds": 4})
            return

        # 4. PURGE / RESET DATABASE
        elif path == "/api/database/clear":
            conn = get_connection()
            cursor = conn.cursor()
            cursor.execute("DELETE FROM sensor_readings")
            cursor.execute("DELETE FROM sqlite_sequence WHERE name='sensor_readings'")
            cursor.execute("DELETE FROM system_events")
            cursor.execute("DELETE FROM sqlite_sequence WHERE name='system_events'")
            cursor.execute("UPDATE system_config SET value='40' WHERE key='threshold'")
            cursor.execute("UPDATE system_config SET value='0' WHERE key='buzzer_muted'")
            conn.commit()
            conn.close()
            self.send_json(200, {"status": "success", "message": "Database cleared successfully"})
            return

        self.send_json(404, {"error": "Endpoint not found"})

def run_server():
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer((HOST, PORT), BackendRouter) as httpd:
        print("=" * 70)
        print("  AURA GASGUARD — CONTAINERIZED IOT BACKEND & DATABASE")
        print("=" * 70)
        print(f"  [>] Server active on: http://{HOST}:{PORT}")
        print(f"  [>] Ingestion Endpoint: POST http://{HOST}:{PORT}/api/sensor-data")
        print(f"  [>] Database Storage: SQLite ({DB_PATH})")
        print(f"  [>] Real-time Dashboard: http://localhost:{PORT}/")
        print("=" * 70)
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down backend server.")

if __name__ == "__main__":
    run_server()

