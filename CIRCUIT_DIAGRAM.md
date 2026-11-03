# 🔌 ESP32 (30-Pin) Gas Leakage Detection System - Circuit & Wiring Diagram

This document contains the verified wiring specifications, physical pin locations, and breadboard connections for building your Gas Leakage Detection System using the **30-Pin DOIT ESP32 DevKit V1**.

---

## 📌 30-Pin ESP32 Physical Pinout Map

Hold your ESP32 board with the **USB port at the bottom** and the **Wi-Fi antenna / metal shield at the top**:

```
                       ┌─────────────────────────┐
                       │     [ Wi-Fi Antenna ]   │
                       │                         │
            (Reset) EN ─┤ 1                    30 ├─ D23 (MOSI)
        (ADC1_0) D36/VP ─┤ 2                    29 ├─ D22 (SCL)
        (ADC1_3) D39/VN ─┤ 3                    28 ├─ TX0 (TX)
MQ-2 AO ──────►    D34 ─┤ 4                    27 ├─ RX0 (RX)
                   D35 ─┤ 5                    26 ├─ D21 (SDA)
                   D32 ─┤ 6                    25 ├─ D19 (MISO)
                   D33 ─┤ 7                    24 ├─ D18 (SCK)
BUZZER Signal ──►  D25 ─┤ 8                    23 ├─ D5
RED LED (+) ────►  D26 ─┤ 9                    22 ├─ D17 (TX2)
GREEN LED (+) ──►  D27 ─┤ 10                   21 ├─ D16 (RX2)
RELAY IN ──────►   D14 ─┤ 11                   20 ├─ D4
                   D12 ─┤ 12                   19 ├─ D2
                   D13 ─┤ 13                   18 ├─ D15
COMMON GND ─────►  GND ─┤ 14                   17 ├─ GND
5V POWER (USB) ─►  VIN ─┤ 15                   16 ├─ 3V3 (3.3V)
                       │       [ USB Port ]      │
                       └─────────────────────────┘
```

> [!IMPORTANT]
> **ALL CONTROL PINS ARE ON THE LEFT SIDE!**  
> Notice that **D34, D25, D26, D27, D14, GND, and VIN** are all located on the **LEFT column of pins**!  
> If you plugged your LEDs into pins on the right side of the board, they are on the wrong pins and will not turn on.

---

## 📋 Exact Component Wiring Table

| Component | Component Pin | ESP32 Pin Location | Details & Direction |
| :--- | :--- | :--- | :--- |
| **MQ-2 Gas Sensor** | **`VCC`** | **`VIN` (Pin 15, bottom-left)** | Needs 5V to heat the internal sensor coil |
| | **`GND`** | **`GND` (Pin 14, left side)** | Connect to breadboard GND rail |
| | **`A0` (Analog)**| **`D34` (Pin 4, left side)** | **Must be A0, NOT D0!** |
| | `D0` (Digital)| *Not connected* | Leave disconnected (see troubleshooting #2) |
| | *(alternative)* `A0` | `D32` (Pin 6, left side) | Also supported - auto-detected at boot |
| **3-Pin Buzzer** | **`S` / `I/O` (Signal)**| **`D25` (Pin 8, left side)** | Active-LOW control signal (`BUZZER_ACTIVE_LOW` in `include/config.h`) |
| | **`+` / `VCC`** | **`VIN` (5V) or `3V3`** | Power for buzzer module |
| | **`-` / `GND`** | **`GND` (left side)** | Common ground |
| **Green Safe LED** | **Long leg (+ Anode)** | **`D27` (Pin 10, left side)** | Connect via 220Ω resistor |
| | **Short leg (- Cathode)**| **`GND` (left side)** | Flat edge of LED to Ground |
| **Red Alert LED** | **Long leg (+ Anode)** | **`D26` (Pin 9, left side)** | Connect via 220Ω resistor |
| | **Short leg (- Cathode)**| **`GND` (left side)** | Flat edge of LED to Ground |
| **Relay Module** | **`IN` (Signal)** | **`D14` (Pin 11, left side)** | Active-LOW trigger for exhaust fan |
| | **`VCC`** | **`VIN` (5V)** | Powers relay coil |
| | **`GND`** | **`GND`** | Common ground |

---

## ⚡ Step-by-Step Troubleshooting for LEDs & Buzzer

### 1. Why Were the Red & Green LEDs Not Showing?
1. **Wrong Side of the Board:**  
   Ensure the wires are plugged into **`D26` (Pin 9)** and **`D27` (Pin 10)** on the **LEFT side of the ESP32**, not the right side!
2. **LED Polarity (Direction):**  
   LEDs are diodes and only work in one direction:
   - **Long leg (Positive / Anode)** ──► goes to ESP32 Pin (`D26` or `D27`).
   - **Short leg (Negative / Cathode, flat side of bulb)** ──► goes through a 220Ω resistor to **`GND`**.  
   *If the LED is flipped backwards, it will not light up at all.*
3. **Common Ground Connection:**  
   Ensure the negative leg of the LEDs connects back to the ESP32 **`GND` pin (Pin 14 on left)**.

---

### 2. Why Was the Buzzer Sounding Without Gas? (root causes found & fixed in firmware v4.0)
1. **Active-LOW buzzer idled in the "ON" state.** 3-pin buzzer modules sound when the signal is `LOW`.
   Firmware v3.3 drove the pin `LOW` whenever the alarm was off, so the buzzer beeped *continuously*.
   `include/config.h` now has `BUZZER_ACTIVE_LOW 1`; the single `driveBuzzer()` function applies it,
   so "off" is always `HIGH` for your module. If your module is active-HIGH, set it to `0`.
   If it is a bare **passive** piezo (no transistor board), set `BUZZER_PASSIVE 1`.
2. **Floating `D0` pin (GPIO 35) counted as "gas".** GPIO 35 has no internal pull-up, so an unconnected
   `D0` randomly reads `LOW` = "gas detected". The digital input is now disabled
   (`USE_DIGITAL_OUT 0`); enable it only if `D0` is wired *and* has a 10 kΩ pull-up to 3V3.
3. **Hard-coded clean-air baseline (740) and a 4 s warm-up.** If your sensor's clean-air reading was
   higher, the percentage started above 10 % immediately. The baseline is now **measured** at the end of a
   20 s warm-up and slowly tracks ambient drift. The alarm needs 3 consecutive samples (300 ms) over the limit.
4. **MQ-2 connected to `D0` instead of `A0`:** Verify the jumper goes to the pin labelled **`A0`**.

### 3. Why Was the Sensor Not Detecting Gas?
- The firmware used GPIO 32 whenever it read > 100 counts. A **floating** GPIO 32 (nothing connected) drifts
  around a few hundred counts, so it "won" over the real sensor on GPIO 34 and never reacted to gas.
  Firmware v4.0 auto-detects the AO pin at boot: GPIO 32 gets an internal pull-down, and a floating pin
  collapses to ~0, while a pin driven by the MQ-2 does not. The serial log shows the decision:
  `[SENSOR] GPIO32(pulled-down)=0 GPIO34=698 -> using GPIO34`.
- If the log shows **`Status: FAULT`** the AO signal reads ~0: the `A0` wire is loose, or the sensor has no 5 V on `VCC`
  (the heater needs it - the module's red power LED should be on, and the sensor body gets warm).
- The MQ-2 module runs from **5 V**, so its `A0` can swing above the ESP32's 3.3 V limit in heavy gas.
  Add a voltage divider (e.g. 10 kΩ from A0 to the ESP32 pin, 20 kΩ from that pin to GND) or at least
  a 10 kΩ series resistor to protect the ADC. Raise the threshold or re-check the log if you add a divider -
  the baseline is re-measured automatically at every boot.
- **Test with gas:** after the green LED stops blinking (20 s) hold an *unlit* lighter (gas only, no flame) near
  the sensor for 2-3 s. The serial `Gas:` value must climb past 10 % and the red LED + buzzer must pulse.

### 4. Status LED meanings
| Green | Red | Meaning |
| :--- | :--- | :--- |
| blinking | off | Warm-up (20 s), alarm disabled |
| solid | off | Safe |
| off | strobing + buzzer pulses | **GAS ALARM** (fan relay ON) |
| off | slow blink | **Sensor fault** (A0 signal lost) |
