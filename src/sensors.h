#pragma once
#include <Arduino.h>
#include "config.h"

// Sector Angles for the Ultrasonic Sensors (6 Directions):
// Set 1 (Cardinal):
// Index 0: 0°   (Front)
// Index 1: 90°  (Right)
// Index 2: 180° (Back)
// Index 3: 270° (Left / -90°)
//
// Set 2 (Rear Diagonals - FL and FR DISABLED):
// Index 4: 135° (Rear-Right / 135°)
// Index 5: 225° (Rear-Left / -135° - connected to GPIO 26)

class SensorsManager {
public:
    SensorsManager();
    void init();
    void update(); // Non-blocking trigger and timeout watchdog

    float getDistance(uint8_t index) const;
    float getRawDistance(uint8_t index) const;
    const float* getAllDistances() const;

    // Toggleable 2nd Set (Diagonal)
    void setDiagonalSetEnabled(bool enabled);
    bool isDiagonalSetEnabled() const;

    // Obstacle Sector Checks
    bool isFrontBlocked(float threshold) const;
    bool isRearBlocked(float threshold) const;
    bool isLeftBlocked(float threshold) const;
    bool isRightBlocked(float threshold) const;
    bool isBothSidesBlocked(float threshold) const;
    bool isTrapped(float threshold) const;

    // Direction Solver: Returns relative angle in degrees (-180 to +180) of maximum clearance
    float getBestClearanceAngle(float threshold) const;

    // Overall Status Evaluation
    ObstacleStatus evaluateStatus(float threshold) const;

    // Direct ISR handlers
    static void IRAM_ATTR handleEchoChange(uint8_t index);

private:
    void triggerPulse();

    static volatile uint32_t echoStartMicros[NUM_ULTRASONIC_SENSORS];
    static volatile uint32_t echoDurationMicros[NUM_ULTRASONIC_SENSORS];
    static volatile bool echoReceived[NUM_ULTRASONIC_SENSORS];

    float rawHistory[NUM_ULTRASONIC_SENSORS][2]; // Double-read validation buffer
    uint8_t sampleCount[NUM_ULTRASONIC_SENSORS];
    float smoothedDistances[NUM_ULTRASONIC_SENSORS];
    uint32_t lastTriggerTime;
    bool triggerPending;
    bool diagonalSetEnabled;
};

extern SensorsManager sensors;
