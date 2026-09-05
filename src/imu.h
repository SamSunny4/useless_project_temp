#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "config.h"

class ImuManager {
public:
    ImuManager();
    bool init();
    void update(); // Continuous non-blocking integration

    void calibrate(uint16_t samples = 250);
    void resetHeading(float angleDeg = 0.0f);
    void resetPosition(float xCm = 0.0f, float yCm = 0.0f);
    void setPose(float xCm, float yCm, float angleDeg);

    // Orientation & Rates
    float getYaw() const;          // Normalized -180.0 to +180.0 degrees
    float getRawYaw() const;       // Continuous un-wrapped degrees
    float getPitch() const;        // Inclination [-90, +90] deg
    float getRoll() const;         // Roll [-180, +180] deg
    float getHeadingError(float targetYaw) const; // Shortest angular distance [-180, +180]
    float getAngularVelocityZ() const; // deg/sec

    // Accelerometer (in Gs)
    float getAccelX() const;
    float getAccelY() const;
    float getAccelZ() const;
    float getDynamicAcceleration() const; // Net dynamic acceleration magnitude |a - 1.0g|

    // Odometry Position (in cm)
    float getPosX() const;
    float getPosY() const;
    float getTotalDistance() const;

    bool isConnected() const;

private:
    bool writeRegister(uint8_t reg, uint8_t val);
    bool readRegisters(uint8_t reg, uint8_t* buffer, size_t length);

    float currentYaw;
    float pitch;
    float roll;
    float gyroBiasZ;
    float rateZ;
    float accelX, accelY, accelZ;

    // Odometry & Dead-reckoning
    float posX;
    float posY;
    float totalDistance;

    uint32_t lastMicros;
    bool connected;
};

extern ImuManager imu;
