/*
 * ==============================================================================
 * TinkerHub CyberBot: Autonomous Evade Bot with 2-Channel Relay & 4 Ultrasonic Sensors
 * ==============================================================================
 * ESP32 DevKit V1 Firmware
 * Compatible with PlatformIO and Arduino IDE 2.x
 *
 * Subsystems:
 * - 4-Directional Ultrasonic Radar (90° Spacing, Non-blocking ISR based)
 * - MPU6050 6-DOF IMU (Gyro Yaw Tracking)
 * - 2-Channel Relay Actuator (Digital Left/Right Motor Switching)
 * - FreeRTOS Dual-Core Processing (Core 0: WiFi/Web/OTA, Core 1: Control/Sensors)
 * - Bi-Directional UART2 Telemetry with Raspberry Pi
 * - Cyberpunk-themed Web Admin Portal with Live Radar Visualizer & Virtual Controls
 * - ArduinoOTA (Over-The-Air Wireless Flashing)
 */

#include "src/config.h"
#include "src/sensors.h"
#include "src/imu.h"
#include "src/motors.h"
#include "src/evasion.h"
#include "src/web_admin.h"

TaskHandle_t NetworkTaskHandle = NULL;

void networkTask(void *pvParameters) {
    webAdmin.init();
    for (;;) {
        webAdmin.update();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=======================================================");
    Serial.println("   TINKERHUB CYBERBOT - ESP32 2-CHANNEL RELAY CONTROLLER");
    Serial.println("=======================================================");

    motors.init();
    sensors.init();
    imu.init();
    evasion.init();

    xTaskCreatePinnedToCore(
        networkTask,
        "NetworkTask",
        8192,
        NULL,
        1,
        &NetworkTaskHandle,
        0
    );

    Serial.println("[System] FreeRTOS initialized. Bot operational.");
}

// --- Serial Monitor Telemetry Reporting ---
static uint32_t lastSerialMonitorPrint = 0;
static ObstacleStatus lastReportedStatus = (ObstacleStatus)-1;

void printSerialMonitorTelemetry(RobotControlMode mode, ObstacleStatus status) {
    // Alert immediately on obstacle status change
    if (status != lastReportedStatus) {
        lastReportedStatus = status;
        const char* stStr = "CLEAR";
        if (status == STATUS_OBJECT_IN_FRONT)        stStr = "OBJECT_IN_FRONT";
        else if (status == STATUS_OBJECT_ON_LEFT)    stStr = "OBJECT_ON_LEFT";
        else if (status == STATUS_OBJECT_ON_RIGHT)   stStr = "OBJECT_ON_RIGHT";
        else if (status == STATUS_OBJECT_IN_REAR)    stStr = "OBJECT_IN_REAR";
        else if (status == STATUS_OBJECT_BOTH_SIDES) stStr = "OBJECT_BOTH_SIDES";
        else if (status == STATUS_ALL_SIDES_TRAPPED) stStr = "ALL_SIDES_TRAPPED (ALARM ON)";

        Serial.printf("\n>>> [ALERT: STATUS CHANGED] >>> %s (Threshold: %.1f cm) <<<\n", stStr, evasion.getThreshold());
    }

    // Print periodic telemetry HUD at 4 Hz (every 250ms)
    uint32_t now = millis();
    if (now - lastSerialMonitorPrint < 250) return;
    lastSerialMonitorPrint = now;

    const char* modeStr = "AUTO_EVADE";
    if (motors.isEmergencyStopped()) modeStr = "EMERG_STOP ";
    else if (mode == MODE_WEB_OVERRIDE) modeStr = "WEB_OVERRIDE";

    const char* alarmStr = evasion.isAlarmActive() ? " [TASER ON]" : "";

    int16_t lSpd, rSpd;
    motors.getSpeeds(lSpd, rSpd);

    if (sensors.isDiagonalSetEnabled()) {
        Serial.printf("[YAW:%+6.1f°] [%s%s] [MOT:L=%+4d R=%+4d] | F:%.0f R:%.0f B:%.0f L:%.0f | RR:%.0f RL:%.0f\n",
            imu.getYaw(),
            modeStr, alarmStr,
            lSpd, rSpd,
            sensors.getDistance(0), sensors.getDistance(1), sensors.getDistance(2), sensors.getDistance(3),
            sensors.getDistance(4), sensors.getDistance(5)
        );
    } else {
        Serial.printf("[YAW:%+6.1f°] [%s%s] [MOT:L=%+4d R=%+4d] | S0(F):%5.1f S1(R):%5.1f S2(B):%5.1f S3(L):%5.1f\n",
            imu.getYaw(),
            modeStr, alarmStr,
            lSpd, rSpd,
            sensors.getDistance(0),
            sensors.getDistance(1),
            sensors.getDistance(2),
            sensors.getDistance(3)
        );
    }
}

void loop() {
    imu.update();
    sensors.update();
    motors.update();

    // Stall Safety Watchdog: throttle commanded ON but no acceleration triggers 1s E-Stop
    motors.checkStallWatchdog(imu.getDynamicAcceleration(), imu.getAngularVelocityZ());

    RobotControlMode activeMode;
    if (webAdmin.isWebOverrideActive()) {
        activeMode = MODE_WEB_OVERRIDE;
    } else {
        activeMode = MODE_AUTO_EVADE;
    }

    if (motors.isEmergencyStopped()) {
        motors.stop();
    } else if (activeMode == MODE_AUTO_EVADE) {
        evasion.update();
    }

    ObstacleStatus status = evasion.getObstacleStatus();

    // Print live dashboard to USB Serial Monitor
    printSerialMonitorTelemetry(activeMode, status);

    delayMicroseconds(500);
}
