#pragma once
#include <Arduino.h>

// ====================================================================
// ESP32 DevKit V1 Pin Configuration
// ====================================================================

// --- Onboard Status Indicator LED ---
#define PIN_STATUS_LED         2   // Built-in Blue LED on ESP32 DevKit V1

// --- Ultrasonic Sensors (6-Directional Array: Cardinal + Rear Diagonals) ---
// Multi-Trigger Pins pulse sensor layers (Trig 1: GPIO 27, Trig 2: GPIO 14 / D14, Trig 3: GPIO 23 / D23)
#define PIN_US_TRIG_1          27  // Primary Trigger (GPIO 27)
#define PIN_US_TRIG_2          14  // Secondary Trigger (GPIO 14 / D14)
#define PIN_US_TRIG_3          23  // Next-Layer Trigger (GPIO 23 / D23)
#define PIN_US_TRIG            PIN_US_TRIG_1  // Compatibility alias

// Set 1: Cardinal Directions (0°, 90°, 180°, 270°) - Default Active
#define PIN_US_ECHO_0          34  // S0: 0°   (Front)
#define PIN_US_ECHO_1          35  // S1: 90°  (Right)
#define PIN_US_ECHO_2          32  // S2: 180° (Back)
#define PIN_US_ECHO_3          25  // S3: 270° (Left)

// Set 2: Rear Diagonals (135°, 225°) - Front-Left (FL) and Front-Right (FR) DISABLED
#define PIN_US_ECHO_4          39  // S4: 135° (Rear-Right / VN)
#define PIN_US_ECHO_5          26  // S5: 225° (Rear-Left - connected to GPIO 26)

#define NUM_ULTRASONIC_SENSORS 6

// --- Hardware Alarm / Buzzer (Trapped / No Moves Available) ---
#define PIN_ALARM              4   // GPIO 4 (D4): Energized HIGH when robot has no moves available

// --- 2-Channel Relay Module (Left & Right Motors) ---
// Relay 1 (Channel 1): Left Motor
// Relay 2 (Channel 2): Right Motor
#define PIN_RELAY_1            18  // Channel 1: Left Motors
#define PIN_RELAY_2            19  // Channel 2: Right Motors

// Relay Module Active Logic Level:
// Standard 5V opto-isolated relay modules are ACTIVE LOW (LOW = Relay ON / Energized)
// Set RELAY_ACTIVE_LOW to false if your relay module is Active HIGH
#define RELAY_ACTIVE_LOW       true
#define RELAY_ON               (RELAY_ACTIVE_LOW ? LOW : HIGH)
#define RELAY_OFF              (RELAY_ACTIVE_LOW ? HIGH : LOW)

// Steering & Motion Truth Table:
// - Relay 1 ON,  Relay 2 OFF -> Left Motors ON  -> Turn Right
// - Relay 1 OFF, Relay 2 ON  -> Right Motors ON -> Turn Left
// - Relay 1 ON,  Relay 2 ON  -> Both Motors ON  -> Move Forward
// - Relay 1 OFF, Relay 2 OFF -> Both Motors OFF -> Stop / E-Stop
//
// 2-channel relay cannot reverse DC polarity without an external H-bridge.
// Set to false so autonomous evasion rotates to best angle when front is blocked.
#define RELAY_CAN_REVERSE      false

// --- MPU6050 Gyro / Accelerometer (I2C) ---
#define PIN_I2C_SDA            21
#define PIN_I2C_SCL            22
#define MPU6050_I2C_ADDR       0x68

// --- Default Robot Parameters ---
#define DEFAULT_THRESHOLD_CM   25.0f
#define MIN_EVADE_THRESHOLD_CM 10.0f
#define MAX_EVADE_THRESHOLD_CM 100.0f  // Up to 100cm max distance control
#define CRITICAL_STOP_CM       12.0f
#define MAX_SENSOR_DISTANCE_CM 300.0f
#define DEFAULT_SPEED          180
#define TURN_SPEED             190
#define EVADE_SPEED            170

// --- Stall Safety Watchdog ---
#define STALL_ESTOP_DURATION_MS 1000   // 1 second E-Stop when throttle is on without acceleration
#define DEFAULT_STALL_ACCEL_THRESHOLD 0.06f // Dynamic acceleration threshold in Gs (|a - 1.0g|)
#define MIN_STALL_ACCEL_THRESHOLD     0.01f
#define MAX_STALL_ACCEL_THRESHOLD     0.30f

// --- Relay Pulse / Tap Parameters ---
#define DEFAULT_TAP_ON_MS      60   // Relay energized pulse duration (ms)
#define MIN_TAP_ON_MS          25   // Minimum tap pulse (micro-nudge)
#define MAX_TAP_ON_MS          160  // Maximum tap pulse
#define DEFAULT_TAP_OFF_MS     110  // Coast & ultrasonic measurement window between taps (ms)
#define MIN_TAP_OFF_MS         60   // Minimum pause between taps
#define MAX_TAP_OFF_MS         300  // Maximum pause between taps

// --- Access Point (AP) & OTA Configuration ---
#define AP_SSID                "ESP32-EvadeBot-AP"
#define AP_PASSWORD            "admin12345"
#define OTA_HOSTNAME           "esp32-evade-bot"
#define OTA_PASSWORD           "admin"
#define WEB_SERVER_PORT        80

// --- Control Modes ---
enum RobotControlMode {
    MODE_AUTO_EVADE = 0,
    MODE_WEB_OVERRIDE
};

// --- Detection Status Signals ---
enum ObstacleStatus {
    STATUS_CLEAR = 0,
    STATUS_OBJECT_IN_FRONT,
    STATUS_OBJECT_ON_RIGHT,
    STATUS_OBJECT_ON_LEFT,
    STATUS_OBJECT_IN_REAR,
    STATUS_OBJECT_BOTH_SIDES,
    STATUS_ALL_SIDES_TRAPPED
};
