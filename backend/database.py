"""
=====================================================================================
Aura GasGuard - SQLite Database Layer
=====================================================================================
Handles persistent storage of sensor readings, system alarms, configuration settings,
and analytical queries using SQLite.
=====================================================================================
"""

import sqlite3
import os
from datetime import datetime

# Database path (configurable via DATABASE_PATH environment variable for Render Persistent Disks)
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_DATA_DIR = os.path.join(BASE_DIR, "data")
DEFAULT_DB_PATH = os.path.join(DEFAULT_DATA_DIR, "sensor_database.db")

DB_PATH = os.environ.get("DATABASE_PATH", DEFAULT_DB_PATH)

DEFAULT_THRESHOLD = 10        # % gas concentration that triggers the alarm (matches firmware)
THRESHOLD_MIN, THRESHOLD_MAX = 5, 95
RETENTION_DAYS = int(os.environ.get("RETENTION_DAYS", 3))   # readings older than this are pruned

def get_connection():
    db_dir = os.path.dirname(os.path.abspath(DB_PATH))
    if db_dir:
        os.makedirs(db_dir, exist_ok=True)
    try:
        conn = sqlite3.connect(DB_PATH, timeout=10.0)
        conn.row_factory = sqlite3.Row
        # Enable WAL mode for efficient concurrent read/writes
        conn.execute("PRAGMA journal_mode=WAL;")
        conn.execute("PRAGMA synchronous=NORMAL;")   # safe in WAL mode, far fewer fsyncs per reading
        return conn
    except sqlite3.Error as e:
        print(f"[ERROR] Database connection failed for path '{DB_PATH}': {e}")
        raise

def init_db():
    conn = get_connection()
    cursor = conn.cursor()

    # Table 1: Sensor Readings (Time-series telemetry)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS sensor_readings (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp TEXT NOT NULL,
        device_id TEXT NOT NULL,
        sensor_type TEXT NOT NULL DEFAULT 'MQ-2',
        raw_value INTEGER NOT NULL,
        gas_percent REAL NOT NULL,
        ppm_estimated INTEGER NOT NULL,
        status TEXT NOT NULL,
        fan_state INTEGER NOT NULL DEFAULT 0,
        buzzer_state INTEGER NOT NULL DEFAULT 0
    )
    """)

    # Table 2: System Event Audit Log (Alarms, overrides, drills)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS system_events (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp TEXT NOT NULL,
        event_type TEXT NOT NULL,
        message TEXT NOT NULL,
        severity TEXT NOT NULL DEFAULT 'INFO'
    )
    """)

    # Table 3: System Configuration (Thresholds, overrides)
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS system_config (
        key TEXT PRIMARY KEY,
        value TEXT NOT NULL
    )
    """)

    # Insert default configurations if not exists
    cursor.execute("INSERT OR IGNORE INTO system_config (key, value) VALUES ('threshold', ?)", (str(DEFAULT_THRESHOLD),))
    cursor.execute("INSERT OR IGNORE INTO system_config (key, value) VALUES ('buzzer_muted', '0')")

    # Migration v2: the alarm setpoint is 10 % (older databases still hold the old 40 % default).
    cursor.execute("SELECT 1 FROM system_config WHERE key = 'schema_version'")
    if not cursor.fetchone():
        cursor.execute("UPDATE system_config SET value = ? WHERE key = 'threshold'", (str(DEFAULT_THRESHOLD),))
        cursor.execute("INSERT INTO system_config (key, value) VALUES ('schema_version', '2')")

    cursor.execute("CREATE INDEX IF NOT EXISTS idx_readings_ts ON sensor_readings (timestamp)")

    conn.commit()
    conn.close()

def insert_reading(data):
    conn = get_connection()
    cursor = conn.cursor()
    now_iso = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    cursor.execute("""
    INSERT INTO sensor_readings (
        timestamp, device_id, sensor_type, raw_value,
        gas_percent, ppm_estimated, status, fan_state, buzzer_state
    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, (
        now_iso,
        data.get("device_id", "ESP32-GAS-01"),
        data.get("sensor_type", "MQ-2"),
        int(data.get("raw_value", 0)),
        float(data.get("gas_percent", 0.0)),
        int(data.get("ppm_estimated", 0)),
        data.get("status", "SAFE"),
        1 if data.get("fan_state") else 0,
        1 if data.get("buzzer_state") else 0
    ))
    reading_id = cursor.lastrowid
    if reading_id % 500 == 0:   # cheap periodic retention so 1 Hz telemetry cannot fill the disk
        cursor.execute("DELETE FROM sensor_readings WHERE timestamp < datetime('now','localtime', ?)",
                       (f"-{RETENTION_DAYS} days",))
        cursor.execute("DELETE FROM system_events WHERE timestamp < datetime('now','localtime', ?)",
                       (f"-{RETENTION_DAYS * 4} days",))
    conn.commit()
    conn.close()
    return reading_id

def insert_event(event_type, message, severity="INFO"):
    conn = get_connection()
    cursor = conn.cursor()
    now_iso = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    cursor.execute("""
    INSERT INTO system_events (timestamp, event_type, message, severity)
    VALUES (?, ?, ?, ?)
    """, (now_iso, event_type, message, severity))
    conn.commit()
    conn.close()

def get_latest_reading():
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("""
    SELECT * FROM sensor_readings ORDER BY id DESC LIMIT 1
    """)
    row = cursor.fetchone()
    conn.close()
    return dict(row) if row else None

def get_recent_readings(limit=60):
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("""
    SELECT * FROM (
        SELECT * FROM sensor_readings ORDER BY id DESC LIMIT ?
    ) ORDER BY id ASC
    """, (limit,))
    rows = cursor.fetchall()
    conn.close()
    return [dict(r) for r in rows]

def get_statistics():
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("""
    SELECT 
        COUNT(*) as total_readings,
        ROUND(AVG(gas_percent), 1) as avg_gas_percent,
        ROUND(MIN(gas_percent), 1) as min_gas_percent,
        ROUND(MAX(gas_percent), 1) as max_gas_percent,
        SUM(CASE WHEN status = 'DANGER' THEN 1 ELSE 0 END) as total_danger_events,
        SUM(CASE WHEN fan_state = 1 THEN 1 ELSE 0 END) as total_fan_activations
    FROM sensor_readings
    """)
    row = cursor.fetchone()
    conn.close()
    return dict(row) if row else {}

def get_recent_events(limit=25):
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("""
    SELECT * FROM system_events ORDER BY id DESC LIMIT ?
    """, (limit,))
    rows = cursor.fetchall()
    conn.close()
    return [dict(r) for r in rows]

def get_config(key, default=None):
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT value FROM system_config WHERE key = ?", (key,))
    row = cursor.fetchone()
    conn.close()
    return row["value"] if row else default

def set_config(key, value):
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("""
    INSERT INTO system_config (key, value) VALUES (?, ?)
    ON CONFLICT(key) DO UPDATE SET value = excluded.value
    """, (key, str(value)))
    conn.commit()
    conn.close()

