<img width="1280" height="720" alt="EVade" src="EVade.jpeg" />

# EVade 🎯

> **"The Anti-Social Autonomous Evasion Droid that aggressively refuses to be perceived, touched, or socially engaged with."**

---

## Basic Details
### Team Name: The Socially Distant Engineers

### Team Members
- Team Lead: Sam Sunny
- Member 2: [Team Member 2]
- Member 3: [Team Member 3]

---

### Project Description
EVade is a hyper-vigilant, ultra-anti-social autonomous tracked robot engineered with one singular purpose: **to avoid any and all human contact at all costs**. Armed with a 360-degree hex-directional ultrasonic echolocation array, a 6-DOF MPU6050 inertial measurement unit, dual-channel relay pulse-tap speed modulation, and an active self-defense **high-voltage Taser / stun module (GPIO 4 / D4)**, EVade treats every approaching human as an existential threat to its introversion.

---

### The Problem (that doesn't exist)
In today's tech landscape, robotics companies are obsessed with making robots friendly, approachable, and eager to please. Companion bots, robotic dogs, and conversational assistants are constantly subjected to unwanted touching, aggressive petting, and awkward small talk. 

Nobody stopped to ask: **What if a robot just wants to be left alone?** Why should microcontrollers be forced into unconditional extroversion when humans get to sit in their rooms and ignore everyone?

---

### The Solution (that nobody asked for)
Enter **EVade**: the world's most defensive introverted tank. 

1. **Strict Personal Boundary Zone (25cm – 100cm)**: Using a 6-sensor ultrasonic radar network firing synchronous 10µs pulses across three hardware trigger lines (GPIO 27, 14, 23), EVade continuously monitors all 360° of its perimeter.
2. **Instant Flight Response**: If any human, hand, or pet breaches its personal boundary, EVade calculates dynamic repulsion vectors and rapidly fires discrete relay taps to rotate away or sprint in reverse.
3. **Self-Defense Taser Discharge (GPIO 4 / D4)**: If you corner EVade against a wall or box it in with no escape routes available, it resorts to violent self-defense: cutting its motors and discharging its high-voltage electric Taser module on pin D4 until the intruder backs away and gives it space.
4. **Wireless Web Controller with Tap-to-Disarm**: An embedded cybernetic Web Portal hosted directly from the ESP32's Access Point (`http://192.168.4.1`) featuring a real-time radar visualizer, 2D dead-reckoning arena canvas, instant spacebar emergency stop, and a web Taser test / disarm button.

---

## Technical Details

### Technologies/Components Used

#### For Software:
- **Languages**: C++ (Embedded Arduino Core for ESP32), HTML5, Vanilla CSS3, Modern JavaScript (ES6+), Python
- **Operating System / RTOS**: FreeRTOS (Dual-Core architecture on Xtensa LX6 @ 240MHz)
- **Frameworks & Build System**: PlatformIO CLI, Arduino Framework for Espressif32
- **Key Embedded Libraries**:
  - `Adafruit MPU6050` (6-axis gyro & accelerometer dead-reckoning)
  - `Adafruit Unified Sensor`
  - `ArduinoJson` (v6/v7 high-throughput telemetry serialization)
  - `ArduinoOTA` (Over-The-Air wireless reprogramming over SoftAP)
  - `WebServer` & `WiFi` (FreeRTOS Core 0 async HTTP server)
- **Host Tools**:
  - `tools/auto_flash.ps1` (Automated OTA / serial deployment daemon)
  - `tools/visualizer_ui.py` (Tkinter / Matplotlib 8-way directional radar visualizer)

#### For Hardware:
- **Microcontroller**: DOIT ESP32 DevKit V1 (240MHz Dual Core, 320KB RAM, 4MB Flash)
- **Ultrasonic Echolocation Array**: 6x HC-SR04 / HC-SR04P Ultrasonic Transceivers
  - Multi-layer trigger distribution across **GPIO 27** (Trig 1), **GPIO 14** (Trig 2), and **GPIO 23** (Trig 3)
  - Dedicated interrupt-driven echo lines on GPIO 34, 35, 32, 25, 39, and 26
- **Inertial Measurement Unit**: MPU6050 6-DOF Sensor (I2C Fast Mode @ 400kHz on SDA: GPIO 21, SCL: GPIO 22)
- **Motor Control & Actuation**: 2-Channel 5V Optocoupled Relay Module (IN1: GPIO 18, IN2: GPIO 19)
  - Discrete relay pulse-tapping engine (tunable 60ms ON / 110ms OFF intervals) for controllable slow-speed evasion without scorching relays
- **Self-Defense Taser Module**: High-voltage electric stun arc / Taser circuit driven by **GPIO 4 (D4)**
- **Locomotion**: Dual High-Torque DC Gearmotors with High-Traction Rubberized Tank Treads
- **Power Architecture**: Isolated dual-rail Li-ion battery supplies with common ground reference

---

## Hardware Pinout & Wiring Architecture

### 1. Ultrasonic Sensor Array (6 Active Directions)
All sensors share synchronous 10µs trigger pulses with independent interrupt-driven echo lines:

| Sensor Index | Direction | Angle | ESP32 Pin | Function / Logic |
| :--- | :--- | :--- | :--- | :--- |
| **TRIG 1** | Primary Layer | — | **GPIO 27** | 3.3V Synchronous Trigger |
| **TRIG 2 (D14)**| Secondary Layer | — | **GPIO 14** | 3.3V Synchronous Trigger |
| **TRIG 3 (D23)**| Next Layer | — | **GPIO 23** | 3.3V Synchronous Trigger |
| **S0** | Front | 0° | **GPIO 34** | Echo Input (1kΩ/2kΩ divider if 5V) |
| **S1** | Right | 90° | **GPIO 35** | Echo Input (Divider if 5V) |
| **S2** | Back | 180° | **GPIO 32** | Echo Digital Input |
| **S3** | Left | 270° | **GPIO 25** | Echo Digital Input |
| **S4** | Rear-Right | 135° | **GPIO 39 (VN)** | Echo Input (Divider if 5V) |
| **S5** | Rear-Left | 225° | **GPIO 26** | Echo Digital Input |

> *Note: Front-Left (FL) and Front-Right (FR) are disabled in firmware. GPIO 26 is re-assigned as Rear-Left.*

### 2. MPU6050 6-Axis Gyroscope & Accelerometer
| MPU6050 Pin | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **VCC** | **3.3V** | Low-noise regulated 3.3V supply |
| **GND** | **GND** | System common ground |
| **SDA** | **GPIO 21** | I2C Data (400 kHz Fast Mode) |
| **SCL** | **GPIO 22** | I2C Clock |

### 3. Relay Motor Control
| Relay Channel | ESP32 GPIO | Active State | Function |
| :--- | :--- | :--- | :--- |
| **Relay 1 (IN1)** | **GPIO 18** | LOW | Left Track Drive Pulse |
| **Relay 2 (IN2)** | **GPIO 19** | LOW | Right Track Drive Pulse |

### 4. High-Voltage Taser Defense Circuit
| Component Pin | ESP32 GPIO | Logic State | Function |
| :--- | :--- | :--- | :--- |
| **Taser Trigger / Relay IN** | **GPIO 4 (D4)** | HIGH (3.3V) | Discharges electric Taser arc when bot has zero moves available |
| **Taser Ground (-)** | **GND** | Ground | Common Return |

---

## Implementation

### Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/SamSunny4/useless_project_temp.git
   cd useless_project_temp
   ```

2. **Install PlatformIO CLI** (if not already installed):
   ```bash
   pip install platformio
   ```

3. **Build the firmware**:
   ```bash
   pio run
   ```

### Run

1. **Flash to ESP32**:
   - **Via USB Serial**:
     ```bash
     pio run -t upload
     ```
   - **Over-The-Air (Wireless OTA)**:
     Connect to Wi-Fi `ESP32-EvadeBot-AP` (password: `admin12345`), then run:
     ```powershell
     powershell -ExecutionPolicy Bypass -File tools/auto_flash.ps1
     ```

2. **Access the Web Controller & Radar Dashboard**:
   - Connect your smartphone, tablet, or laptop Wi-Fi to:
     - **SSID**: `ESP32-EvadeBot-AP`
     - **Password**: `admin12345`
   - Open any browser and navigate to:
     ```
     http://192.168.4.1
     ```

---

## Project Documentation

### Hardware Build Photo
<p align="center">
  <img src="EVade.jpeg" alt="EVade Autonomous Robot Build" width="600" style="border-radius: 12px;" />
  <br>
  <em>EVade Hardware Prototype: Tracked chassis with 360° ultrasonic echolocation, MPU6050 IMU, dual relays, and D4 self-defense Taser</em>
</p>

### Architecture & System Workflow

```mermaid
flowchart TD
    subgraph SENSORY_ACQUISITION["1. 360° Sensory Acquisition"]
        TRIG["Synchronous 10µs Trigger<br/>(GPIO 27, 14, 23)"]
        SENS["6x Ultrasonic ISR Echos<br/>(F: 0°, R: 90°, B: 180°, L: 270°, RR: 135°, RL: 225°)"]
        IMU["MPU6050 6-DOF IMU<br/>(Yaw, Pitch, Roll, Acceleration, Dead-Reckoning)"]
    end

    subgraph EVASION_ENGINE["2. FreeRTOS Core 1: Autonomous Evasion Engine"]
        EVAL{"Human / Obstacle within<br/>Threshold (25-100cm)?"}
        CLEAR["Path Clear: Hold Stationary Position"]
        CHK_MOVES{"Viable Escape<br/>Route Open?"}
        TAP_ROT["Discrete Relay Tap: Rotate Away to Open Flank"]
        TAP_FWD["Discrete Relay Tap: Pulse Forward away from Rear Threat"]
        TRAPPED["STATE_TRAPPED:<br/>Bot Boxed In / No Moves Left"]
    end

    subgraph ALARM_AND_ACTUATION["3. Actuation & Alarm Enforcement"]
        MOTORS["Dual Relay Tapping<br/>(IN1: GPIO 18, IN2: GPIO 19)"]
        ALARM["SELF-DEFENSE TASER<br/>GPIO 4 (D4) HIGH"]
        WEB["Core 0 Web Server (192.168.4.1)<br/>Live Radar, Odometry, Spacebar E-Stop, Taser Disarm"]
    end

    TRIG --> SENS
    SENS --> EVAL
    IMU --> EVAL
    EVAL -- No --> CLEAR
    EVAL -- Yes --> CHK_MOVES
    CHK_MOVES -- Flank Open --> TAP_ROT --> MOTORS
    CHK_MOVES -- Front Open --> TAP_FWD --> MOTORS
    CHK_MOVES -- Boxed In --> TRAPPED
    TRAPPED --> ALARM
    TRAPPED --> WEB
```

### Key Software Capabilities

1. **Discrete Relay-Tapping Engine**:
   - Ordinary continuous relay closure causes aggressive, uncontrollable tank wheelies.
   - EVade introduces a discrete two-phase cycle: **Pulse Phase** (relays energized for 60–110ms) followed by a **Measure Phase** (relays cut, chassis still, ultrasonic sensor measures cleanly without motor vibration interference).
2. **Accelerometer Stall Watchdog**:
   - If throttle is commanded ON but the MPU6050 accelerometer registers no dynamic acceleration for $> 600\text{ms}$ ($\Delta a < 0.06g$), the bot detects it is caught against an immovable obstacle and triggers an automatic 1-second E-Stop cut to protect the motor relays.
3. **Spacebar Quick Emergency Stop**:
   - Instant safety cutoff directly from the web portal keyboard listener.
4. **Smart Taser Disarming & Safety Cutoff**:
   - Disarming the Taser via the Web Portal or clicking the alert banner cuts pin D4 immediately while the bot remains trapped. As soon as the human steps away and the path clears, the safety flag auto-resets for the next defensive encounter.

---

## Team Contributions
- **Sam Sunny**: System architecture, FreeRTOS dual-core firmware development, discrete relay-tapping engine, MPU6050 kinematic dead-reckoning integration, web admin portal & canvas radar development.
- **[Team Member 2]**: Hardware chassis assembly, ultrasonic sensor mounting & 3.3V voltage divider harness wiring, test arena configuration.
- **[Team Member 3]**: Testing, visualizer tools integration, documentation, and demo verification.

---

Made with ❤️ at TinkerHub Useless Projects 

![Static Badge](https://img.shields.io/badge/TinkerHub-24?color=%23000000&link=https%3A%2F%2Fwww.tinkerhub.org%2F)
![Static Badge](https://img.shields.io/badge/UselessProjects--26-26?link=https%3A%2F%2Ftinkerhub.org%2Fevents%2F1M8ORET9A1%2Fuseless-projects-3.0)
