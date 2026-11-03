#ifndef GASGUARD_CONFIG_H
#define GASGUARD_CONFIG_H

// =====================================================================================
//  Aura GasGuard - hardware & behaviour configuration (single source of truth)
// =====================================================================================

// ---- Pins (30-pin ESP32 DevKit V1) ---------------------------------------------------
// MQ-2 analog output (AO) may be wired to GPIO 34 (documented default) OR GPIO 32.
// The firmware auto-detects which one is driven by the sensor at boot (see detectSensor).
#define PIN_MQ_A          34
#define PIN_MQ_B          32
#define PIN_BUZZER        25
// Red = alarm (with buzzer), green = idle/safe. If they look swapped on your board, the LEDs are
// plugged into each other's pins: either swap the wires (red -> D26, green -> D27) or set this to 1.
#define LEDS_SWAPPED      1
#if LEDS_SWAPPED
#define PIN_LED_RED       27
#define PIN_LED_GREEN     26
#else
#define PIN_LED_RED       26
#define PIN_LED_GREEN     27
#endif

// ---- Buzzer --------------------------------------------------------------------------
// 3-pin transistor buzzer modules (as in CIRCUIT_DIAGRAM.md) sound when the signal is LOW.
// Setting this wrong makes the buzzer beep CONTINUOUSLY while idle.
#define BUZZER_ACTIVE_LOW 1
// 0 = active buzzer (plain on/off level), 1 = passive buzzer (needs a tone() frequency).
#define BUZZER_PASSIVE    0
#define BUZZER_TONE_HZ    2400

// ---- MQ-2 digital output (DO) --------------------------------------------------------
// GPIO 35 has NO internal pull-up. A disconnected DO floats and reads LOW at random,
// which used to raise false alarms. Enable only if DO is wired AND has a 10k pull-up to 3V3.
#define USE_DIGITAL_OUT   0
#define PIN_MQ_DO         35

// ---- Detection -----------------------------------------------------------------------
#define DEFAULT_THRESHOLD_PCT   10      // alarm at >= 10 % (user-adjustable 5..95)
#define WARMUP_MS               20000UL // MQ-2 heater needs ~20 s to stabilise
#define BASELINE_WINDOW_MS      3000UL  // last part of warm-up averaged into the clean-air baseline
#define SAMPLE_INTERVAL_MS      50      // sensor sampled 20x per second
#define TRIGGER_SAMPLES         2       // consecutive samples over limit before alarming (~100 ms)
                                        // (a reading >= 2x the limit alarms on the very first sample)
#define SENSOR_MIN_VALID_RAW    20      // below this the AO signal is considered missing
#define FAULT_HOLD_MS           5000UL  // ...and only after it stays missing this long, so a brief
                                        // wire wobble / power dip never changes the LEDs
                                        // (a saturated 4095 is NOT a fault: it means a lot of gas)
#define SENSOR_NAME             "MQ-9"  // label sent to the dashboard (MQ-2, MQ-9, ...)

// ---- Exhaust fan relay (optional, see CIRCUIT_DIAGRAM.md) ------------------------------
#define PIN_RELAY         14
#define RELAY_ACTIVE_LOW  1

// ---- Backend -------------------------------------------------------------------------
#define BACKEND_URL      "https://aura-gasguard.onrender.com/api/sensor-data"
#define DEVICE_ID        "ESP32-GAS-01"
#define SOFTAP_SSID      "ESP32-Gas-Detector"
#define SOFTAP_PASSWORD  "12345678"

#endif
