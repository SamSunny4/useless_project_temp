#include "evasion.h"
#include "sensors.h"
#include "imu.h"
#include "motors.h"
#include <math.h>

EvasionManager evasion;

EvasionManager::EvasionManager()
    : thresholdDistance(DEFAULT_THRESHOLD_CM),
      currentState(STATE_CLEAR_IDLE),
      currentStatus(STATUS_CLEAR),
      threatOrigin(STATUS_CLEAR),
      activeTapCmd(CMD_STOP),
      currentTapPhase(TAP_PHASE_PULSE),
      targetYaw(0.0f),
      phaseStartTime(0),
      consecutiveTaps(0),
      autoAlarmActive(false),
      manualAlarmOverride(false),
      alarmSilenced(false) {}

void EvasionManager::init() {
    thresholdDistance = DEFAULT_THRESHOLD_CM;
    currentState = STATE_CLEAR_IDLE;
    currentStatus = STATUS_CLEAR;
    threatOrigin = STATUS_CLEAR;
    activeTapCmd = CMD_STOP;
    currentTapPhase = TAP_PHASE_PULSE;
    targetYaw = 0.0f;
    phaseStartTime = millis();
    consecutiveTaps = 0;
    autoAlarmActive = false;
    manualAlarmOverride = false;
    alarmSilenced = false;

    pinMode(PIN_ALARM, OUTPUT);
    digitalWrite(PIN_ALARM, LOW);
}

void EvasionManager::setThreshold(float cm) {
    thresholdDistance = constrain(cm, MIN_EVADE_THRESHOLD_CM, 100.0f);
}

float EvasionManager::getThreshold() const {
    return thresholdDistance;
}

EvadeState EvasionManager::getCurrentState() const {
    return currentState;
}

ObstacleStatus EvasionManager::getObstacleStatus() const {
    return currentStatus;
}

float EvasionManager::getTargetYaw() const {
    return targetYaw;
}

uint8_t EvasionManager::getTapCount() const {
    return consecutiveTaps;
}

void EvasionManager::updateAlarmOutput() {
    bool active = manualAlarmOverride || (autoAlarmActive && !alarmSilenced);
    digitalWrite(PIN_ALARM, active ? HIGH : LOW);
}

bool EvasionManager::isAlarmActive() const {
    return manualAlarmOverride || (autoAlarmActive && !alarmSilenced);
}

bool EvasionManager::isManualAlarm() const {
    return manualAlarmOverride;
}

void EvasionManager::setManualAlarm(bool enable) {
    manualAlarmOverride = enable;
    if (enable) alarmSilenced = false;
    updateAlarmOutput();
}

void EvasionManager::toggleManualAlarm() {
    if (isAlarmActive()) {
        manualAlarmOverride = false;
        alarmSilenced = true;
    } else {
        manualAlarmOverride = true;
        alarmSilenced = false;
    }
    updateAlarmOutput();
}

void EvasionManager::silenceAlarm() {
    manualAlarmOverride = false;
    alarmSilenced = true;
    updateAlarmOutput();
}

void EvasionManager::update() {
    executeEvadeStateMachine();
}

void EvasionManager::executeEvadeStateMachine() {
    currentStatus = sensors.evaluateStatus(thresholdDistance);

    switch (currentState) {
        case STATE_CLEAR_IDLE: {
            motors.stop();
            consecutiveTaps = 0;

            if (currentStatus == STATUS_ALL_SIDES_TRAPPED || sensors.isTrapped(thresholdDistance)) {
                currentState = STATE_TRAPPED;
                autoAlarmActive = true;
                updateAlarmOutput();
                phaseStartTime = millis();
            } else {
                autoAlarmActive = false;
                alarmSilenced = false;
                updateAlarmOutput();

                if (currentStatus == STATUS_OBJECT_IN_REAR) {
                    // Rear threat detected: tap forward away from rear obstacle if front is clear
                    if (!sensors.isFrontBlocked(thresholdDistance)) {
                        currentState = STATE_TAP_FORWARD;
                        currentTapPhase = TAP_PHASE_PULSE;
                        activeTapCmd = CMD_FORWARD;
                        threatOrigin = STATUS_OBJECT_IN_REAR;
                        consecutiveTaps = 1;
                        phaseStartTime = millis();
                        motors.singleTap(CMD_FORWARD);
                    } else {
                        // Both front and rear tight -> tap rotate toward more open flank
                        currentState = STATE_TAP_ROTATING;
                        currentTapPhase = TAP_PHASE_PULSE;
                        activeTapCmd = (sensors.getDistance(1) > sensors.getDistance(3)) ? CMD_ROTATE_RIGHT : CMD_ROTATE_LEFT;
                        threatOrigin = STATUS_OBJECT_IN_FRONT;
                        consecutiveTaps = 1;
                        phaseStartTime = millis();
                        motors.singleTap(activeTapCmd);
                    }
                } else if (currentStatus == STATUS_OBJECT_ON_LEFT) {
                    // Left threat detected: tap turn RIGHT away from obstacle
                    currentState = STATE_TAP_ROTATING;
                    currentTapPhase = TAP_PHASE_PULSE;
                    activeTapCmd = CMD_ROTATE_RIGHT;
                    threatOrigin = STATUS_OBJECT_ON_LEFT;
                    consecutiveTaps = 1;
                    phaseStartTime = millis();
                    motors.singleTap(CMD_ROTATE_RIGHT);
                } else if (currentStatus == STATUS_OBJECT_ON_RIGHT) {
                    // Right threat detected: tap turn LEFT away from obstacle
                    currentState = STATE_TAP_ROTATING;
                    currentTapPhase = TAP_PHASE_PULSE;
                    activeTapCmd = CMD_ROTATE_LEFT;
                    threatOrigin = STATUS_OBJECT_ON_RIGHT;
                    consecutiveTaps = 1;
                    phaseStartTime = millis();
                    motors.singleTap(CMD_ROTATE_LEFT);
                } else if (currentStatus == STATUS_OBJECT_IN_FRONT || currentStatus == STATUS_OBJECT_BOTH_SIDES) {
                    // Front or both flanks threat: tap rotate away to more open flank
                    currentState = STATE_TAP_ROTATING;
                    currentTapPhase = TAP_PHASE_PULSE;
                    activeTapCmd = (sensors.getDistance(1) > sensors.getDistance(3)) ? CMD_ROTATE_RIGHT : CMD_ROTATE_LEFT;
                    threatOrigin = STATUS_OBJECT_IN_FRONT;
                    consecutiveTaps = 1;
                    phaseStartTime = millis();
                    motors.singleTap(activeTapCmd);
                } else {
                    // Path clear: hold stationary position
                    motors.stop();
                }
            }
            break;
        }

        case STATE_TAP_ROTATING: {
            uint32_t now = millis();

            if (currentTapPhase == TAP_PHASE_PULSE) {
                // Relay pulse phase: single tap energized for tapOnMs
                if (now - phaseStartTime >= motors.getTapOnMs()) {
                    motors.stop(); // De-energize relay
                    currentTapPhase = TAP_PHASE_MEASURE;
                    phaseStartTime = now;
                }
            } else {
                // Rest / Coast phase: relays OFF, chassis still, ultrasonic sensor measures cleanly
                if (now - phaseStartTime >= motors.getTapOffMs()) {
                    // --- CHECK MEASURE ---
                    bool threatCleared = false;

                    if (threatOrigin == STATUS_OBJECT_ON_LEFT) {
                        threatCleared = !sensors.isLeftBlocked(thresholdDistance);
                    } else if (threatOrigin == STATUS_OBJECT_ON_RIGHT) {
                        threatCleared = !sensors.isRightBlocked(thresholdDistance);
                    } else {
                        // Front or both sides
                        threatCleared = !sensors.isFrontBlocked(thresholdDistance);
                    }

                    if (threatCleared) {
                        // Obstacle cleared the threshold: evasion complete!
                        motors.stop();
                        currentState = STATE_CLEAR_IDLE;
                        consecutiveTaps = 0;
                    } else if (sensors.isTrapped(thresholdDistance)) {
                        // No more moves available: bot trapped, fire alarm
                        motors.stop();
                        currentState = STATE_TRAPPED;
                        autoAlarmActive = true;
                        updateAlarmOutput();
                        consecutiveTaps = 0;
                    } else if (consecutiveTaps >= 10) {
                        // Safety guard: max 10 taps reached, return to idle evaluation
                        motors.stop();
                        currentState = STATE_CLEAR_IDLE;
                        consecutiveTaps = 0;
                    } else {
                        // --- REPEAT ---
                        // Obstacle still present: execute next controlled tap pulse
                        consecutiveTaps++;
                        currentTapPhase = TAP_PHASE_PULSE;
                        phaseStartTime = now;
                        motors.singleTap(activeTapCmd);
                    }
                }
            }
            break;
        }

        case STATE_TAP_FORWARD: {
            uint32_t now = millis();

            if (currentTapPhase == TAP_PHASE_PULSE) {
                // Relay pulse phase: forward energized for tapOnMs
                if (now - phaseStartTime >= motors.getTapOnMs()) {
                    motors.stop(); // De-energize relays
                    currentTapPhase = TAP_PHASE_MEASURE;
                    phaseStartTime = now;
                }
            } else {
                // Rest / Coast phase: relays OFF, chassis still, ultrasonic sensor measures cleanly
                if (now - phaseStartTime >= motors.getTapOffMs()) {
                    // --- CHECK MEASURE ---
                    bool frontHazard = sensors.isFrontBlocked(CRITICAL_STOP_CM);
                    bool rearCleared = !sensors.isRearBlocked(thresholdDistance);

                    if (frontHazard || rearCleared) {
                        // Rear cleared or front danger reached: halt
                        motors.stop();
                        currentState = STATE_CLEAR_IDLE;
                        consecutiveTaps = 0;
                    } else if (sensors.isTrapped(thresholdDistance)) {
                        // Trapped while attempting forward move
                        motors.stop();
                        currentState = STATE_TRAPPED;
                        autoAlarmActive = true;
                        updateAlarmOutput();
                        consecutiveTaps = 0;
                    } else if (consecutiveTaps >= 8) {
                        // Safety guard: max 8 forward taps reached
                        motors.stop();
                        currentState = STATE_CLEAR_IDLE;
                        consecutiveTaps = 0;
                    } else {
                        // --- REPEAT ---
                        // Rear still blocked and front path open: execute next forward tap
                        consecutiveTaps++;
                        currentTapPhase = TAP_PHASE_PULSE;
                        phaseStartTime = now;
                        motors.singleTap(CMD_FORWARD);
                    }
                }
            }
            break;
        }

        case STATE_TRAPPED: {
            motors.stop();
            if (!sensors.isTrapped(thresholdDistance)) {
                currentState = STATE_CLEAR_IDLE;
                autoAlarmActive = false;
                alarmSilenced = false;
                updateAlarmOutput();
            } else {
                autoAlarmActive = true;
                updateAlarmOutput();
            }
            break;
        }
    }
}
