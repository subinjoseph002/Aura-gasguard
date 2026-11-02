# 🚨 Aura GasGuard™ — IoT Gas Leakage Detection System
### ESP32 • Containerized Python Backend • SQLite Database • Real-Time Web Dashboard • Cloud Ready (Render & Docker)

A complete, production-grade Internet-of-Things (IoT) safety sentinel engineered for high-precision gas detection, real-time edge telemetry, persistent SQLite data logging, and cloud visualization.

---

## 🌟 Key Features

* **High-Precision MQ-2 Gas Sensing:** Real-time analog acquisition (ADC1 GPIO 34) calibrated for flammable gases (LPG, Propane, Methane, Hydrogen, and Smoke).
* **20-Second Smart Warmup Grace Period:** Prevents false alarms during electrochemical sensor heater stabilization.
* **Dual Edge Alarm Actuation:** Hardware-level buzzer alerts and dual 3D animated status LEDs (Green = Safe, Red = Emergency Strobe).
* **Containerized Microservice Architecture:** Production Dockerfile and `docker-compose.yml` with automated container healthcheck and volume persistence.
* **Robust SQLite 3 Relational Database:** Persistent time-series logging with WAL (Write-Ahead Logging) mode, auto-directory creation, and zero-loss restart survival.
* **Modern Light Theme Web Dashboard:** Clean, intuitive UI featuring SVG arc dials, 60-second animated trend graphs, live database audit table, and zero mockup data.
* **Dual ESP32 Connectivity:** Operates on local Wi-Fi and simultaneously hosts an autonomous presentation SoftAP (`ESP32-Gas-Detector`) at `http://192.168.4.1`.
* **One-Click Cloud Deployment:** Ready for GitHub and Render deployment with automated Docker builds, health monitoring, and persistent disk mounting.

---

## 🛠️ Technology Stack

| Domain | Technology | Purpose |
| :--- | :--- | :--- |
| **Edge Hardware** | ESP32 DevKit V1 (30-Pin) | 240MHz Dual-Core SoC for sampling and telemetry |
| **Sensing Module** | MQ-2 Semiconductor Sensor | Flammable gas and smoke detection |
| **Firmware Runtime**| C++ / Arduino via PlatformIO | 100ms non-blocking FreeRTOS sampling loop |
| **Backend Service** | Python 3.12 (Standard Library) | Lightweight, zero-overhead HTTP REST API & static server |
| **Database** | SQLite 3 (`WAL` mode) | Persistent relational telemetry and audit logs |
| **Frontend UI** | HTML5, CSS3, JavaScript (ES6+), Canvas | Responsive light theme real-time telemetry dashboard |
| **Containerization**| Docker & Docker Compose | Containerized isolated execution and multi-environment portability |
| **Cloud Hosting** | Render Web Services | Cloud deployment with persistent disk & SSL |
| **Version Control** | Git & GitHub | Source code tracking and CI/CD automated deployment |

---

## 📂 Project Structure

```text
IOT Project/
├── .dockerignore                  # Files excluded from Docker builds
├── .gitignore                     # Files excluded from Git version control
├── .env.example                   # Environment configuration template
├── docker-compose.yml             # Local multi-container / service orchestration
├── platformio.ini                 # PlatformIO embedded build configuration
├── render.yaml                    # Render Blueprint specification (IaC)
├── README.md                      # Complete project documentation
├── PROJECT_REPORT.md              # Academic and technical report
├── CIRCUIT_DIAGRAM.md             # Hardware schematics and wiring pinout
├── serial_bridge.py               # Low-latency USB serial-to-REST bridge (5 Hz)
│
├── backend/                       # Python Containerized Backend
│   ├── Dockerfile                 # Production Python 3.12-slim Dockerfile
│   ├── requirements.txt           # Python backend dependencies
│   ├── app.py                     # REST API Router, healthchecks & static server
│   ├── database.py                # SQLite schema, WAL mode, persistence logic
│   ├── data/                      # Persistent database directory
│   │   └── .gitkeep               # Directory placeholder (DB files ignored)
│   └── static/                    # Frontend Dashboard Assets
│       └── index.html             # High-performance light theme dashboard
│
├── src/                           # ESP32 Firmware Source Code
│   └── main.cpp                   # Sampling, alarm triggers, Wi-Fi & REST client
└── include/                       # Embedded C++ Headers
    └── web_dashboard.h            # On-chip presentation AP dashboard
```

---

## ⚙️ Environment Variables

The backend dynamically configures itself via environment variables:

| Variable | Default Value | Description |
| :--- | :--- | :--- |
| `PORT` | `5000` | Port for the HTTP REST server (auto-configured by Render). |
| `DATABASE_PATH` | `/app/data/sensor_database.db` | Absolute path to SQLite file. For Render Disks, set to `/var/data/sensor_database.db`. |
| `PYTHONUNBUFFERED`| `1` | Forces real-time stdout/stderr flushing for live logs. |
| `PYTHONDONTWRITEBYTECODE` | `1` | Prevents `.pyc` clutter inside containers. |

---

## 🚀 Local Docker Setup

### Prerequisites
* [Docker Desktop](https://www.docker.com/) installed and running.

### 1. Build and Start Container
```bash
docker compose up -d --build
```

### 2. Verify Container Health
```bash
# Check status and healthcheck output:
docker compose ps

# View live container logs:
docker compose logs -f
```

### 3. Access Dashboard & Healthcheck
* **Web Dashboard:** [http://localhost:5000](http://localhost:5000)
* **Healthcheck Endpoint:** [http://localhost:5000/health](http://localhost:5000/health)

---

## 📡 REST API Documentation

### 1. Ingest Sensor Telemetry
* **Method & Path:** `POST /api/sensor-data`
* **Headers:** `Content-Type: application/json`
* **Request Body:**
  ```json
  {
    "device_id": "ESP32-GAS-01",
    "sensor_type": "MQ-2",
    "raw_value": 780,
    "gas_percent": 19.0,
    "ppm_estimated": 190,
    "status": "SAFE",
    "buzzer_state": 0
  }
  ```
* **Response:** `201 Created`
  ```json
  {
    "status": "success",
    "id": 142,
    "threshold": 40
  }
  ```

### 2. Health Check
* **Method & Path:** `GET /health` (or `GET /api/health`)
* **Response:** `200 OK`
  ```json
  {
    "status": "healthy",
    "service": "Aura GasGuard Backend",
    "database": "connected",
    "db_path": "/app/data/sensor_database.db",
    "timestamp": "2026-11-02 15:00:00"
  }
  ```

### 3. Fetch Latest Sensor Reading
* **Method & Path:** `GET /api/sensor-data/latest`
* **Response:** `200 OK` (returns most recent database record with active threshold)

### 4. Fetch Telemetry History (Chart Data)
* **Method & Path:** `GET /api/sensor-data/history?limit=60`
* **Response:** `200 OK` (returns chronological list of readings)

### 5. Fetch Aggregate Statistics
* **Method & Path:** `GET /api/statistics`
* **Response:** `200 OK` (returns total readings, peak gas %, alarm counts, uptime)

### 6. Export Database to CSV
* **Method & Path:** `GET /api/export/csv`
* **Response:** `200 OK` (CSV file download of all historical readings)

### 7. Update Alarm Threshold Setpoint
* **Method & Path:** `POST /api/config/threshold`
* **Request Body:** `{"threshold": 45}`
* **Response:** `200 OK`

---

## ☁️ Render Cloud Deployment Instructions

### Option 1: Blueprint Deployment via `render.yaml` (Recommended)
1. Push this repository to your GitHub account (see Git instructions below).
2. Log in to [Render Dashboard](https://dashboard.render.com/).
3. Click **New +** ➔ **Blueprint**.
4. Connect your GitHub repository.
5. Render detects [`render.yaml`](file:///c:/Users/hp/Desktop/IOT%20Project/render.yaml) automatically:
   - Sets **Runtime** to `Docker`.
   - Attaches a **1 GB Persistent Disk** mounted at `/var/data` (Starter plan).
   - Configures `DATABASE_PATH=/var/data/sensor_database.db`.
   - Configures healthcheck to `/health`.
6. Click **Apply** to launch!

### Option 2: Manual Web Service Deployment
1. In Render Dashboard, click **New +** ➔ **Web Service**.
2. Connect your GitHub repository.
3. Choose **Docker** as the Runtime:
   - **Dockerfile Path:** `backend/Dockerfile`
   - **Docker Context:** `backend`
4. Set Environment Variables:
   - `PORT`: `5000`
   - `PYTHONUNBUFFERED`: `1`
   - `DATABASE_PATH`: `/var/data/sensor_database.db` (if persistent disk attached) or `/app/data/sensor_database.db` (if on Free Tier).
5. **Persistent Disk (Optional for SQLite survival on paid tier):**
   - Under **Disks**, add a disk named `gasguard-data` with Mount Path `/var/data` (1 GB).
6. Under **Advanced**, set **Health Check Path** to `/health`.
7. Click **Create Web Service**.

> [!IMPORTANT]
> **Understanding SQLite Persistence on Render:**
> - Render instances have an ephemeral file system by default. When the service restarts or spins down due to inactivity (Free Tier), files stored inside the container disappear.
> - Attaching a **Render Persistent Disk** (`/var/data`) ensures your SQLite database file permanently survives restarts, redeployments, and scaling.
> - If operating on the Free Tier without a persistent disk, the application functions normally using `/app/data/sensor_database.db`, but the database resets upon service restart.

---

## 🔌 Connecting ESP32 to Render Cloud

Once your Render Web Service is deployed, copy its public URL (e.g. `https://aura-gasguard.onrender.com`).

### 1. Direct ESP32 Wi-Fi Mode
In [`src/main.cpp`](file:///c:/Users/hp/Desktop/IOT%20Project/src/main.cpp#L48-L50), update `BACKEND_SERVER_URL`:
```cpp
const char* WIFI_SSID          = "Your_Home_WiFi";
const char* WIFI_PASSWORD      = "Your_WiFi_Password";
const char* BACKEND_SERVER_URL = "https://aura-gasguard.onrender.com/api/sensor-data";
```
Recompile and upload using PlatformIO:
```bash
pio run -t upload
```

### 2. USB Serial Bridge Mode (Local Hardware ➔ Cloud)
If your ESP32 is connected via USB:
```bash
$env:BACKEND_URL="https://aura-gasguard.onrender.com/api/sensor-data"
python -u serial_bridge.py
```
Hardware sensor readings from COM5 will stream straight to your live Render cloud deployment!
