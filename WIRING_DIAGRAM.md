# TinkerHub CyberBot — Complete Hardware Wiring & Circuit Guide

This document provides complete, pin-by-pin hardware schematics, power distribution layouts, and terminal connection guides for the **TinkerHub ESP32 Autonomous Evade Robot**.

---

## 1. System Block Diagram

```
                             +---------------------------------------+
                             |       Main Battery (7.4V - 12V)       |
                             +-------------------+-------------------+
                                                 |
                         +-----------------------+-----------------------+
                         |                                               |
                         v                                               v
           +---------------------------+                   +---------------------------+
           |   DC-DC Buck Converter    |                   |   Relay Power Bus (+V)    |
           |   Output: 5.0V Regulated  |                   | (Through optional Master) |
           +-------------+-------------+                   +-------------+-------------+
                         |                                               |
         +---------------+---------------+                               |
         |               |               |                               |
         v               v               v                               v
  +--------------+ +-----------+ +---------------+                +--------------+
  | ESP32 DevKit | | 4x HC-SR04| | 2/3-Ch Relays |                | COM1 & COM2  |
  |  (VIN / 5V)  | |  (VCC 5V) | |  (VCC / JD)   |                | Relay Inputs |
  +-------+------+ +-----+-----+ +-------+-------+                +-------+------+
          |              |               |                                |
          | I2C (3.3V)   | Shared Trig   | IN1, IN2, (IN3)                |
          v              | 4x Echos      v                                v
  +--------------+       |       +---------------+                +--------------+
  |   MPU6050    |<------+       | ESP32 GPIOs   |                | NO1 -> Left  |
  | (SDA21/SCL22)|               | 18, 19, (4)   |                | NO2 -> Right |
  +--------------+               +---------------+                +-------+------+
                                                                          |
                                                                          v
                                                              +----------------------+
                                                              | 4x DC Geared Motors  |
                                                              +----------------------+
```

---

## 2. Power Distribution & Grounding Architecture

Robots with DC motors and mechanical relays generate electromagnetic interference (EMI) and inductive voltage spikes. Follow this wiring design to prevent ESP32 resets:

### Common Ground (Star Ground)
- **ALL grounds must tie together**: Connect ESP32 `GND`, Buck Converter `GND`, Relay Module `GND`, Sensor `GND`, Raspberry Pi `GND`, and Battery `(-)`.
- Always connect logic grounds to a central bus bar rather than daisy-chaining through the motor power wires.

### Power Rails Summary
| Power Rail | Voltage | Source | Supplies |
| :--- | :--- | :--- | :--- |
| **Motor High-Current Rail** | 7.4V – 12V | Battery (+) | Relay COM terminals -> DC Motors |
| **Logic 5V Rail** | 5.0V Regulated | DC-DC Step-Down (3A+) | ESP32 `VIN`, Relay Board `VCC`, Ultrasonic `VCC`, Raspberry Pi |
| **Sensor 3.3V Rail** | 3.3V Clean | ESP32 `3V3` Pin | MPU6050 IMU |

---

## 3. Relay Module Wiring (Motor Drive & Steering)

### 2-Channel Relay Board Pinout
| Relay Board Pin | ESP32 Pin | Description |
| :--- | :--- | :--- |
| **VCC** | **5V (VIN)** | Powers the optocoupler logic |
| **GND** | **GND** | Logic ground |
| **IN1** | **GPIO 18** | Triggers Relay 1 (Left Motors) |
| **IN2** | **GPIO 19** | Triggers Relay 2 (Right Motors) |

### Optocoupler Isolation Jumper (`VCC` / `JD-VCC`)
* Most multi-channel relay boards feature a 3-pin header with a blue jumper linking `VCC` and `JD-VCC`.
* **Standard Setup (Single 5V supply)**: Keep the jumper on `VCC-JDVCC` and supply 5V to the `VCC` pin.
* **Full Optical Isolation (Best for zero noise)**: Remove the jumper. Connect ESP32 `5V` to `VCC`. Connect an independent 5V supply to `JD-VCC` and `GND` to power only the relay coils.

### Relay Output Terminal Wiring (DC Motors)
Each relay has three screw terminals: `COM` (Common), `NO` (Normally Open), and `NC` (Normally Closed).

```
   [Battery (+) / +V Motor Power]
                 |
        +--------+--------+
        |                 |
        v                 v
   +----------+      +----------+
   | Relay 1  |      | Relay 2  |
   |   COM    |      |   COM    |
   |    NO    |      |    NO    |
   +----+-----+      +----+-----+
        |                 |
        v                 v
   [Left Motor +]   [Right Motor +]
        |                 |
        v                 v
   [Left Motor -]   [Right Motor -]
        |                 |
        +--------+--------+
                 |
                 v
   [Battery (-) Ground Return]
```

#### Motor Flyback Protection (Recommended)
Place a **1N4007** or **1N5819 Schottky diode** directly across each DC motor's terminals (Cathode/band to Motor `+`, Anode to Motor `-`). This absorbs inductive back-EMF spikes when relays switch off, preventing contact pitting and ESP32 resets.

---

## 4. Optional 3rd Relay (Single-Channel) Integration

You can use a 3rd single-channel relay for one of the following setups:

### Option A: Master Safety Kill-Switch / Hardware E-Stop (Recommended)
Wiring the 3rd relay in series before the motor relays gives you a physical power cutoff:
```
[Battery +] ---> [Relay 3 (Master) COM]
                 [Relay 3 (Master) NO ] ---> [Relay 1 COM] & [Relay 2 COM]
```
* **Control Pin**: Connect Relay 3 `IN` to **`GPIO 4`**.
* **Safety Benefit**: Cutting Relay 3 kills all motor power immediately during emergencies or if the software watchdog triggers.

### Option B: High-Power Searchlight / Siren / Actuator
```
[12V / 5V +] ---> [Relay 3 COM]
                  [Relay 3 NO ] ---> [Light / Siren / Solenoid (+)]
[Light / Siren / Solenoid (-)] ---> [Battery / GND]
```
* **Control Pin**: Connect Relay 3 `IN` to **`GPIO 4`**.

---

## 5. Ultrasonic Sensor Array (6 Directions: Cardinal & Rear Diagonals)

The robot supports **6 ultrasonic sensors** arranged around the chassis:
* **Set 1: Cardinal Set (Default Active)**: Front (0°), Right (90°), Back (180°), Left (270°).
* **Set 2: Rear Diagonal Set (Web Toggleable)**: Rear-Right (135°), Rear-Left (225°).
*(Note: Front-Left and Front-Right are **disabled**).*

### Triple Trigger Lines (GPIO 27, GPIO 14 / D14, & GPIO 23 / D23)
The sensors are triggered via dedicated trigger lines pulsed simultaneously (10µs pulse):
* **Trigger 1 (ESP32 GPIO 27)**: Primary Trigger for Set 1 (S0..S3 Cardinal).
* **Trigger 2 (ESP32 GPIO 14 / D14)**: Secondary Trigger for Set 2 (S4..S5 Rear Diagonals).
* **Trigger 3 (ESP32 GPIO 23 / D23)**: Expansion Trigger for next-layer / auxiliary ultrasonic sensors.
*(All three lines pulse synchronously in firmware, preventing pin overdrive and distributing drive current).*

### Dedicated Echo Lines (6 Sensors)
Each sensor returns its echo pulse to a dedicated ESP32 input pin:

| Set | Sensor Index | Direction Angle | ESP32 GPIO | Logic Level Handling | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Set 1 (Cardinal)** | **S0** (Front) | 0° | **GPIO 34** | Input Only (1kΩ / 2kΩ divider if 5V) | Front obstacle detection |
| **Set 1 (Cardinal)** | **S1** (Right) | 90° | **GPIO 35** | Input Only (1kΩ / 2kΩ divider if 5V) | Right flank |
| **Set 1 (Cardinal)** | **S2** (Back) | 180° | **GPIO 32** | Digital Input | Rear flank |
| **Set 1 (Cardinal)** | **S3** (Left) | 270° | **GPIO 25** | Digital Input | Left flank |
| **Set 2 (Rear Diag)** | **S4** (Rear-Right)| 135° | **GPIO 39 (VN)**| Input Only (1kΩ / 2kΩ divider if 5V) | Rear-right protection |
| **Set 2 (Rear Diag)** | **S5** (Rear-Left) | 225° | **GPIO 26** | Digital Input | Re-assigned from FL |

> [!NOTE]
> - **Front-Left (FL)** and **Front-Right (FR)** are disabled in firmware.
> - The sensor previously connected as Front-Left (**GPIO 26**) is now mapped as **Rear-Left (225°)**.
> - Pins **GPIO 36 (VP)** and **GPIO 33** are freed up.

### Voltage Divider on 5V HC-SR04 Echo Pins
Standard HC-SR04 sensors output a **5V Echo pulse**, but the ESP32 is **3.3V tolerant only**. Use this simple resistor network on each Echo line:

```
Sensor ECHO Pin (5V)
       |
     [ 1kΩ Resistor ]
       |
       +-------------------> To ESP32 GPIO (e.g. GPIO 34, 35, 39)
       |
     [ 2kΩ Resistor ]
       |
      GND
```
*(Note: If using **HC-SR04P** or **RCWL-9610**, these support 3.3V natively and do not require resistors).*

---

## 6. MPU6050 6-DOF IMU (I2C)

The MPU6050 provides real-time gyro yaw integration and dynamic linear acceleration monitoring for the **Stall Watchdog (1-Second E-Stop)**.

| MPU6050 Pin | ESP32 DevKit V1 Pin | Description |
| :--- | :--- | :--- |
| **VCC** | **3.3V** | Low-noise 3.3V power from ESP32 regulator |
| **GND** | **GND** | Ground |
| **SDA** | **GPIO 21** | I2C Data line (Hardware I2C) |
| **SCL** | **GPIO 22** | I2C Clock line (Hardware I2C) |
| **AD0** | **GND** | Selects default I2C address `0x68` (Leave tied to GND or unconnected) |
| **INT** | *Not Connected* | Interrupt line (polling/FIFO used) |

---

## 7. Self-Defense High-Voltage Taser Module (GPIO 4 / D4)

When autonomous evasion determines that **no more moves are available** (front and both rotation flanks blocked, or bot is trapped), the ESP32 energizes **GPIO 4 (D4)** to trigger the high-voltage Taser / electric stun module for active personal space defense.

```
[ ESP32 GPIO 4 (D4) ] ---> [ High-Voltage Taser Relay / Arc Module Trigger IN (+) ]
[ ESP32 GND         ] ---> [ Taser Module Ground Return (-) ]
```

- **Logic**: Driven **HIGH** (3.3V) when trapped / no moves available; driven **LOW** (0V) when an open path is detected.
- **Safety Control**: Can be tested or disarmed manually from the Web Controller or via keyboard spacebar.

---

## 8. Complete Master Pin Mapping Table

| ESP32 GPIO | Direction | Connected Peripheral | Voltage | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **GPIO 2** | Output | Built-in Status LED | 3.3V | Blinks when WiFi connected, OFF on disconnect/E-Stop |
| **GPIO 4** | Output | **High-Voltage Defense Taser Module (D4)** | 3.3V | **HIGH when trapped / no moves available** |
| **GPIO 18** | Output | Relay 1 (Left Motors) | 5V Opto | Active LOW |
| **GPIO 19** | Output | Relay 2 (Right Motors) | 5V Opto | Active LOW |
| **GPIO 27** | Output | Ultrasonic Trigger 1 | 3.3V/5V | Primary trigger (S0..S3 Cardinal) |
| **GPIO 14** | Output | Ultrasonic Trigger 2 (D14) | 3.3V/5V | Secondary trigger (S4..S5 Rear Diagonals) |
| **GPIO 23** | Output | Ultrasonic Trigger 3 (D23) | 3.3V/5V | Expansion trigger (Next-layer ultrasonic array) |
| **GPIO 34** | Input | Ultrasonic S0 Echo (Front - 0°) | 3.3V Max | Input-only pin (use divider if 5V) |
| **GPIO 35** | Input | Ultrasonic S1 Echo (Right - 90°) | 3.3V Max | Input-only pin (use divider if 5V) |
| **GPIO 32** | Input | Ultrasonic S2 Echo (Back - 180°)| 3.3V Max | Digital Input |
| **GPIO 25** | Input | Ultrasonic S3 Echo (Left - 270°)| 3.3V Max | Digital Input |
| **GPIO 39 (VN)**| Input | Ultrasonic S4 Echo (Rear-Right - 135°)| 3.3V Max | Rear Diagonal (use divider if 5V) |
| **GPIO 26** | Input | Ultrasonic S5 Echo (Rear-Left - 225°)| 3.3V Max | **Re-assigned from FL to Rear-Left** |
| **GPIO 21** | I/O | MPU6050 SDA | 3.3V | I2C Data (400 kHz) |
| **GPIO 22** | Output | MPU6050 SCL | 3.3V | I2C Clock |
| **GPIO 16** | *Unused* | *Freed up* (Formerly Pi RX) | 3.3V | General purpose I/O |
| **GPIO 17** | *Unused* | *Freed up* (Formerly Pi TX) | 3.3V | General purpose I/O |
| **GPIO 33** | *Unused* | *Freed up* (Formerly BL) | 3.3V | General purpose I/O |
| **GPIO 36 (VP)**| *Unused* | *Freed up* (Formerly FR) | 3.3V | Input-only pin |

---

## 9. Pre-Power Checklist

Before connecting your main battery:
1. [ ] **Verify Common Ground**: Continuity beep test between ESP32 GND, Relay GND, and Battery (-).
2. [ ] **Check Buck Converter Voltage**: Measure the step-down converter output with a multimeter to ensure it reads **5.0V - 5.2V** before plugging into the ESP32 `VIN`.
3. [ ] **Verify No Shorts across Motors**: Ensure motor leads are isolated and cannot touch the chassis.
4. [ ] **Ensure 3.3V on GPIO 34-39**: If using 5V HC-SR04 sensors, verify voltage dividers are installed on Echo lines.
5. [ ] **Relay Test**: With motors disconnected, power on the ESP32 and confirm the relay LEDs click and toggle appropriately during boot and commands.
