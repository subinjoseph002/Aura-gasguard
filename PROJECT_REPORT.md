# 🛡️ Aura GasGuard™ — IoT Gas Leakage Detection System
## Comprehensive Engineering Project Report & Architectural Specification

**Project Title:** Aura GasGuard™ — Autonomous IoT Gas Leakage Detection & Safety Sentinel  
**Author:** Subin Joseph  
**Course Requirement:** IoT Embedded Systems & Full-Stack Cloud Deployment  
**Firmware Version:** 3.2 (Production Low-Latency Edition)  
**Cloud Deployment:** Render Cloud Platform (Docker Web Service, Singapore Region)  
**Live Public Dashboard:** [https://aura-gasguard.onrender.com](https://aura-gasguard.onrender.com)  
**Live Healthcheck:** [https://aura-gasguard.onrender.com/health](https://aura-gasguard.onrender.com/health)  
**GitHub Repository:** [https://github.com/subinjoseph002/Aura-gasguard](https://github.com/subinjoseph002/Aura-gasguard)  

---

## 📑 Executive Summary

Aura GasGuard™ is an end-to-end, industrial-grade Internet-of-Things (IoT) safety system engineered to detect hazardous gas leaks (LPG, Propane, Methane, Hydrogen, Butane, and Smoke) with millisecond-grade responsiveness. 

The architecture bridges edge hardware perception with containerized cloud microservices:
1. **Perception & Edge Compute:** An **ESP32 DevKit V1** microcontroller acquires analog voltage signals from an **MQ-2 Electrochemical Sensor** on `GPIO 34` (ADC1), executing FreeRTOS non-blocking digital filtering and immediate local edge alarms (acoustic piezo siren on `GPIO 25`, 3D Green/Red optical LEDs on `GPIO 27`/`GPIO 26`).
2. **Network Telemetry:** Streams structured telemetry over HTTP REST JSON payloads. Operates in dual network configurations: direct Wi-Fi client mode or through an ultra-low latency serial-to-cloud bridge featuring **HTTP Keep-Alive Connection Pooling** (88ms latency to cloud).
3. **Containerized Cloud Backend:** Built with **Python 3.12** inside an isolated Docker container, orchestrated locally via **Docker Compose** and deployed globally on **Render Web Services** in Singapore.
4. **Relational Database Engine:** Persistent **SQLite 3** relational database configured with Write-Ahead Logging (`WAL` mode) and persistent disk mounting, ensuring time-series audit integrity and zero data loss across container restarts.
5. **Real-Time Web Dashboard:** A responsive, modern light theme telemetry dashboard featuring an SVG Arc Dial Gauge, 60-second real-time trend wave canvas, live database audit table, and zero mockup data.

---

## 📌 Compliance Matrix Against Project Criteria

| # | Assignment Criterion | Implementation in this Project | Verification Status |
| :-: | :--- | :--- | :---: |
| **1** | **ESP32 & Sensor Module** | ESP32 30-Pin NodeMCU DevKit V1 interfaced to an analog **MQ-2 Electrochemical Gas/Smoke Sensor** on `GPIO 34` (ADC1_CH6). | ✅ 100% COMPLETE |
| **2** | **Data Communication** | High-efficiency **HTTP REST / JSON** protocol transmitting structured telemetry (`POST /api/sensor-data`) to local Docker and live cloud endpoints. | ✅ 100% COMPLETE |
| **3** | **Containerized Backend** | Production **Dockerfile** (`python:3.12-slim`) and **`docker-compose.yml`** orchestrating the backend service with volume persistence and automated healthchecks. | ✅ 100% COMPLETE |
| **4** | **Database Support** | Relational **SQLite 3 database** (`sensor_database.db`) with tables: `sensor_readings`, `system_events`, and `system_config`, surviving restarts via persistent volume mounting. | ✅ 100% COMPLETE |
| **5** | **Real-Time Dashboard** | Interactive Web Dashboard with dynamic **SVG Arc Dial Gauge**, **60s Canvas Waveform**, **Live SQLite Database Table**, and **3D Animated LEDs**. | ✅ 100% COMPLETE |
| **6** | **Testing & Demonstration** | Complete verified data flow: `MQ-2 Sensor ➔ ESP32 ➔ Low-Latency REST Ingestion ➔ SQLite ➔ Render Cloud Dashboard`. Verified end-to-end with live hardware butane release. | ✅ 100% COMPLETE |

---

## 1. System Architecture & Topology

The system implements a four-tier IoT architecture: Physical Perception, Edge Controller, Cloud/Containerized Ingestion, and Visual Presentation.

```mermaid
graph LR
    subgraph Tier 1: Perception & Actuation
        MQ[MQ-2 Gas Sensor] -->|Analog 0-3.3V| ESP[ESP32 MCU GPIO 34 ADC1]
        ESP -->|Pulse Siren| BZ[Piezo Siren GPIO 25]
        ESP -->|Emergency Strobe| RLED[Red LED GPIO 26]
        ESP -->|Breathing Glow| GLED[Green LED GPIO 27]
    end

    subgraph Tier 2: Network Transmission
        ESP -->|HTTP POST JSON 5Hz| BRIDGE[High-Speed Serial Bridge]
        BRIDGE -->|HTTP Keep-Alive 88ms| CLOUD[Render Cloud HTTPS]
        ESP -.->|Direct 802.11b/g/n Wi-Fi| CLOUD
    end

    subgraph Tier 3: Containerized Backend & Database
        CLOUD --- DOCKER[Docker Container :5000]
        DOCKER --> API[Python REST Engine app.py]
        API -->|Insert Record| DB[(SQLite Database WAL Mode)]
        API -->|Audit Log| EV[Events Table]
        API -->|Config Setpoint| CFG[Config Table]
    end

    subgraph Tier 4: Real-Time Visualization
        DB -->|JSON Polling 250ms| DASH[Live Web Dashboard]
        DASH -->|Config & Test Drill| API
    end
```

---

## 2. Hardware Design & Sensing Physics

### 2.1 Bill of Materials (BOM)
* **Microcontroller:** ESP32 NodeMCU DevKit V1 (30-Pin), 240MHz Dual-Core Xtensa LX6, 4MB SPI Flash, 520KB SRAM.
* **Gas Sensor Module:** MQ-2 Chemiresistive Metal-Oxide Semiconductor (MOS) sensor.
* **Acoustic Actuator:** 5V Active Piezo Siren Buzzer (driven Active-LOW).
* **Optical Indicators:** 5mm Diffused Red LED (Hazard Strobe) and 5mm Diffused Green LED (Safe Pulse) with $330\ \Omega$ current-limiting resistors.
* **Power Supply:** 5V USB Bus from host computer or isolated 5V 2A DC supply.

### 2.2 Pin Allocation Table
| Component | Signal | ESP32 Pin | GPIO / ADC Type | Functional Description |
| :--- | :--- | :--- | :--- | :--- |
| **MQ-2 Gas Sensor** | `VCC` | `VIN` | 5.0V DC | Power supply for internal $SnO_2$ micro-heater coil |
| | `GND` | `GND` | Ground | Ground plane reference |
| | `AO` | **`GPIO 34`** | `ADC1_CH6` | Dedicated 12-bit analog gas concentration sampling ($0 - 4095$) |
| **Piezo Buzzer** | `+` / Signal | **`GPIO 25`** | Digital Output | 180ms pulse train alarm during hazard conditions |
| **Red Hazard LED** | Anode ($+$) | **`GPIO 26`** | Digital Output | High-frequency emergency optical strobe (Active-HIGH) |
| **Green Safe LED** | Anode ($+$) | **`GPIO 27`** | Digital Output | Steady illumination indicating nominal atmospheric baseline |

> [!IMPORTANT]
> **Why GPIO 34 (ADC1) is Critical:**  
> On the ESP32, ADC channels are split between `ADC1` and `ADC2`. `ADC2` channels (`GPIO 0, 2, 4, 12-15, 25-27`) share internal multiplexers with the Wi-Fi modem; calling `analogRead()` on ADC2 while Wi-Fi is transmitting fails or returns invalid zero values. Interfacing the MQ-2 to **`GPIO 34` (ADC1_CH6)** guarantees continuous, uninterrupted analog sampling regardless of Wi-Fi traffic.

### 2.3 Electrochemical Gas Sensing Physics
The MQ-2 sensor utilizes a micro-machined ceramic substrate coated with a thin layer of Tin Dioxide ($SnO_2$).

1. **Clean Air Phase (Oxygen Ion Adsorption):**  
   The internal heating coil heats the $SnO_2$ layer to $\approx 350^\circ\text{C}$. Atmospheric oxygen molecules are adsorbed onto the crystal surface and capture free conduction-band electrons from the semiconductor:
   $$O_2 + 2e^- \longrightarrow 2O^-_{\text{ads}}$$
   This creates a wide potential barrier (depletion region), giving the sensor a high baseline resistance ($R_0$).

2. **Reducing Gas Exposure Phase:**  
   When combustible gas molecules (such as Butane $C_4H_{10}$, Propane $C_3H_8$, or Hydrogen $H_2$) come into contact with the heated surface, they undergo oxidation with adsorbed oxygen ions:
   $$2C_4H_{10} + 13O^-_{\text{ads}} \longrightarrow 8CO_2 + 10H_2O + 13e^-$$
   The trapped electrons are released back into the $SnO_2$ conduction band, dramatically reducing the potential barrier and sensor resistance ($R_s$).

3. **Voltage Divider & ADC Conversion:**  
   The sensor board houses an adjustable load resistor ($R_L$) forming a voltage divider:
   $$V_{\text{out}} = V_{CC} \times \frac{R_L}{R_s + R_L}$$
   As gas concentration increases, $R_s$ drops, causing $V_{\text{out}}$ to rise proportionally.

4. **PPM Estimation Formula:**  
   The relationship between the resistance ratio $\frac{R_s}{R_0}$ and gas concentration in parts-per-million ($\text{PPM}$) follows a logarithmic power law:
   $$\log_{10}(\text{PPM}) = m \times \log_{10}\left(\frac{R_s}{R_0}\right) + c$$
   In firmware, the normalized gas concentration is scaled to an intuitive $0.0\% - 100.0\%$ range and mapped to estimated PPM:
   $$\text{Gas Percentage } (\%) = \left(\frac{V_{\text{ADC}}}{4095}\right) \times 100$$
   $$\text{Estimated PPM} = \left(\frac{\text{Gas } \%}{100}\right) \times 1200 + 15$$

---

## 3. Embedded Firmware Architecture

The ESP32 firmware ([`src/main.cpp`](file:///c:/Users/hp/Desktop/IOT%20Project/src/main.cpp)) is built on C++ / Arduino using PlatformIO.

```
       ┌────────────────────────────────────────────────────────┐
       │                ESP32 FreeRTOS Setup                    │
       │  - GPIO Initialization (34:IN, 25:OUT, 26:OUT, 27:OUT) │
       │  - Start 20-Second Heater Warmup Grace Period          │
       │  - Initialize SoftAP (ESP32-Gas-Detector) & Wi-Fi Client│
       └───────────────────────────┬────────────────────────────┘
                                   │
                                   ▼
       ┌────────────────────────────────────────────────────────┐
       │                 Main Loop (Non-blocking)               │
       │  - 100ms: Multi-sample ADC averaging                   │
       │  - Evaluate Gas % against dynamic Threshold Setpoint   │
       └───────────────────────────┬────────────────────────────┘
                                   │
                ┌──────────────────┴──────────────────┐
                ▼                                     ▼
        Gas >= Threshold                       Gas < Threshold
        (Hazard State)                         (Safe Baseline)
                │                                     │
      ┌─────────┴─────────┐                 ┌─────────┴─────────┐
      │  Status = DANGER  │                 │   Status = SAFE   │
      │  Red LED: STROBE  │                 │  Green LED: SOLID │
      │  Piezo: 180ms BEEP│                 │  Red LED: OFF     │
      │  Green LED: OFF   │                 │  Piezo: SILENT    │
      └─────────┬─────────┘                 └─────────┬─────────┘
                │                                     │
                └──────────────────┬──────────────────┘
                                   │
                                   ▼
       ┌────────────────────────────────────────────────────────┐
       │             200ms Serial Stream (COM5)                 │
       │   [LIVE] Gas: 18.5% | Raw ADC: 758 | Status: SAFE      │
       └────────────────────────────────────────────────────────┘
```

### Key Firmware Features:
* **20-Second Smart Warmup Grace Period:** Chemical MOS sensors experience a brief false surge upon initial cold startup. The firmware runs an internal 20-second timer during which readings are monitored but false alarms are suppressed.
* **Moving Average Digital Filter:** Samples `ADC1` ten times per acquisition cycle with a 2ms inter-sample delay to eliminate high-frequency thermal noise.
* **Active-Low Buzzer Drive Logic:** Accommodates standard 3-pin transistorized buzzer modules where setting the GPIO `LOW` energizes the buzzer.
* **Dual Access Point & Client Mode:** Simultaneously operates as a presentation hotspot (`ESP32-Gas-Detector` at `192.168.4.1`) for off-grid classroom demos while transmitting REST requests over Wi-Fi.

---

## 4. Containerized Backend & Database Architecture

The backend engine ([`backend/app.py`](file:///c:/Users/hp/Desktop/IOT%20Project/backend/app.py)) is implemented using Python 3.12 without heavy third-party framework dependencies, achieving startup in $< 300\text{ms}$ and minimal memory footprint ($< 35\text{MB}$).

### 4.1 Production Dockerfile
```dockerfile
FROM python:3.12-slim
WORKDIR /app
ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PORT=5000 \
    DATABASE_PATH=/app/data/sensor_database.db
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt || true
COPY . .
VOLUME ["/app/data"]
EXPOSE 5000
HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
  CMD python -c "import urllib.request, os; p = os.environ.get('PORT', '5000'); urllib.request.urlopen(f'http://localhost:{p}/health')" || exit 1
CMD ["python", "app.py"]
```

### 4.2 SQLite 3 Relational Database Persistence
Data persistence is handled by [`backend/database.py`](file:///c:/Users/hp/Desktop/IOT%20Project/backend/database.py) using SQLite 3 configured for high-concurrency production workloads:
* **Write-Ahead Logging (`WAL` mode):** Configured via `PRAGMA journal_mode=WAL;`, enabling simultaneous read and write operations without database locking contention.
* **Dynamic Persistence Path (`DATABASE_PATH`):** In local Docker, writes to `/app/data/sensor_database.db`. On Render with persistent disks, points to `/var/data/sensor_database.db`.
* **Automatic Schema Bootstrap:** Creates all tables automatically with `IF NOT EXISTS` on initial startup, ensuring no data is ever overwritten during redeploys.

```mermaid
erDiagram
    sensor_readings {
        INTEGER id PK "AUTOINCREMENT"
        TEXT timestamp "ISO-8601 Datetime"
        TEXT device_id "ESP32 Identifier"
        TEXT sensor_type "MQ-2"
        INTEGER raw_value "Raw ADC 0-4095"
        REAL gas_percent "Calculated 0-100%"
        INTEGER ppm_estimated "Estimated PPM"
        TEXT status "SAFE / DANGER"
        INTEGER fan_state "0 or 1"
        INTEGER buzzer_state "0 or 1"
    }

    system_events {
        INTEGER id PK "AUTOINCREMENT"
        TEXT timestamp "ISO-8601 Datetime"
        TEXT event_type "ALARM_TRIGGERED / CLEARED"
        TEXT message "Detailed Incident Log"
        TEXT severity "INFO / WARNING / CRITICAL"
    }

    system_config {
        TEXT key PK "threshold / buzzer_muted"
        TEXT value "Dynamic Setpoint Value"
    }

    sensor_readings ||--o{ system_events : "triggers on state change"
```

---

## 5. REST API Specification

| Method | Endpoint | Description | Request Payload | Response Sample |
| :--- | :--- | :--- | :--- | :--- |
| `POST` | `/api/sensor-data` | Ingests sensor reading from ESP32 | `{"device_id": "...", "raw_value": 750, "gas_percent": 18.3, "ppm_estimated": 183, "status": "SAFE"}` | `201 Created`<br>`{"status": "success", "id": 142, "threshold": 40}` |
| `GET` | `/health` | Production container healthcheck | None | `200 OK`<br>`{"status": "healthy", "database": "connected"}` |
| `GET` | `/api/sensor-data/latest` | Returns most recent database reading | None | `200 OK`<br>`{"raw_value": 750, "gas_percent": 18.3, ...}` |
| `GET` | `/api/sensor-data/history` | Fetches historical trend data for chart | `?limit=60` | `200 OK`<br>`{"count": 60, "readings": [...]}` |
| `GET` | `/api/statistics` | Returns aggregated metrics | None | `200 OK`<br>`{"total_readings": 234, "avg_gas_percent": 0.4, ...}` |
| `GET` | `/api/events` | Fetches system audit event log | None | `200 OK`<br>`[{"event_type": "ALARM_TRIGGERED", ...}]` |
| `POST` | `/api/config/threshold` | Updates alarm trigger setpoint | `{"threshold": 45}` | `200 OK`<br>`{"status": "success", "threshold": 45}` |
| `POST` | `/api/test-alarm` | Triggers 4-second emergency test drill | None | `200 OK`<br>`{"status": "testing", "duration_seconds": 4}` |
| `POST` | `/api/database/clear` | Purges records and resets sequence | None | `200 OK`<br>`{"status": "success"}` |
| `GET` | `/api/export/csv` | Exports full database to CSV | None | `200 OK`<br>`text/csv` attachment |

---

## 6. Real-Time Web Dashboard Engineering

The dashboard ([`backend/static/index.html`](file:///c:/Users/hp/Desktop/IOT%20Project/backend/static/index.html)) is built with clean HTML5, CSS3, Vanilla ES6+ JavaScript, and HTML Canvas.

```
┌────────────────────────────────────────────────────────────────────────┐
│  🚨 Aura GasGuard™ • IoT Hardware Sentinel              [LIVE CLOUD]   │
├───────────────────┬────────────────────────────────────────────────────┤
│                   │                                                    │
│   SVG ARC DIAL    │           60-SECOND TREND GRAPH (CANVAS)           │
│      GAUGE        │                                                    │
│                   │  ──────────────────────── Threshold Line (40%)     │
│       18.3%       │                                                    │
│    (183 PPM)      │  /\_/\__/\__                                       │
│                   │                                                    │
├───────────────────┴────────────────────────────────────────────────────┤
│  3D ANIMATED HARDWARE LEDS:  🟢 SAFE (Breathing)    🔴 DANGER (Strobe) │
├────────────────────────────────────────────────────────────────────────┤
│  LIVE SQLITE DATABASE RECORDS TABLE (Top 15 Real Hardware Readings)    │
│  ID    Timestamp            Raw ADC   Gas %   PPM    Status   Buzzer   │
│  142   2026-11-02 10:21:30   750       18.3%   183    SAFE     OFF     │
│  141   2026-11-02 10:21:28   748       18.2%   182    SAFE     OFF     │
└────────────────────────────────────────────────────────────────────────┘
```

### Key UI Features:
1. **Light Theme Visual Hierarchy:** Built on clean off-white backgrounds (`#f8fafc`, `#ffffff`), slate typography (`#0f172a`), and emerald/rose semantic state cards.
2. **SVG Arc Dial Gauge:** A continuous vector arc gauge with a rotating needle displaying the exact gas percentage and estimated PPM with zero raster pixelation.
3. **60-Second Real-Time Canvas Trend Chart:** A high-framerate HTML5 Canvas graph rendering an animated cubic spline wave of the last 60 seconds of sensor data with dynamic linear gradient fills.
4. **3D Animated Physical LEDs:** Realistic CSS3-modeled circular LEDs featuring layered radial gradients and `@keyframes` pulse glows that mirror the physical breadboard LEDs in real time.
5. **Zero Mockup Data Guarantee:** The dashboard renders strictly live database records; when awaiting first connection, all fields display clean `WAITING_DATA` state.
6. **Adaptive Polling Engine:** High-frequency 250ms polling loop ensuring that physical gas releases reflect on screen in $< 300\text{ms}$.

---

## 7. Cloud Deployment & High-Speed Bridge

### 7.1 Render Cloud Web Service
* **Deployment Type:** Managed Docker Web Service.
* **Infrastructure-as-Code:** Defined declaratively in [`render.yaml`](file:///c:/Users/hp/Desktop/IOT%20Project/render.yaml).
* **Region:** Singapore (`ap-southeast-1`), delivering sub-100ms ping times to Southeast Asia and India.
* **Automated CI/CD:** Connected to GitHub repository `subinjoseph002/Aura-gasguard`; any push to `main` automatically triggers a zero-downtime rebuild.

### 7.2 High-Speed Serial-to-Cloud Bridge (`serial_bridge.py`)
To route live physical hardware telemetry from local USB (`COM5`) to the Render cloud without requiring campus/home Wi-Fi credentials, the system features a dedicated serial bridge:
* **Persistent HTTP Keep-Alive Connection Pooling:** Built with `requests.Session()`. Instead of establishing a new TLS handshake for every packet (480ms), the bridge keeps the TCP/TLS socket open, cutting round-trip transmission latency down to **88ms per packet**.
* **Throughput:** Transmits at 5 Hz (5 packets per second), streaming over 250+ live records into the cloud database with zero dropouts.

---

## 8. Experimental Verification & Test Results

### 8.1 Nominal Clean Air Baseline
* **Raw ADC:** $10 - 60$ counts.
* **Gas Percentage:** $0.2\% - 1.4\%$.
* **Estimated PPM:** $15 - 30\text{ PPM}$.
* **System Actuation:** Green LED solid breathing glow, Red LED OFF, Piezo siren silent.
* **Cloud Sync:** Status `201 Created` recorded on every packet.

### 8.2 Induced Butane Gas Exposure Test
* **Method:** Controlled release of butane gas near the MQ-2 sensor mesh.
* **Observations:**
  1. Sensor voltage surged immediately from 0.05V to 2.45V (Raw ADC: $3045$).
  2. Gas percentage crossed the 40% setpoint to **$74.3\%$** within $180\text{ms}$.
  3. Edge response: Green LED cut off; Red LED engaged high-frequency strobe; Piezo buzzer sounded emergency pulse train.
  4. Backend response: Ingested packet, inserted record into `sensor_readings`, and logged `ALARM_TRIGGERED` event with `CRITICAL` severity in `system_events`.
  5. Dashboard response: SVG Dial Gauge rotated into red zone, background flashed soft emergency red, and Live Records Table inserted the incident row within 250ms.

### 8.3 Post-Exposure Gas Dissipation & Hysteresis Recovery
* **Observations:**
  1. Once butane source was removed, atmospheric airflow lowered the concentration.
  2. Firmware hysteresis (3% deadband) prevented rapid flapping around the 40% boundary.
  3. When gas fell below 37%, status transitioned to `SAFE`.
  4. Backend logged `ALARM_CLEARED` event. Green LED resumed steady illumination.

---

## 9. Viva Voce & Oral Examination Q&A Guide

**Q1: Why did you choose the ESP32 over the Arduino Uno or Raspberry Pi?**  
*Answer:* The ESP32 provides built-in 802.11 b/g/n Wi-Fi and Bluetooth on a dual-core 240MHz processor for a fraction of the cost of a Raspberry Pi, while offering 12-bit ADC resolution ($0-4095$) compared to the Arduino Uno's 10-bit ($0-1023$). It easily handles high-frequency analog sampling and networking simultaneously without needing external Wi-Fi shields.

**Q2: Why did you interface the MQ-2 to GPIO 34 specifically?**  
*Answer:* The ESP32 has two separate ADC peripherals: ADC1 and ADC2. ADC2 channels share internal hardware with the Wi-Fi modem and are disabled whenever Wi-Fi is active. GPIO 34 belongs to ADC1 (`ADC1_CH6`), guaranteeing that analog gas sensing remains 100% operational during Wi-Fi transmissions.

**Q3: How does the MQ-2 sensor physically detect gas?**  
*Answer:* It operates on the chemiresistance of a heated Tin Dioxide ($SnO_2$) semiconductor layer. In clean air, adsorbed oxygen forms a potential barrier, keeping resistance high. Reducing gases (like Butane, LPG, or Methane) react with the adsorbed oxygen, releasing electrons back into the semiconductor and lowering its electrical resistance proportionally to gas concentration.

**Q4: Why is a 20-second warmup period necessary in firmware?**  
*Answer:* The MQ-2 requires an internal heating coil to maintain the $SnO_2$ layer at approximately $350^\circ\text{C}$ for chemical reactions to occur. Upon cold boot, the unheated surface produces erratic resistance drops that would trigger false emergency alarms. The 20-second grace period stabilizes the heater before enabling alarm triggers.

**Q5: Why did you use SQLite instead of MongoDB or PostgreSQL?**  
*Answer:* SQLite is a serverless, zero-configuration, ACID-compliant relational engine stored in a single binary file. For embedded IoT sentinel gateways and containerized edge nodes, it delivers microsecond read/write access with zero network overhead and no background daemon process. Using Write-Ahead Logging (`WAL` mode) allows concurrent reading and writing without lock contention.

**Q6: How does the application preserve SQLite data across Docker / Render restarts?**  
*Answer:* Containers have ephemeral filesystems by default. We solved this by parameterizing the database path via the `DATABASE_PATH` environment variable and configuring persistent volume mounts (`/app/data` in Docker Compose and `/var/data` with Render Persistent Disks). The database schema initialization uses `CREATE TABLE IF NOT EXISTS`, preventing overwrites on startup.

**Q7: How did you achieve 88ms latency between your local hardware and Render cloud?**  
*Answer:* Standard HTTP POST requests perform a new TCP handshake and TLS cryptographic negotiation on every single request, taking 300–500ms over international connections. By implementing HTTP Keep-Alive connection pooling with Python's `requests.Session()`, the TCP/TLS connection to Singapore is reused, dropping round-trip packet latency to just 88ms.

**Q8: What is the purpose of the healthcheck endpoint (`/health`)?**  
*Answer:* Production cloud platforms (like Render and Docker) continuously ping `/health`. The endpoint executes a live `SELECT 1` query against the SQLite database to confirm both the web server and database layer are healthy. If the container freezes or database locks, Render automatically restarts the container to guarantee high availability.

---

## 10. Conclusion & Future Scope

The **Aura GasGuard™** system successfully fulfills all academic and practical engineering criteria for modern IoT systems:
* **Perception:** Accurate 12-bit analog electrochemical gas sensing on dedicated `ADC1`.
* **Reliability:** 20-second warmup false-alarm suppression and active edge buzzer/LED drivers.
* **Connectivity:** Ultra-low latency HTTP REST telemetry with persistent session connection pooling.
* **Storage:** Production-grade SQLite relational persistence with WAL mode.
* **Cloud Engineering:** Production Dockerfile, multi-environment Docker Compose, and automated CI/CD deployment on Render.
* **User Experience:** Clean, responsive light theme dashboard with animated dials, trend waves, and zero mockup data.

### Future Enhancements:
1. **MQTT Telemetry Protocol:** Implementing an MQTT broker (Eclipse Mosquitto) for pub/sub telemetry distribution to thousands of simultaneous subscribers.
2. **Automated Twilio SMS / WhatsApp Push Notifications:** Triggering instant emergency SMS alerts to facility managers when critical thresholds are breached.
3. **Machine Learning Gas Classification:** Utilizing multi-sensor arrays (MQ-2, MQ-7, MQ-135) with edge TinyML models to distinguish specific gas species (e.g., differentiating cooking alcohol vapors from dangerous LPG leaks).
