#pragma once
#include <Arduino.h>
#include "MPU9250/MPU9250.h"
#include "Observer.h"
#include "IMUStatus.h"
#include "AHRS/MadgwickAHRS.h"
#include "AHRS/Adafruit_AHRS_Mahony.h"

#define CAL_SIG 0x2452
#define CAL_OFFISET 0
struct sIMUCalibration
{
    int16_t signature;
    float magMinX;
    float magMaxX;
    float magMinY;
    float magMaxY;    
    float magMinZ;
    float magMaxZ;    
    float magBiasX;
    float magBiasY;
    float magBiasZ;
    float magScaleX;
    float magScaleY;
    float magScaleZ;

    float accBiasX;
    float accBiasY;
    float accBiasZ;
    float accScaleX;
    float accScaleY;
    float accScaleZ;

    float gyroBiasX;
    float gyroBiasY;
    float gyroBiasZ;
    sIMUCalibration()
    {
        signature = 0;
        magMaxX = magMaxY = magMaxZ = magMinX = magMinY = magMinZ = 0;
        magBiasX = magBiasY = magBiasZ = magScaleX = magScaleY = magScaleZ = 0;
        accBiasX = accBiasY = accBiasZ = accScaleX = accScaleY = accScaleZ = 0;
        gyroBiasX = gyroBiasY = gyroBiasZ = 0;
    }
};

class cIMU : public Observable<void *>
{
protected:
    MPU9250 *m_imu;
    bool m_isConnected;
    int64_t m_lastUpdate;
    //Adafruit_AHRS_FusionInterface *m_filter;
    Madgwick *m_filter;
    sIMUCalibration m_calibration;
public:
    Observable<const IMUStatus *> newStatus;
    cIMU();
    virtual ~cIMU();
    virtual bool setup();
    virtual void loop();
    void calibrate();
    void shutdown();
    void updateCalibration();
    void saveCalibration();
    void loadCalibration();
    void setAutoMag(bool v) { m_imu->setAutoMag(v); }
    bool getAutoMag() { return m_imu->getAutoMag(); }
    void adjustMagCalibrationBias(float value,uint8_t axis);
    sIMUCalibration &getCalibration() { return m_calibration; }
};
