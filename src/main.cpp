/*
 * =====================================================================================
 *  IoT Gas Leakage Detection & Safety System using ESP32 (30-Pin DevKit)
 *  PlatformIO Edition - Firmware Version 3.1
 * =====================================================================================
 *  Features:
 *    1. ESP32 & MQ-2 Sensor Acquisition (ADC1 GPIO 34)
 *    2. 20-Second Smart Warmup Grace Period (Eliminates power-on false alarms)
 *    3. Active-HIGH / Active-LOW Buzzer Configurable Driver
 *    4. Periodic HTTP REST JSON Telemetry to Docker / Backend Server (:5000)
 *    5. Dual Wi-Fi Mode (Presentation Hotspot + Home Wi-Fi)
 *    6. Embedded Fallback Real-Time Web Dashboard (http://192.168.4.1)
 *    7. Non-blocking FreeRTOS architecture
 * =====================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include "web_dashboard.h"

// =====================================================================================
// PIN CONFIGURATION (Optimized for 30-Pin ESP32 NodeMCU DevKit V1)
// =====================================================================================
// ADC1 pins (GPIO 32 - 39) MUST be used for analog inputs when Wi-Fi is active.
#define PIN_MQ_ANALOG     34   // MQ Sensor Analog Out (AO) -> GPIO 34 (ADC1_CH6)
#define PIN_MQ_DIGITAL    35   // MQ Sensor Digital Out (DO) -> GPIO 35 (Optional)
#define PIN_BUZZER        25   // Active or Passive Buzzer -> GPIO 25
#define PIN_LED_RED       26   // Red Alert LED -> GPIO 26
#define PIN_LED_GREEN     27   // Green Normal/Safe LED -> GPIO 27

// Set to 1 if your buzzer module is Active-LOW (most 3-pin buzzer modules are active-low)
#define BUZZER_ACTIVE_LOW 1

// =====================================================================================
// WI-FI & BACKEND CONFIGURATION
// =====================================================================================
// Home Wi-Fi credentials:
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Presentation SoftAP credentials (always active everywhere)
const char* AP_SSID       = "ESP32-Gas-Detector";
const char* AP_PASSWORD   = "12345678";

// Backend Server URL (Requirement 2: Data Communication to Backend)
// Deployed production cloud URL on Render:
const char* BACKEND_SERVER_URL = "https://aura-gasguard.onrender.com/api/sensor-data";

WebServer server(80);

// =====================================================================================
// GLOBAL VARIABLES & SYSTEM STATE
// =====================================================================================
float gasThresholdPercent  = 40.0; // Default alarm threshold percentage (40%)
float gasPercent           = 0.0;
int   gasRawValue          = 0;
bool  isGasDanger          = false;
bool  isBuzzerMuted        = false;
bool  alarmTestActive      = false;
unsigned long testAlarmEnd = 0;

// Timing counters (non-blocking)
unsigned long bootTime           = 0;
unsigned long lastSensorReadTime = 0;
unsigned long lastBuzzerToggle   = 0;
unsigned long lastBackendTxTime  = 0;
unsigned long lastSerialPrint    = 0;
bool buzzerState                 = false;

// 20-Second Warmup Grace Period: MQ-2 heater needs ~20s to stabilize on boot
const unsigned long WARMUP_DURATION_MS = 20000;

// =====================================================================================
// HELPER FUNCTIONS
// =====================================================================================

void setBuzzer(bool state) {
  if (isBuzzerMuted || !state) {
#if BUZZER_ACTIVE_LOW
    digitalWrite(PIN_BUZZER, HIGH);
#else
    digitalWrite(PIN_BUZZER, LOW);
#endif
    return;
  }
#if BUZZER_ACTIVE_LOW
  digitalWrite(PIN_BUZZER, LOW);
#else
  digitalWrite(PIN_BUZZER, HIGH);
#endif
}

// Multi-sample ADC reading to filter electrical noise
int readSmoothADC(int pin, int samples = 10) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delayMicroseconds(50);
  }
  return (int)(sum / samples);
}

// Add CORS headers to all responses
void setCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// Requirement 2: Transmit Telemetry to Containerized Backend API
void transmitSensorDataToBackend() {
  if (WiFi.status() != WL_CONNECTED) return; // Only transmit when connected to Wi-Fi station

  HTTPClient http;
  http.begin(BACKEND_SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(1200);

  int ppm = (int)((gasPercent / 100.0f) * 1200 + 15);
  String statusStr = isGasDanger ? "DANGER" : ((gasPercent >= gasThresholdPercent * 0.75f) ? "WARNING" : "SAFE");

  String json = "{";
  json += "\"device_id\":\"ESP32-GAS-01\",";
  json += "\"sensor_type\":\"MQ-2\",";
  json += "\"raw_value\":" + String(gasRawValue) + ",";
  json += "\"gas_percent\":" + String(gasPercent, 1) + ",";
  json += "\"ppm_estimated\":" + String(ppm) + ",";
  json += "\"status\":\"" + statusStr + "\",";
  json += "\"buzzer_state\":" + String(buzzerState ? "true" : "false") + ",";
  json += "\"threshold\":" + String((int)gasThresholdPercent);
  json += "}";

  int httpCode = http.POST(json);
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      // Data persisted to SQLite database on backend successfully
    }
  }
  http.end();
}

// =====================================================================================
// WEB SERVER API HANDLERS (Local Fallback Dashboard)
// =====================================================================================
void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatusApi() {
  setCORSHeaders();
  String statusStr = "SAFE";
  if (isGasDanger || alarmTestActive) {
    statusStr = "DANGER";
  } else if (gasPercent >= (gasThresholdPercent * 0.75f)) {
    statusStr = "WARNING";
  }

  int wifiRssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;

  String json = "{";
  json += "\"gasRaw\":" + String(gasRawValue) + ",";
  json += "\"gasPercent\":" + String(gasPercent, 1) + ",";
  json += "\"threshold\":" + String((int)gasThresholdPercent) + ",";
  json += "\"status\":\"" + statusStr + "\",";
  json += "\"buzzer\":" + String(buzzerState ? "true" : "false") + ",";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"wifiRssi\":" + String(wifiRssi) + ",";
  json += "\"freeHeap\":" + String(ESP.getFreeHeap());
  json += "}";

  server.send(200, "application/json", json);
}

void handleConfigApi() {
  setCORSHeaders();
  if (server.hasArg("threshold")) {
    int val = server.arg("threshold").toInt();
    if (val >= 10 && val <= 95) {
      gasThresholdPercent = (float)val;
      Serial.printf("[CONFIG] Alarm threshold updated to %d%%\n", val);
    }
  }
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleMuteApi() {
  setCORSHeaders();
  if (server.hasArg("state")) {
    isBuzzerMuted = (server.arg("state") == "1");
    if (isBuzzerMuted) {
      setBuzzer(false);
      buzzerState = false;
    }
    Serial.printf("[CONTROL] Buzzer Mute: %s\n", isBuzzerMuted ? "MUTED" : "UNMUTED");
  }
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleTestAlarm() {
  setCORSHeaders();
  alarmTestActive = true;
  testAlarmEnd = millis() + 4000; // 4 seconds test
  Serial.println("[TEST] Manual alarm test triggered for 4 seconds");
  server.send(200, "application/json", "{\"status\":\"testing\"}");
}

void handleOptions() {
  setCORSHeaders();
  server.send(204);
}

// =====================================================================================
// SETUP
// =====================================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  bootTime = millis();

  Serial.println("\n=======================================================");
  Serial.println("  ESP32 30-Pin Gas Leakage Detection System Initializing");
  Serial.println("  Firmware Version 3.1 • Smart Warmup & Diagnostics");
  Serial.println("=======================================================");

  // Initialize GPIO Pins
  pinMode(PIN_MQ_ANALOG, INPUT);
  pinMode(PIN_MQ_DIGITAL, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  // Default states
  setBuzzer(false);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, HIGH); // Safe LED ON

  // ADC Calibration for ESP32 (12-bit: 0 - 4095, 3.3V attenuation)
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db); // Full range 0 - 3.3V

  // Dual Wi-Fi Mode: Both Classroom Presentation Hotspot AND Home Wi-Fi active concurrently
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println("[WiFi] Presentation Hotspot Active Everywhere!");
  Serial.printf("       SSID: '%s'  (Password: '%s')\n", AP_SSID, AP_PASSWORD);
  Serial.print("       Classroom / Direct Access: http://");
  Serial.println(WiFi.softAPIP());

  // Attempt Home Wi-Fi connection if credentials are configured
  if (String(WIFI_SSID) != "YOUR_WIFI_SSID" && String(WIFI_SSID).length() > 0) {
    Serial.printf("[WiFi] Connecting to Home Wi-Fi: %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long startWifi = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startWifi) < 6000) {
      delay(300);
      Serial.print(".");
      digitalWrite(PIN_LED_GREEN, !digitalRead(PIN_LED_GREEN));
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n[WiFi] Connected to Home Wi-Fi successfully!");
      Serial.print("[WiFi] Home Network URL: http://");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\n[WiFi] Home Wi-Fi not in range. (Classroom mode active)");
    }
  } else {
    Serial.println("[WiFi] Running in Presentation Hotspot mode.");
  }
  digitalWrite(PIN_LED_GREEN, HIGH);

  // Start mDNS responder (http://gasguard.local)
  if (MDNS.begin("gasguard")) {
    Serial.println("[mDNS] Domain active: http://gasguard.local");
  }

  // Setup Local Web Server Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatusApi);
  server.on("/api/status", HTTP_OPTIONS, handleOptions);
  server.on("/api/config", HTTP_POST, handleConfigApi);
  server.on("/api/config", HTTP_OPTIONS, handleOptions);
  server.on("/api/mute", HTTP_POST, handleMuteApi);
  server.on("/api/mute", HTTP_OPTIONS, handleOptions);
  server.on("/api/test", HTTP_POST, handleTestAlarm);
  server.on("/api/test", HTTP_OPTIONS, handleOptions);

  server.begin();
  Serial.println("[HTTP] Local Web Server online. Ready to detect gas leakage.");
  Serial.println("[INFO] MQ-2 Sensor 20s warmup in progress. Buzzer held OFF...\n");
}

// =====================================================================================
// MAIN LOOP (Non-blocking)
// =====================================================================================
void loop() {
  server.handleClient();
  unsigned long currentMillis = millis();
  bool isWarmingUp = (currentMillis - bootTime < WARMUP_DURATION_MS);

  // Test Alarm Timer Check
  if (alarmTestActive && currentMillis >= testAlarmEnd) {
    alarmTestActive = false;
    Serial.println("[TEST] Alarm test completed");
  }

  // Read MQ-2 Sensor every 100ms (High-Frequency Sampling)
  if (currentMillis - lastSensorReadTime >= 100) {
    lastSensorReadTime = currentMillis;

    gasRawValue = readSmoothADC(PIN_MQ_ANALOG, 8);
    // Convert 12-bit ADC (0 - 4095) to percentage (0.0% - 100.0%)
    gasPercent = (gasRawValue / 4095.0f) * 100.0f;

    // During warmup (first 20 seconds), ignore false spikes while heater stabilizes
    if (isWarmingUp) {
      isGasDanger = false;
    } else {
      // Hysteresis threshold logic:
      if (!isGasDanger && (gasPercent >= gasThresholdPercent)) {
        isGasDanger = true;
        Serial.printf("[ALARM TRIGGERED] Gas Level: %.1f%% (Raw: %d)\n", gasPercent, gasRawValue);
        transmitSensorDataToBackend();
      } else if (isGasDanger && (gasPercent < (gasThresholdPercent - 3.0f))) {
        isGasDanger = false;
        Serial.printf("[ALARM CLEARED] Gas Level returned to normal: %.1f%%\n", gasPercent);
        transmitSensorDataToBackend();
      }
    }
  }

  // Periodic Telemetry to Backend Server over Wi-Fi (every 1 second)
  if (currentMillis - lastBackendTxTime >= 1000) {
    lastBackendTxTime = currentMillis;
    if (!isWarmingUp) {
      transmitSensorDataToBackend();
    }
  }

  // Ultra-Low Latency Serial Stream (every 200ms = 5 times per second)
  if (currentMillis - lastSerialPrint >= 200) {
    lastSerialPrint = currentMillis;
    if (isWarmingUp) {
      int remainSec = (WARMUP_DURATION_MS - (currentMillis - bootTime)) / 1000;
      Serial.printf("[WARMUP] Heating MQ-2 (%2ds left) | Raw ADC: %4d (%4.1f%%) | Alarm: HELD OFF\n",
                    remainSec, gasRawValue, gasPercent);
    } else {
      Serial.printf("[LIVE] Gas: %4.1f%% | Raw ADC: %4d | Limit: %d%% | Status: %-7s\n",
                    gasPercent, gasRawValue, (int)gasThresholdPercent,
                    isGasDanger ? "DANGER" : "SAFE");
    }
  }

  // Actuator Alarm Logic (Buzzer & LEDs)
  bool triggerAlarm = (!isWarmingUp && isGasDanger) || alarmTestActive;

  if (triggerAlarm) {
    // Pulse buzzer & flash red LED every 180ms for an urgent intermittent siren sound
    if (currentMillis - lastBuzzerToggle >= 180) {
      lastBuzzerToggle = currentMillis;
      buzzerState = !buzzerState;

      setBuzzer(buzzerState);
      digitalWrite(PIN_LED_RED, buzzerState ? HIGH : LOW);
      digitalWrite(PIN_LED_GREEN, LOW); // Turn off safe green LED
    }
  } else {
    // Normal / Safe State (or during Warmup)
    setBuzzer(false);
    digitalWrite(PIN_LED_RED, LOW);
    if (isWarmingUp) {
      // Blink green LED gently during warmup
      digitalWrite(PIN_LED_GREEN, ((currentMillis / 500) % 2 == 0) ? HIGH : LOW);
    } else {
      digitalWrite(PIN_LED_GREEN, HIGH);
    }
    buzzerState = false;
  }
}
