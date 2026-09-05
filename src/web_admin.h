#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include "config.h"

class WebAdminManager {
public:
    WebAdminManager();
    void init();
    void update(); // Non-blocking handleClient() and ArduinoOTA.handle()

    RobotControlMode getActiveMode() const;
    void setActiveMode(RobotControlMode mode);
    bool isWebOverrideActive() const;
    bool isConnected() const;

private:
    void setupRoutes();
    void handleRoot();
    void handleApiStatus();
    void handleApiControl();
    void handleApiEstop();
    void handleApiMode();
    void handleApiConfig();
    void handleApiImuReset();
    void handleApiSensorsMode();
    void handleApiAlarm();

    WebServer server;
    RobotControlMode activeMode;
    uint32_t lastWebCmdTime;
    String localIPStr;

    bool wifiWasConnected;
    uint32_t lastLedBlinkTime;
    bool ledState;
    uint32_t lastDisconnectAlertTime;
};

extern WebAdminManager webAdmin;
