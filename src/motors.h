#pragma once
#include <Arduino.h>
#include "config.h"

enum MotorCommand {
    CMD_STOP = 0,
    CMD_FORWARD,
    CMD_BACKWARD,
    CMD_ROTATE_LEFT,
    CMD_ROTATE_RIGHT,
    CMD_PIVOT_LEFT,
    CMD_PIVOT_RIGHT
};

class MotorsManager {
public:
    MotorsManager();
    void init();

    // High-level tank steering primitives
    void forward(uint8_t speed = DEFAULT_SPEED);
    void backward(uint8_t speed = DEFAULT_SPEED);
    void rotateLeft(uint8_t speed = TURN_SPEED);
    void rotateRight(uint8_t speed = TURN_SPEED);
    void pivotLeft(uint8_t speed = TURN_SPEED);
    void pivotRight(uint8_t speed = TURN_SPEED);
    void stop();

    // Low-level direct tank speed control (-255 to +255)
    void setSpeeds(int16_t leftSpeed, int16_t rightSpeed);

    // Emergency Stop
    void emergencyStop();
    void resetEmergencyStop();
    bool isEmergencyStopped() const;

    // Stall Watchdog (1-second E-Stop when throttle is on without acceleration)
    void checkStallWatchdog(float dynamicAccel, float gyroRateZ);
    void triggerStallEstop();
    bool isStallEstopActive() const;
    void setStallAccelThreshold(float g);
    float getStallAccelThreshold() const;

    // Dynamic max speed configuration
    void setBaseSpeed(uint8_t speed);
    uint8_t getBaseSpeed() const;

    MotorCommand getCurrentCommand() const;
    void getSpeeds(int16_t &left, int16_t &right) const;

    // Relay state queries
    bool isRelay1On() const;
    bool isRelay2On() const;

    // Relay Tap & Inching Engine
    void update(); // Non-blocking tap duty-cycle engine
    void singleTap(MotorCommand cmd); // Executes one discrete pulse and coasts
    void setTapSpeed(uint8_t speed); // Dynamically maps speed to tapOnMs & tapOffMs
    void setTapTiming(uint16_t onMs, uint16_t offMs);
    uint16_t getTapOnMs() const;
    uint16_t getTapOffMs() const;
    bool isTapping() const;

private:
    void applyRelays(bool relay1On, bool relay2On);
    void applyCommandRelays(MotorCommand cmd);

    uint8_t baseSpeed;
    int16_t currentLeftSpeed;
    int16_t currentRightSpeed;
    MotorCommand currentCmd;
    bool eStopActive;

    // Stall safety tracking
    bool stallEstopActive;
    uint32_t stallEstopEndTime;
    uint32_t throttleOnStartTime;
    uint32_t lastMotionTime;
    float stallAccelThreshold;

    // Tap pulse engine variables
    bool tapModeActive;
    bool inTapPulse;
    bool singleTapOnly;
    uint32_t tapCycleStartTime;
    uint16_t tapOnMs;
    uint16_t tapOffMs;

    bool relay1State;
    bool relay2State;
};

extern MotorsManager motors;
