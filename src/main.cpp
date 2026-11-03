/*
 * =====================================================================================
 *  Aura GasGuard - IoT Gas Leakage Detection System
 *  ESP32 30-Pin DevKit V1 - Firmware v4.0
 * =====================================================================================
 *  All pins / polarity / thresholds live in include/config.h.
 *  Wi-Fi credentials (optional) live in include/secrets.h (see secrets.example.h).
 *
 *  Safety logic
 *    - 20 s warm-up: alarm disabled, clean-air baseline measured from the last 3 s.
 *    - Gas % = (ADC - baseline) / (4095 - baseline), alarm when >= threshold (default 10 %)
 *      for 3 consecutive samples (300 ms), cleared with 2.5 % hysteresis.
 *    - Sensor wire open / unpowered => FAULT (red LED slow blink, reported to backend).
 *    - Buzzer polarity handled in ONE place (driveBuzzer) - idle level is always "off".
 *    - Cloud telemetry runs in its own FreeRTOS task so HTTPS never delays the alarm.
 * =====================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <esp_system.h>
#include "config.h"
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#include "web_dashboard.h"

WebServer server(80);

// ---- Shared state (written by loop(), read by telemetry task / web handlers) ----------
volatile float gasThresholdPercent = DEFAULT_THRESHOLD_PCT;
volatile float gasPercent          = 0.0f;
volatile int   gasRawValue         = 0;
volatile bool  isGasDanger         = false;
volatile bool  sensorFault         = false;
volatile bool  isBuzzerMuted       = false;
volatile bool  buzzerAudible       = false;
volatile bool  fanOn               = false;
volatile bool  isWarmingUp         = true;
volatile bool  telemetryNow        = false;   // request an immediate upload (state change)

int           sensorPin      = PIN_MQ_A;
float         cleanAirBase   = 0.0f;
long          baseSum        = 0;
int           baseCount      = 0;
bool          windowStarted  = false;
bool          calibrated     = false;
int           overCount      = 0;
int           faultCount     = 0;
bool          wasFault       = false;
unsigned long settleUntil    = 0;
unsigned long lastRedetect   = 0;
bool          prevAlarm      = false;
bool          fanManual      = false;
bool          alarmTestActive = false;
unsigned long testAlarmEnd   = 0;
unsigned long bootTime = 0, lastSensorRead = 0, lastBeat = 0, lastSerial = 0;
bool          beat = false;

// ---- Buzzer: the ONLY place that knows the polarity -----------------------------------
void driveBuzzer(bool on) {
  static int8_t last = -1;
  if (last == (on ? 1 : 0)) return;
  last = on ? 1 : 0;
#if BUZZER_PASSIVE
  if (on) { tone(PIN_BUZZER, BUZZER_TONE_HZ); return; }
  noTone(PIN_BUZZER);
  pinMode(PIN_BUZZER, OUTPUT);
#endif
  digitalWrite(PIN_BUZZER, ((on ? 1 : 0) ^ BUZZER_ACTIVE_LOW) ? HIGH : LOW);
}

void driveRelay(bool on) {
  digitalWrite(PIN_RELAY, ((on ? 1 : 0) ^ RELAY_ACTIVE_LOW) ? HIGH : LOW);
  fanOn = on;
}

int readAdc(int pin, int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) { sum += analogRead(pin); delayMicroseconds(50); }
  return (int)(sum / samples);
}

// Decide whether the MQ AO is wired to GPIO 34 or GPIO 32. GPIO 32 gets an internal
// pull-down: a floating (unconnected) pin collapses to ~0, a pin driven by the sensor does not.
void detectSensor() {
  pinMode(PIN_MQ_B, INPUT_PULLDOWN);
  delay(30);
  int b = readAdc(PIN_MQ_B, 32);
  pinMode(PIN_MQ_B, INPUT);
  int a = readAdc(PIN_MQ_A, 32);
  sensorPin = (b > 60) ? PIN_MQ_B : PIN_MQ_A;
  Serial.printf("[SENSOR] GPIO%d(pulled-down)=%d GPIO%d=%d -> using GPIO%d\n",
                PIN_MQ_B, b, PIN_MQ_A, a, sensorPin);
}

const char* statusText() {
  if (sensorFault) return "FAULT";
  if (isGasDanger) return "DANGER";
  if (!isWarmingUp && gasPercent >= gasThresholdPercent * 0.75f) return "WARNING";
  return "SAFE";
}

// ---- Cloud telemetry (own task, never blocks the alarm loop) --------------------------
void telemetryTask(void*) {
  WiFiClientSecure tls;
  tls.setInsecure();                       // public Render endpoint; no CA bundle on device
  WiFiClient plain;
  const bool https = String(BACKEND_URL).startsWith("https");
  unsigned long lastTx = 0, backoffUntil = 0;
  int lastBackendThr = -1;

  for (;;) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    unsigned long now = millis();
    if (WiFi.status() != WL_CONNECTED || isWarmingUp || now < backoffUntil) continue;
    if (!telemetryNow && now - lastTx < 1000) continue;
    telemetryNow = false;
    lastTx = now;

    float pct = gasPercent;
    int ppm = (int)((pct / 100.0f) * 1200 + 15);
    String json = "{\"device_id\":\"" DEVICE_ID "\",\"sensor_type\":\"" SENSOR_NAME "\"";
    json += ",\"raw_value\":" + String(gasRawValue);
    json += ",\"gas_percent\":" + String(pct, 1);
    json += ",\"ppm_estimated\":" + String(ppm);
    json += ",\"status\":\"" + String(statusText()) + "\"";
    json += ",\"buzzer_state\":" + String(buzzerAudible ? "true" : "false");
    json += ",\"fan_state\":" + String(fanOn ? "true" : "false");
    json += ",\"threshold\":" + String((int)gasThresholdPercent) + "}";

    HTTPClient http;
    bool began = https ? http.begin(tls, BACKEND_URL) : http.begin(plain, BACKEND_URL);
    if (!began) { backoffUntil = now + 5000; continue; }
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    int code = http.POST(json);
    if (code == 200 || code == 201) {
      // Backend is the source of truth for the alarm threshold set on the cloud dashboard.
      String body = http.getString();
      int p = body.indexOf("\"threshold\"");
      p = (p >= 0) ? body.indexOf(':', p) : -1;
      int thr = (p >= 0) ? body.substring(p + 1).toInt() : 0;
      if (thr >= 5 && thr <= 95 && thr != lastBackendThr) {
        lastBackendThr = thr;
        if (thr != (int)gasThresholdPercent) {
          gasThresholdPercent = (float)thr;
          Serial.printf("[CONFIG] Threshold synced from cloud: %d%%\n", thr);
        }
      }
    } else {
      backoffUntil = now + 3000;           // Render free tier may be cold-starting
    }
    http.end();
  }
}

// ---- Local web API ----------------------------------------------------------------------
void setCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void sendOk(const char* body = "{\"status\":\"ok\"}") {
  setCORSHeaders();
  server.send(200, "application/json", body);
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatusApi() {
  String json = "{\"gasRaw\":" + String(gasRawValue);
  json += ",\"gasPercent\":" + String(gasPercent, 1);
  json += ",\"threshold\":" + String((int)gasThresholdPercent);
  json += ",\"status\":\"" + String(statusText()) + "\"";
  json += ",\"buzzer\":" + String(buzzerAudible ? "true" : "false");
  json += ",\"muted\":" + String(isBuzzerMuted ? "true" : "false");
  json += ",\"fan\":" + String(fanOn ? "true" : "false");
  json += ",\"warmup\":" + String(isWarmingUp ? "true" : "false");
  json += ",\"uptime\":" + String(millis() / 1000) + "}";
  sendOk(json.c_str());
}

void handleConfigApi() {
  int val = server.arg("threshold").toInt();
  if (val >= 5 && val <= 95) {
    gasThresholdPercent = (float)val;
    Serial.printf("[CONFIG] Alarm threshold set to %d%%\n", val);
  }
  sendOk();
}

void handleMuteApi() {
  if (server.hasArg("state")) isBuzzerMuted = (server.arg("state") == "1");
  sendOk();
}

void handleTestAlarm() {
  alarmTestActive = true;
  testAlarmEnd = millis() + 4000;
  sendOk("{\"status\":\"testing\"}");
}

void handleFanToggle() {
  fanManual = !fanManual;
  sendOk();
}

void handleOptions() { setCORSHeaders(); server.send(204); }

// ---- Setup --------------------------------------------------------------------------------
void setup() {
  // Safe outputs FIRST so nothing beeps/clicks while the rest initialises.
  pinMode(PIN_BUZZER, OUTPUT);
  driveBuzzer(false);
  pinMode(PIN_RELAY, OUTPUT);
  driveRelay(false);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, HIGH);

  Serial.begin(115200);
  delay(300);
  bootTime = millis();
  esp_reset_reason_t rr = esp_reset_reason();
  Serial.printf("\n[BOOT] Reset reason: %d (%s)\n", (int)rr,
                rr == ESP_RST_POWERON ? "power-on" : rr == ESP_RST_SW ? "software/upload" :
                rr == ESP_RST_BROWNOUT ? "BROWNOUT - weak USB/5V supply!" :
                rr == ESP_RST_PANIC ? "crash" : rr == ESP_RST_EXT ? "reset button" : "other/watchdog");
  Serial.println("=======================================================");
  Serial.println("  AURA GASGUARD - ESP32 GAS LEAKAGE DETECTION v4.0");
  Serial.printf("  Threshold: %d%% | Warm-up: %lus | Buzzer: %s\n", DEFAULT_THRESHOLD_PCT,
                WARMUP_MS / 1000, BUZZER_ACTIVE_LOW ? "active-LOW" : "active-HIGH");
  Serial.println("=======================================================");

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
#if USE_DIGITAL_OUT
  pinMode(PIN_MQ_DO, INPUT);
#endif
  detectSensor();

  // 350 ms audible self-test
  digitalWrite(PIN_LED_RED, HIGH);
  driveBuzzer(true);
  delay(350);
  driveBuzzer(false);
  digitalWrite(PIN_LED_RED, LOW);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(SOFTAP_SSID, SOFTAP_PASSWORD);
  Serial.println("[WiFi] Access Point: http://192.168.4.1");
#ifdef WIFI_STA_SSID
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);
  Serial.printf("[WiFi] Joining '%s' for cloud telemetry...\n", WIFI_STA_SSID);
#else
  Serial.println("[WiFi] No secrets.h - cloud upload via USB serial bridge only.");
#endif
  if (MDNS.begin("gasguard")) Serial.println("[mDNS] http://gasguard.local");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatusApi);
  server.on("/api/config", HTTP_POST, handleConfigApi);
  server.on("/api/mute", HTTP_POST, handleMuteApi);
  server.on("/api/test", HTTP_POST, handleTestAlarm);
  server.on("/api/fan/toggle", HTTP_POST, handleFanToggle);
  for (const char* p : {"/api/status", "/api/config", "/api/mute", "/api/test", "/api/fan/toggle"})
    server.on(p, HTTP_OPTIONS, handleOptions);
  server.begin();

  xTaskCreatePinnedToCore(telemetryTask, "telemetry", 16384, nullptr, 1, nullptr, 0);
  Serial.printf("[INFO] Warming up sensor for %lu s...\n\n", WARMUP_MS / 1000);
}

// ---- Main loop ----------------------------------------------------------------------------
void sampleSensor(unsigned long now) {
  unsigned long elapsed = now - bootTime;
  isWarmingUp = elapsed < WARMUP_MS;

  // Re-detect the wiring once the heater is hot, then average the clean-air baseline.
  if (isWarmingUp && !windowStarted && elapsed >= WARMUP_MS - BASELINE_WINDOW_MS) {
    windowStarted = true;
    detectSensor();
  }

  int raw = readAdc(sensorPin, 16);
  gasRawValue = raw;
  faultCount = (raw < SENSOR_MIN_VALID_RAW) ? faultCount + 1 : 0;
  sensorFault = faultCount >= (int)(FAULT_HOLD_MS / SAMPLE_INTERVAL_MS);
  if (sensorFault && now - lastRedetect >= 5000) {   // wiring may have changed: look at both pins again
    lastRedetect = now;
    detectSensor();
  }
  if (wasFault && !sensorFault && !isWarmingUp) {   // wire reconnected: re-learn clean air first
    settleUntil = now + 5000;
    Serial.println("[SENSOR] Signal restored - re-calibrating baseline for 5 s");
  }
  wasFault = sensorFault;

  if (isWarmingUp) {
    if (windowStarted && !sensorFault) { baseSum += raw; baseCount++; }
    gasPercent = 0.0f;
    return;
  }
  if (!calibrated) {
    cleanAirBase = baseCount > 0 ? (float)baseSum / baseCount : (float)raw;
    calibrated = true;
    Serial.printf("[CALIBRATED] Clean-air baseline = %.0f counts (GPIO%d)\n", cleanAirBase, sensorPin);
  }

  if (sensorFault) {                       // never alarm (or learn a baseline) on a dead input
    if (isGasDanger) { isGasDanger = false; telemetryNow = true; }
    gasPercent = 0.0f;
    overCount = 0;
    return;
  }

  if (raw < SENSOR_MIN_VALID_RAW) {         // signal momentarily missing: never learn it, never alarm on it
    gasPercent = 0.0f;
    overCount = 0;
    return;
  }

  if ((long)(now - settleUntil) < 0) {     // settling after a fault: follow the signal, no alarms
    cleanAirBase = (float)raw;
    gasPercent = 0.0f;
    overCount = 0;
    return;
  }

  float pct = (raw > cleanAirBase) ? (raw - cleanAirBase) / (4095.0f - cleanAirBase) * 100.0f : 0.0f;
  if (pct > 100.0f) pct = 100.0f;
  gasPercent = pct;

  // Baseline tracks slow ambient drift only; a real leak (>2 %) is never absorbed.
  if (!isGasDanger) {
    if (raw < cleanAirBase)  cleanAirBase += (raw - cleanAirBase) * 0.05f;
    else if (pct < 2.0f)     cleanAirBase += (raw - cleanAirBase) / 1200.0f;
  }

  bool digitalTrigger = false;
#if USE_DIGITAL_OUT
  digitalTrigger = (digitalRead(PIN_MQ_DO) == LOW);
#endif
  overCount = (pct >= gasThresholdPercent || digitalTrigger) ? min(overCount + 1, 255) : 0;
  int needed = (pct >= gasThresholdPercent * 2.0f) ? 1 : TRIGGER_SAMPLES;   // strong gas: alarm at once

  if (!isGasDanger && overCount >= needed) {
    isGasDanger = true;
    telemetryNow = true;
    Serial.printf("\n[ALARM TRIGGERED] %.1f%% >= %d%% (raw %d)\n\n", pct, (int)gasThresholdPercent, raw);
  } else if (isGasDanger && pct < gasThresholdPercent - 2.5f && !digitalTrigger) {
    isGasDanger = false;
    isBuzzerMuted = false;                 // a mute only ever lasts for one alarm
    telemetryNow = true;
    Serial.printf("\n[ALARM CLEARED] %.1f%%\n\n", pct);
  }
}

void updateActuators(unsigned long now) {
  if (alarmTestActive && (long)(now - testAlarmEnd) >= 0) alarmTestActive = false;
  bool alarm = isGasDanger || alarmTestActive;

  if (alarm) {
    if (!prevAlarm) { beat = true; lastBeat = now; }       // first beep + red flash start immediately
    else if (now - lastBeat >= 180) { lastBeat = now; beat = !beat; }
    buzzerAudible = beat && !isBuzzerMuted;
    driveBuzzer(buzzerAudible);
    digitalWrite(PIN_LED_RED, beat ? HIGH : LOW);          // red flashes with the buzzer
    digitalWrite(PIN_LED_GREEN, LOW);
  } else {
    beat = false;
    buzzerAudible = false;
    driveBuzzer(false);
    if (sensorFault) {                                     // sensor signal gone > 5 s: green off, brief red blip
      digitalWrite(PIN_LED_RED, (now % 2000) < 100 ? HIGH : LOW);
      digitalWrite(PIN_LED_GREEN, LOW);
    } else {                                               // idle / normal / warm-up: green STEADY
      digitalWrite(PIN_LED_RED, LOW);
      digitalWrite(PIN_LED_GREEN, HIGH);
    }
  }
  prevAlarm = alarm;
  driveRelay(isGasDanger || fanManual);
}

// Optional host command from serial_bridge.py: "T<5..95>\n" sets the alarm threshold.
void pollSerialCommands() {
  static String line;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (line.length() > 1 && line[0] == 'T') {
        int v = line.substring(1).toInt();
        if (v >= 5 && v <= 95) {
          gasThresholdPercent = (float)v;
          Serial.printf("[CONFIG] Alarm threshold set to %d%% (serial)\n", v);
        }
      }
      line = "";
    } else if (line.length() < 8) {
      line += c;
    }
  }
}

void loop() {
  server.handleClient();
  pollSerialCommands();
  unsigned long now = millis();

  if (now - lastSensorRead >= SAMPLE_INTERVAL_MS) {
    lastSensorRead = now;
    sampleSensor(now);
  }
  updateActuators(now);

  if (now - lastSerial >= 200) {
    lastSerial = now;
    if (isWarmingUp) {
      Serial.printf("[WARMUP] %lus left | Raw ADC: %4d | Alarm: OFF\n",
                    (WARMUP_MS - (now - bootTime)) / 1000, (int)gasRawValue);
    } else {
      // Format is parsed by serial_bridge.py - keep the first four fields unchanged.
      Serial.printf("[LIVE] Gas: %4.1f%% | Raw ADC: %4d | Limit: %d%% | Status: %-7s | Base: %.0f | Pin: %d\n",
                    (float)gasPercent, (int)gasRawValue, (int)gasThresholdPercent, statusText(),
                    cleanAirBase, sensorPin);
    }
  }
}
