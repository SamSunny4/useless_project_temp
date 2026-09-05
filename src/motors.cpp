#include "motors.h"

MotorsManager motors;

MotorsManager::MotorsManager()
    : baseSpeed(DEFAULT_SPEED),
      currentLeftSpeed(0),
      currentRightSpeed(0),
      currentCmd(CMD_STOP),
      eStopActive(true),
      stallEstopActive(false),
      stallEstopEndTime(0),
      throttleOnStartTime(0),
      lastMotionTime(0),
      stallAccelThreshold(DEFAULT_STALL_ACCEL_THRESHOLD),
      tapModeActive(false),
      inTapPulse(false),
      singleTapOnly(false),
      tapCycleStartTime(0),
      tapOnMs(DEFAULT_TAP_ON_MS),
      tapOffMs(DEFAULT_TAP_OFF_MS),
      relay1State(false),
      relay2State(false) {}

void MotorsManager::init() {
    pinMode(PIN_RELAY_1, OUTPUT);
    pinMode(PIN_RELAY_2, OUTPUT);

    // Initial safe boot state: Both relays de-energized
    digitalWrite(PIN_RELAY_1, RELAY_OFF);
    digitalWrite(PIN_RELAY_2, RELAY_OFF);

    relay1State = false;
    relay2State = false;
    eStopActive = true;
    stallEstopActive = false;
    stallEstopEndTime = 0;
    throttleOnStartTime = 0;
    lastMotionTime = 0;
    stallAccelThreshold = DEFAULT_STALL_ACCEL_THRESHOLD;
    setTapSpeed(DEFAULT_SPEED);

    stop();
    Serial.println("[RelayMotors] 2-Channel Relay Module initialized in SAFETY E-STOP state.");
}

void MotorsManager::applyRelays(bool r1, bool r2) {
    if (eStopActive || stallEstopActive) {
        digitalWrite(PIN_RELAY_1, RELAY_OFF);
        digitalWrite(PIN_RELAY_2, RELAY_OFF);
        relay1State = false;
        relay2State = false;
        currentLeftSpeed = 0;
        currentRightSpeed = 0;
        return;
    }

    relay1State = r1;
    relay2State = r2;

    digitalWrite(PIN_RELAY_1, r1 ? RELAY_ON : RELAY_OFF);
    digitalWrite(PIN_RELAY_2, r2 ? RELAY_ON : RELAY_OFF);

    currentLeftSpeed = r1 ? 255 : 0;
    currentRightSpeed = r2 ? 255 : 0;
}

void MotorsManager::applyCommandRelays(MotorCommand cmd) {
    switch (cmd) {
        case CMD_FORWARD:
            applyRelays(true, true);
            break;
        case CMD_ROTATE_LEFT:
        case CMD_PIVOT_LEFT:
            applyRelays(false, true); // Right motors ON -> Turn Left
            break;
        case CMD_ROTATE_RIGHT:
        case CMD_PIVOT_RIGHT:
            applyRelays(true, false); // Left motors ON -> Turn Right
            break;
        case CMD_BACKWARD:
        case CMD_STOP:
        default:
            applyRelays(false, false);
            break;
    }
}

void MotorsManager::setSpeeds(int16_t leftSpeed, int16_t rightSpeed) {
    bool r1 = (leftSpeed > 0);
    bool r2 = (rightSpeed > 0);

    applyRelays(r1, r2);

    if (!r1 && !r2) currentCmd = CMD_STOP;
    else if (r1 && r2) currentCmd = CMD_FORWARD;
    else if (r1 && !r2) currentCmd = CMD_ROTATE_RIGHT;
    else if (!r1 && r2) currentCmd = CMD_ROTATE_LEFT;
}

void MotorsManager::forward(uint8_t speed) {
    if (speed != baseSpeed) setTapSpeed(speed);
    currentCmd = CMD_FORWARD;
    singleTapOnly = false;
    tapModeActive = true;
    inTapPulse = true;
    tapCycleStartTime = millis();
    applyCommandRelays(CMD_FORWARD);
}

void MotorsManager::backward(uint8_t speed) {
    // 2-channel relay cannot reverse DC polarity -> safely halt
    stop();
}

void MotorsManager::rotateLeft(uint8_t speed) {
    if (speed != baseSpeed) setTapSpeed(speed);
    currentCmd = CMD_ROTATE_LEFT;
    singleTapOnly = false;
    tapModeActive = true;
    inTapPulse = true;
    tapCycleStartTime = millis();
    applyCommandRelays(CMD_ROTATE_LEFT);
}

void MotorsManager::rotateRight(uint8_t speed) {
    if (speed != baseSpeed) setTapSpeed(speed);
    currentCmd = CMD_ROTATE_RIGHT;
    singleTapOnly = false;
    tapModeActive = true;
    inTapPulse = true;
    tapCycleStartTime = millis();
    applyCommandRelays(CMD_ROTATE_RIGHT);
}

void MotorsManager::pivotLeft(uint8_t speed) {
    rotateLeft(speed);
}

void MotorsManager::pivotRight(uint8_t speed) {
    rotateRight(speed);
}

void MotorsManager::singleTap(MotorCommand cmd) {
    if (cmd == CMD_STOP || eStopActive) {
        stop();
        return;
    }
    currentCmd = cmd;
    singleTapOnly = true;
    tapModeActive = true;
    inTapPulse = true;
    tapCycleStartTime = millis();
    applyCommandRelays(cmd);
}

void MotorsManager::stop() {
    currentCmd = CMD_STOP;
    tapModeActive = false;
    inTapPulse = false;
    singleTapOnly = false;
    applyRelays(false, false);
}

void MotorsManager::update() {
    if (eStopActive || !tapModeActive || currentCmd == CMD_STOP) {
        if (relay1State || relay2State) {
            applyRelays(false, false);
        }
        return;
    }

    uint32_t now = millis();
    uint32_t elapsed = now - tapCycleStartTime;

    if (inTapPulse) {
        // Active ON pulse phase
        if (elapsed >= tapOnMs) {
            // Pulse duration expired: de-energize relays for quiet coast/measurement window
            applyRelays(false, false);
            inTapPulse = false;
            tapCycleStartTime = now;

            if (singleTapOnly) {
                tapModeActive = false;
                currentCmd = CMD_STOP;
            }
        }
    } else {
        // Quiet OFF coast / measurement phase
        if (elapsed >= tapOffMs) {
            // Coast duration expired: start next pulse if still driving
            if (!singleTapOnly && currentCmd != CMD_STOP) {
                inTapPulse = true;
                tapCycleStartTime = now;
                applyCommandRelays(currentCmd);
            } else {
                tapModeActive = false;
            }
        }
    }
}

void MotorsManager::setBaseSpeed(uint8_t speed) {
    setTapSpeed(speed);
}

uint8_t MotorsManager::getBaseSpeed() const {
    return baseSpeed;
}

void MotorsManager::setTapSpeed(uint8_t speed) {
    speed = constrain(speed, 50, 255);
    baseSpeed = speed;

    // Map speed (50 to 255) to tap ON pulse width (MIN_TAP_ON_MS to MAX_TAP_ON_MS)
    tapOnMs = map(speed, 50, 255, MIN_TAP_ON_MS, MAX_TAP_ON_MS);

    // Inversely map speed to tap OFF rest duration (longer pause at lower speed)
    tapOffMs = map(speed, 50, 255, 250, MIN_TAP_OFF_MS);
}

void MotorsManager::setTapTiming(uint16_t onMs, uint16_t offMs) {
    tapOnMs = constrain(onMs, (uint16_t)MIN_TAP_ON_MS, (uint16_t)MAX_TAP_ON_MS);
    tapOffMs = constrain(offMs, (uint16_t)MIN_TAP_OFF_MS, (uint16_t)MAX_TAP_OFF_MS);
}

uint16_t MotorsManager::getTapOnMs() const {
    return tapOnMs;
}

uint16_t MotorsManager::getTapOffMs() const {
    return tapOffMs;
}

bool MotorsManager::isTapping() const {
    return tapModeActive;
}

MotorCommand MotorsManager::getCurrentCommand() const {
    return currentCmd;
}

void MotorsManager::getSpeeds(int16_t &left, int16_t &right) const {
    left = currentLeftSpeed;
    right = currentRightSpeed;
}

bool MotorsManager::isRelay1On() const {
    return relay1State;
}

bool MotorsManager::isRelay2On() const {
    return relay2State;
}

void MotorsManager::emergencyStop() {
    eStopActive = true;
    tapModeActive = false;
    inTapPulse = false;
    singleTapOnly = false;
    applyRelays(false, false);
    currentCmd = CMD_STOP;
    Serial.println("[RelayMotors] >>> EMERGENCY STOP ACTIVATED! ALL RELAYS DE-ENERGIZED <<<");
}

void MotorsManager::resetEmergencyStop() {
    eStopActive = false;
    tapModeActive = false;
    inTapPulse = false;
    singleTapOnly = false;
    applyRelays(false, false);
    currentCmd = CMD_STOP;
    Serial.println("[RelayMotors] Emergency stop reset. Relay controls restored.");
}

void MotorsManager::triggerStallEstop() {
    stallEstopActive = true;
    stallEstopEndTime = millis() + STALL_ESTOP_DURATION_MS;
    stop();
    Serial.println("\n>>> [SAFETY ALERT] Stall detected (throttle ON without acceleration)! 1-second Emergency Stop triggered. <<<");
}

bool MotorsManager::isStallEstopActive() const {
    return stallEstopActive;
}

void MotorsManager::setStallAccelThreshold(float g) {
    stallAccelThreshold = constrain(g, MIN_STALL_ACCEL_THRESHOLD, MAX_STALL_ACCEL_THRESHOLD);
}

float MotorsManager::getStallAccelThreshold() const {
    return stallAccelThreshold;
}

void MotorsManager::checkStallWatchdog(float dynamicAccel, float gyroRateZ) {
    uint32_t now = millis();

    // 1. Check active stall cooldown
    if (stallEstopActive) {
        if (now >= stallEstopEndTime) {
            stallEstopActive = false;
            Serial.println("[SAFETY] Stall 1-second cooldown complete. Motion resumed.");
        } else {
            // Still in stall cooldown: force relays OFF
            if (relay1State || relay2State) {
                applyRelays(false, false);
            }
            return;
        }
    }

    // 2. Check if throttle is actively driving or pulsing
    bool throttleOn = (currentCmd != CMD_STOP) && (tapModeActive || relay1State || relay2State);

    if (!throttleOn) {
        throttleOnStartTime = 0;
        lastMotionTime = now;
        return;
    }

    // Throttle is ON: start timing
    if (throttleOnStartTime == 0) {
        throttleOnStartTime = now;
        lastMotionTime = now;
    }

    // 3. Motion detection: IMU dynamic linear acceleration (>stallAccelThreshold) or gyro turn rate (>4.0 deg/s)
    bool hasMotion = (dynamicAccel > stallAccelThreshold) || (fabs(gyroRateZ) > 4.0f);

    if (hasMotion) {
        lastMotionTime = now;
    } else {
        // No motion detected while throttle is ON for > 600ms
        if ((now - throttleOnStartTime >= 600) && (now - lastMotionTime >= 600)) {
            triggerStallEstop();
        }
    }
}

bool MotorsManager::isEmergencyStopped() const {
    return eStopActive || stallEstopActive;
}
