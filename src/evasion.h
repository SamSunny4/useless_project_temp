#pragma once
#include <Arduino.h>
#include "config.h"
#include "motors.h"

enum EvadeState {
    STATE_CLEAR_IDLE = 0,
    STATE_TAP_ROTATING,
    STATE_TAP_FORWARD,
    STATE_TRAPPED,
    // Aliases for telemetry and visualizer compatibility
    STATE_ROTATING_TO_BEST_ANGLE = STATE_TAP_ROTATING,
    STATE_PUSHING_CLEAR = STATE_TAP_FORWARD,
    STATE_BACKING_UP = STATE_TAP_FORWARD
};

enum TapPhase {
    TAP_PHASE_PULSE = 0,   // Relay ON for tapOnMs
    TAP_PHASE_MEASURE      // Relay OFF for tapOffMs (chassis still, sensors measure clean)
};

class EvasionManager {
public:
    EvasionManager();
    void init();
    void update(); // Non-blocking state machine tick

    void setThreshold(float cm);
    float getThreshold() const;

    EvadeState getCurrentState() const;
    ObstacleStatus getObstacleStatus() const;
    float getTargetYaw() const;
    uint8_t getTapCount() const;
    bool isAlarmActive() const;
    bool isManualAlarm() const;
    void setManualAlarm(bool enable);
    void toggleManualAlarm();
    void silenceAlarm();

private:
    void executeEvadeStateMachine();
    void updateAlarmOutput();

    float thresholdDistance;
    EvadeState currentState;
    ObstacleStatus currentStatus;
    ObstacleStatus threatOrigin;
    MotorCommand activeTapCmd;
    TapPhase currentTapPhase;

    float targetYaw;
    uint32_t phaseStartTime;
    uint8_t consecutiveTaps;
    bool autoAlarmActive;
    bool manualAlarmOverride;
    bool alarmSilenced;
};

extern EvasionManager evasion;
