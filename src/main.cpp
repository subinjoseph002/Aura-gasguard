/*
 * =====================================================================================
 *  Aura GasGuard™ - IoT Gas Leakage Detection System
 *  ESP32 30-Pin DevKit V1 - Production Firmware v3.3
 * =====================================================================================
 *  Pin Mapping Verified by Live Hardware Diagnostic:
 *    - MQ-2 Analog Out (AO) -> GPIO 32 (ADC1_CH4, active reading: 750 clean air)
 *    - MQ-2 Fallback AO     -> GPIO 34 (ADC1_CH6)
 *    - MQ-2 Digital Out (DO)-> GPIO 35 (Active-LOW gas comparator)
 *    - Alarm Buzzer         -> GPIO 25 (2.4kHz pulsed siren)
 *    - Red Alert LED        -> GPIO 26 (Emergency strobe)
 *    - Green Safe LED       -> GPIO 27 (Normal status indicator)
 * =====================================================================================
 *  Alarm Trigger Specification:
 *    - Normal room air baseline: ~0% - 2% (Safe, Green LED ON, Buzzer OFF)
 *    - Alarm Threshold: 10% gas concentration
 *    - Trigger: When reading hits >= 10.0%, BUZZER SOUNDS & RED LED FLASHES
 * =====================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include "web_dashboard.h"

// Hardware Pin Configuration
#define PIN_MQ_ANALOG     32   // Primary MQ Sensor AO -> GPIO 32
#define PIN_MQ_FALLBACK   34   // Fallback MQ Sensor AO -> GPIO 34
#define PIN_MQ_DIGITAL    35   // MQ Sensor Digital DO -> GPIO 35
#define PIN_BUZZER        25   // Buzzer -> GPIO 25
#define PIN_LED_RED       26   // Red Alert LED -> GPIO 26
#define PIN_LED_GREEN     27   // Green Safe LED -> GPIO 27

// SoftAP Hotspot credentials
const char* AP_SSID       = "ESP32-Gas-Detector";
const char* AP_PASSWORD   = "12345678";

// Cloud Backend REST API URL
const char* BACKEND_SERVER_URL = "https://aura-gasguard.onrender.com/api/sensor-data";

WebServer server(80);

// Global Variables & State
float gasThresholdPercent  = 10.0; // Trigger alarm at 10% gas concentration
float gasPercent           = 0.0;
int   gasRawValue          = 0;
int   cleanAirBase         = 740;  // Clean air baseline (~740 counts measured on GPIO 32)
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

// 4-Second Warmup Grace Period on Boot
const unsigned long WARMUP_DURATION_MS = 4000;

void setBuzzer(bool state) {
  if (isBuzzerMuted || !state) {
    noTone(PIN_BUZZER);
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
    return;
  }
  // Loud 2.4kHz oscillating acoustic siren (supports both passive and active buzzers)
  tone(PIN_BUZZER, 2400);
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

void setCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void transmitSensorDataToBackend() {
  if (WiFi.status() != WL_CONNECTED) return;

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

  http.POST(json);
  http.end();
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatusApi() {
  setCORSHeaders();
  String statusStr = isGasDanger ? "DANGER" : ((gasPercent >= gasThresholdPercent * 0.75f) ? "WARNING" : "SAFE");
  String json = "{";
  json += "\"gasRaw\":" + String(gasRawValue) + ",";
  json += "\"gasPercent\":" + String(gasPercent, 1) + ",";
  json += "\"threshold\":" + String((int)gasThresholdPercent) + ",";
  json += "\"status\":\"" + statusStr + "\",";
  json += "\"buzzer\":" + String(buzzerState ? "true" : "false") + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";
  server.send(200, "application/json", json);
}

void handleConfigApi() {
  setCORSHeaders();
  if (server.hasArg("threshold")) {
    int val = server.arg("threshold").toInt();
    if (val >= 5 && val <= 95) {
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
  }
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleTestAlarm() {
  setCORSHeaders();
  alarmTestActive = true;
  testAlarmEnd = millis() + 4000;
  server.send(200, "application/json", "{\"status\":\"testing\"}");
}

void handleOptions() {
  setCORSHeaders();
  server.send(204);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  bootTime = millis();

  Serial.println("\n=======================================================");
  Serial.println("  AURA GASGUARD - ESP32 GAS LEAKAGE DETECTION SYSTEM   ");
  Serial.println("  Sensor: GPIO 32 | Buzzer: GPIO 25 | Threshold: 10%   ");
  Serial.println("=======================================================");

  // Initialize GPIO Pins
  pinMode(PIN_MQ_ANALOG, INPUT);
  pinMode(PIN_MQ_FALLBACK, INPUT);
  pinMode(PIN_MQ_DIGITAL, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  // Default Pin States
  setBuzzer(false);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, HIGH);

  // Quick Hardware Self-Test (350ms audible confirmation):
  Serial.println("[BOOT] Running hardware self-test (LEDs & Buzzer)...");
  digitalWrite(PIN_LED_RED, HIGH);
  digitalWrite(PIN_LED_GREEN, HIGH);
  setBuzzer(true);
  delay(350);
  setBuzzer(false);
  digitalWrite(PIN_LED_RED, LOW);
  Serial.println("[BOOT] Self-test complete.");

  // ADC Calibration
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Wi-Fi SoftAP
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println("[WiFi] Access Point Active: http://192.168.4.1");

  // mDNS
  if (MDNS.begin("gasguard")) {
    Serial.println("[mDNS] http://gasguard.local");
  }

  // Web Server Routes
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

  Serial.println("[HTTP] Local Web Server online.");
  Serial.println("[INFO] Stabilizing baseline... Ready in 4 seconds.\n");
}

void loop() {
  server.handleClient();
  unsigned long currentMillis = millis();
  bool isWarmingUp = (currentMillis - bootTime < WARMUP_DURATION_MS);

  if (alarmTestActive && currentMillis >= testAlarmEnd) {
    alarmTestActive = false;
  }

  // Read MQ-2 Sensor every 100ms
  if (currentMillis - lastSensorReadTime >= 100) {
    lastSensorReadTime = currentMillis;

    // Read GPIO 32 (primary) and GPIO 34 (fallback)
    int val32 = readSmoothADC(PIN_MQ_ANALOG, 8);
    int val34 = readSmoothADC(PIN_MQ_FALLBACK, 8);

    if (val32 > 100) {
      gasRawValue = val32;
    } else if (val34 > 100) {
      gasRawValue = val34;
    } else {
      gasRawValue = max(val32, val34);
    }

    // Dynamic Ambient Baseline tracking
    if (isWarmingUp) {
      if (gasRawValue > 100 && gasRawValue < cleanAirBase) {
        cleanAirBase = gasRawValue;
      }
    } else {
      if (gasRawValue > 100 && gasRawValue < cleanAirBase) {
        cleanAirBase = gasRawValue;
      }
    }

    // Compute relative Gas Concentration (0.0% to 100.0%)
    if (gasRawValue > cleanAirBase) {
      gasPercent = ((float)(gasRawValue - cleanAirBase) / (float)(4095 - cleanAirBase)) * 100.0f;
    } else {
      gasPercent = 0.0f;
    }
    if (gasPercent > 100.0f) gasPercent = 100.0f;

    // Check DO pin (Digital Out, LOW = gas detected by comparator)
    bool digitalTrigger = (digitalRead(PIN_MQ_DIGITAL) == LOW);

    // CRITICAL REQUIREMENT: WHEN READING HITS 10 (or 10%), TRIGGER BUZZER!
    if (isWarmingUp) {
      isGasDanger = false;
    } else {
      // Trigger threshold: gasPercent >= 10.0% OR digital comparator trigger:
      if (!isGasDanger && (gasPercent >= gasThresholdPercent || digitalTrigger)) {
        isGasDanger = true;
        Serial.printf("\n[ALARM TRIGGERED] >>> READING HIT %.1f%% (Limit: %.1f%%, Raw: %d) - BUZZER ACTIVATED <<<\n\n",
                      gasPercent, gasThresholdPercent, gasRawValue);
        transmitSensorDataToBackend();
      } else if (isGasDanger && (gasPercent < (gasThresholdPercent - 2.5f) && !digitalTrigger)) {
        isGasDanger = false;
        Serial.printf("\n[ALARM CLEARED] Gas cleared: %.1f%% - BUZZER DEACTIVATED\n\n", gasPercent);
        transmitSensorDataToBackend();
      }
    }
  }

  // Periodic Telemetry to Backend Server (every 1s)
  if (currentMillis - lastBackendTxTime >= 1000) {
    lastBackendTxTime = currentMillis;
    if (!isWarmingUp) {
      transmitSensorDataToBackend();
    }
  }

  // Serial Stream every 200ms
  if (currentMillis - lastSerialPrint >= 200) {
    lastSerialPrint = currentMillis;
    if (isWarmingUp) {
      int remainSec = (int)(WARMUP_DURATION_MS - (currentMillis - bootTime)) / 1000;
      Serial.printf("[WARMUP] Stabilizing (%ds) | Raw ADC: %4d (%4.1f%%) | Limit: %d%% | Alarm: OFF\n",
                    remainSec, gasRawValue, gasPercent, (int)gasThresholdPercent);
    } else {
      Serial.printf("[LIVE] Gas: %4.1f%% | Raw ADC: %4d | Limit: %d%% | Status: %-7s\n",
                    gasPercent, gasRawValue, (int)gasThresholdPercent,
                    isGasDanger ? "DANGER" : "SAFE");
    }
  }

  // Actuator Alarm Logic (Buzzer & LEDs)
  bool triggerAlarm = (!isWarmingUp && isGasDanger) || alarmTestActive;

  if (triggerAlarm) {
    // Pulse buzzer & flash red LED every 180ms
    if (currentMillis - lastBuzzerToggle >= 180) {
      lastBuzzerToggle = currentMillis;
      buzzerState = !buzzerState;

      setBuzzer(buzzerState);
      digitalWrite(PIN_LED_RED, buzzerState ? HIGH : LOW);
      digitalWrite(PIN_LED_GREEN, LOW); // Green safe LED OFF
    }
  } else {
    // Safe State
    setBuzzer(false);
    digitalWrite(PIN_LED_RED, LOW);
    if (isWarmingUp) {
      digitalWrite(PIN_LED_GREEN, ((currentMillis / 500) % 2 == 0) ? HIGH : LOW);
    } else {
      digitalWrite(PIN_LED_GREEN, HIGH); // Green safe LED ON
    }
    buzzerState = false;
  }
}
