#include "imu.h"
#include "motors.h"
#include <math.h>

#define MPU6050_SMPLRT_DIV   0x19
#define MPU6050_CONFIG       0x1A
#define MPU6050_GYRO_CONFIG  0x1B
#define MPU6050_ACCEL_CONFIG 0x1C
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_WHO_AM_I     0x75
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_GYRO_ZOUT_H  0x47

// Sensitivity factors:
// Gyro FS_SEL = 1 (+/- 500 deg/s) -> 65.5 LSB per deg/s
#define GYRO_SCALE_500DPS    65.5f
// Accel AFS_SEL = 0 (+/- 2g) -> 16384 LSB per g
#define ACCEL_SCALE_2G       16384.0f

ImuManager imu;

ImuManager::ImuManager()
    : currentYaw(0.0f),
      pitch(0.0f),
      roll(0.0f),
      gyroBiasZ(0.0f),
      rateZ(0.0f),
      accelX(0.0f),
      accelY(0.0f),
      accelZ(1.0f),
      posX(0.0f),
      posY(0.0f),
      totalDistance(0.0f),
      lastMicros(0),
      connected(false) {}

bool ImuManager::writeRegister(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(MPU6050_I2C_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return (Wire.endTransmission() == 0);
}

bool ImuManager::readRegisters(uint8_t reg, uint8_t* buffer, size_t length) {
    Wire.beginTransmission(MPU6050_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;

    Wire.requestFrom((uint8_t)MPU6050_I2C_ADDR, (uint8_t)length);
    for (size_t i = 0; i < length && Wire.available(); i++) {
        buffer[i] = Wire.read();
    }
    return true;
}

bool ImuManager::init() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000); // 400kHz Fast I2C

    uint8_t whoAmI = 0;
    if (!readRegisters(MPU6050_WHO_AM_I, &whoAmI, 1) || whoAmI != 0x68) {
        Serial.printf("[IMU] MPU6050 not detected at 0x68 (WHO_AM_I: 0x%02X). Odometry fallback active.\n", whoAmI);
        connected = false;
        lastMicros = micros();
        return false;
    }

    // Wake up MPU6050 (clear sleep bit)
    writeRegister(MPU6050_PWR_MGMT_1, 0x00);
    delay(10);

    // Set sample rate divider = 7 -> 1kHz / (1 + 7) = 125 Hz
    writeRegister(MPU6050_SMPLRT_DIV, 0x07);

    // Digital Low Pass Filter = 44 Hz bandwidth
    writeRegister(MPU6050_CONFIG, 0x03);

    // Gyro Full Scale = +/- 500 deg/s
    writeRegister(MPU6050_GYRO_CONFIG, 0x08);

    // Accel Full Scale = +/- 2g
    writeRegister(MPU6050_ACCEL_CONFIG, 0x00);

    connected = true;
    Serial.println("[IMU] MPU6050 6-DOF IMU initialized successfully.");

    calibrate(250);
    lastMicros = micros();
    return true;
}

void ImuManager::calibrate(uint16_t samples) {
    if (!connected) return;

    Serial.println("[IMU] Calibrating Gyro Z-axis. Keep bot stationary...");
    float sumZ = 0.0f;
    uint8_t buf[2];

    for (uint16_t i = 0; i < samples; i++) {
        if (readRegisters(MPU6050_GYRO_ZOUT_H, buf, 2)) {
            int16_t rawZ = (int16_t)((buf[0] << 8) | buf[1]);
            sumZ += (float)rawZ / GYRO_SCALE_500DPS;
        }
        delay(4);
    }
    gyroBiasZ = sumZ / (float)samples;
    Serial.printf("[IMU] Calibration Complete. Gyro Bias Z: %.3f deg/s\n", gyroBiasZ);
}

void ImuManager::update() {
    uint32_t now = micros();
    if (lastMicros == 0) {
        lastMicros = now;
        return;
    }

    float dt = (float)(now - lastMicros) / 1000000.0f;
    lastMicros = now;

    if (dt <= 0.0f || dt > 0.5f) {
        // Prevent huge integration jump after long delay
        return;
    }

    if (connected) {
        // Burst read 14 registers: Accel (6), Temp (2), Gyro (6)
        uint8_t buf[14];
        if (readRegisters(MPU6050_ACCEL_XOUT_H, buf, 14)) {
            int16_t rawAX = (int16_t)((buf[0] << 8) | buf[1]);
            int16_t rawAY = (int16_t)((buf[2] << 8) | buf[3]);
            int16_t rawAZ = (int16_t)((buf[4] << 8) | buf[5]);
            int16_t rawGZ = (int16_t)((buf[12] << 8) | buf[13]);

            accelX = (float)rawAX / ACCEL_SCALE_2G;
            accelY = (float)rawAY / ACCEL_SCALE_2G;
            accelZ = (float)rawAZ / ACCEL_SCALE_2G;

            // Pitch and Roll calculation from gravity vector
            pitch = atan2(accelY, sqrt(accelX * accelX + accelZ * accelZ)) * (180.0f / PI);
            roll = atan2(-accelX, accelZ) * (180.0f / PI);

            // Gyro Z rate with bias compensation
            float rawRateZ = (float)rawGZ / GYRO_SCALE_500DPS;
            rateZ = rawRateZ - gyroBiasZ;

            // Deadband filter to eliminate stationary drift
            if (fabs(rateZ) < 0.25f) {
                rateZ = 0.0f;
            }

            // Integrate yaw: Note Z-axis right-hand rule (counter-clockwise is positive)
            currentYaw += rateZ * dt;
        }
    } else {
        // Odometry / Kinematic rotation estimation when physical MPU6050 is offline
        bool r1 = motors.isRelay1On();
        bool r2 = motors.isRelay2On();
        if (r1 && !r2) {
            // Left ON -> Rotating Right (clockwise = negative yaw in right-hand rule)
            rateZ = -120.0f;
            currentYaw += rateZ * dt;
        } else if (!r1 && r2) {
            // Right ON -> Rotating Left (counter-clockwise = positive yaw)
            rateZ = +120.0f;
            currentYaw += rateZ * dt;
        } else {
            rateZ = 0.0f;
        }
    }

    // Dead-reckoning position odometry update
    bool r1 = motors.isRelay1On();
    bool r2 = motors.isRelay2On();
    if (r1 && r2) {
        // Forward motion: ~28 cm/sec at active relay duty
        float linearSpeed = 28.0f;
        float ds = linearSpeed * dt;

        float rad = getYaw() * (PI / 180.0f);
        posX += ds * sin(rad);
        posY += ds * cos(rad);
        totalDistance += ds;
    }
}

float ImuManager::getYaw() const {
    // Normalize to [-180.0, +180.0] degrees
    float y = fmod(currentYaw, 360.0f);
    if (y > 180.0f) y -= 360.0f;
    else if (y < -180.0f) y += 360.0f;
    return y;
}

float ImuManager::getRawYaw() const {
    return currentYaw;
}

float ImuManager::getPitch() const {
    return pitch;
}

float ImuManager::getRoll() const {
    return roll;
}

float ImuManager::getHeadingError(float targetYaw) const {
    float diff = targetYaw - getYaw();
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return diff;
}

float ImuManager::getAngularVelocityZ() const {
    return rateZ;
}

float ImuManager::getAccelX() const {
    return accelX;
}

float ImuManager::getAccelY() const {
    return accelY;
}

float ImuManager::getAccelZ() const {
    return accelZ;
}

float ImuManager::getDynamicAcceleration() const {
    if (!connected) return 0.0f;
    float mag = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);
    return fabs(mag - 1.0f);
}

float ImuManager::getPosX() const {
    return posX;
}

float ImuManager::getPosY() const {
    return posY;
}

float ImuManager::getTotalDistance() const {
    return totalDistance;
}

void ImuManager::resetHeading(float angleDeg) {
    currentYaw = angleDeg;
}

void ImuManager::resetPosition(float xCm, float yCm) {
    posX = xCm;
    posY = yCm;
    totalDistance = 0.0f;
}

void ImuManager::setPose(float xCm, float yCm, float angleDeg) {
    posX = xCm;
    posY = yCm;
    currentYaw = angleDeg;
}

bool ImuManager::isConnected() const {
    return connected;
}
