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
| | `D0` (Digital)| *Not connected* | Leave disconnected |
| **3-Pin Buzzer** | **`S` / `I/O` (Signal)**| **`D25` (Pin 8, left side)** | Active-LOW control signal |
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

### 2. Why Was the Buzzer Sounding Without Gas?
1. **3-Pin Buzzer Active-LOW Inversion:**  
   3-pin buzzer modules contain an onboard transistor that sounds when given a `LOW (0V)` signal.  
   We fixed this in firmware by setting `#define BUZZER_ACTIVE_LOW 1`.
2. **Preheating Grace Period:**  
   When the MQ-2 sensor first powers on, it takes 20 seconds to heat up. During this time, it outputs a temporary false spike. The firmware now mutes the buzzer during the first 20 seconds so it never false-alarms on startup.
3. **MQ-2 Connected to `D0` instead of `A0`:**  
   Verify that your jumper wire on the MQ-2 is connected to the pin labeled **`A0`**, NOT `D0`.
