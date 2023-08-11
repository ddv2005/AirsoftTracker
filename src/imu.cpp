#include "imu.h"
#include "EEPROM.h"

#define Pi 3.14159265359
//#define IMU_DEBUG

cIMU::cIMU()
{
    m_imu = NULL;
    m_isConnected = false;
    m_lastUpdate = 0;
}

void cIMU::adjustMagCalibrationBias(float value,uint8_t axis)
{
    float o,s;
    switch(axis)
    {
        case 0:
        {
            o = m_imu->getMagBiasX_uT();
            s = m_imu->getMagScaleFactorX();
            break;
        }
        case 1:
        {
            o = m_imu->getMagBiasY_uT();
            s = m_imu->getMagScaleFactorY();
            break;
        }
        case 2:
        {
            o = m_imu->getMagBiasZ_uT();
            s = m_imu->getMagScaleFactorZ();
            break;
        }
    }
    o+=value;
    switch(axis)
    {
        case 0:
        {
            m_imu->setMagCalX(o,s);
            break;
        }
        case 1:
        {
            m_imu->setMagCalY(o,s);
            break;
        }
        case 2:
        {
            m_imu->setMagCalZ(o,s);
            break;
        }
    }
    m_imu->recalibrateMagByBias();
    updateCalibration();
    DEBUG_MSG("Mag X: %0.2f/%0.2f, Y: %0.2f/%0.2f, Z: %0.2f/%0.2f\n",m_calibration.magBiasX,m_calibration.magScaleX,m_calibration.magBiasY,m_calibration.magScaleY,m_calibration.magBiasZ,m_calibration.magScaleZ);    
}

void cIMU::updateCalibration()
{
    m_calibration.signature = CAL_SIG;
    m_calibration.magBiasX = m_imu->getMagBiasX_uT();
    m_calibration.magBiasY = m_imu->getMagBiasY_uT();
    m_calibration.magBiasZ = m_imu->getMagBiasZ_uT();
    m_calibration.magScaleX = m_imu->getMagScaleFactorX();
    m_calibration.magScaleY = m_imu->getMagScaleFactorY();
    m_calibration.magScaleZ = m_imu->getMagScaleFactorZ();
    m_imu->getMagMinMax(m_calibration.magMinX,m_calibration.magMaxX,m_calibration.magMinY,m_calibration.magMaxY,m_calibration.magMinZ,m_calibration.magMaxZ);

    m_calibration.gyroBiasX = m_imu->getGyroBiasX_rads();
    m_calibration.gyroBiasY = m_imu->getGyroBiasY_rads();
    m_calibration.gyroBiasZ = m_imu->getGyroBiasZ_rads();
    
    m_calibration.accBiasX = m_imu->getAccelBiasX_mss();
    m_calibration.accScaleX = m_imu->getAccelScaleFactorX();
    m_calibration.accBiasY = m_imu->getAccelBiasY_mss();
    m_calibration.accScaleY = m_imu->getAccelScaleFactorY();
    m_calibration.accBiasZ = m_imu->getAccelBiasZ_mss();
    m_calibration.accScaleZ = m_imu->getAccelScaleFactorZ();
}

void cIMU::saveCalibration()
{
    updateCalibration();
    EEPROM.writeBytes(CAL_OFFISET,&m_calibration,sizeof(m_calibration));
    EEPROM.commit();
}

void cIMU::loadCalibration()
{
    EEPROM.readBytes(CAL_OFFISET,&m_calibration,sizeof(m_calibration));    
    if(m_calibration.signature==CAL_SIG)
    {
        m_imu->setGyroBiasX_rads(m_calibration.gyroBiasX);
        m_imu->setGyroBiasY_rads(m_calibration.gyroBiasY);
        m_imu->setGyroBiasZ_rads(m_calibration.gyroBiasZ);
        
        m_imu->setMagMinMax(m_calibration.magMinX,m_calibration.magMaxX,m_calibration.magMinY,m_calibration.magMaxY,m_calibration.magMinZ,m_calibration.magMaxZ);
        m_imu->setMagCalX(m_calibration.magBiasX,m_calibration.magScaleX);
        m_imu->setMagCalY(m_calibration.magBiasY,m_calibration.magScaleY);
        m_imu->setMagCalZ(m_calibration.magBiasZ,m_calibration.magScaleZ);

        m_imu->setAccelCalX(m_calibration.accBiasX,m_calibration.accScaleX);
        m_imu->setAccelCalY(m_calibration.accBiasY,m_calibration.accScaleY);
        m_imu->setAccelCalZ(m_calibration.accBiasZ,m_calibration.accScaleZ);
    }
}

void cIMU::shutdown()
{
}

void cIMU::calibrate()
{
    DEBUG_MSG("Calibrating Gyro\n");
    m_imu->calibrateGyro();
    delay(100);
    DEBUG_MSG("Calibrating Accel\n");
    m_imu->calibrateAccel();    
    delay(100);
    DEBUG_MSG("Calibrating MAG\n");
    m_imu->calibrateMag();
    saveCalibration();
}
bool cIMU::setup()
{
    EEPROM.readBytes(CAL_OFFISET,&m_calibration,sizeof(m_calibration));
    m_filter = new Madgwick();// new Adafruit_Mahony();
    m_filter->begin(50);
    m_lastUpdate = 0;
    m_isConnected = false;
    void *ptr = heap_caps_calloc(1,sizeof(MPU9250),MALLOC_CAP_8BIT);
    m_imu = new(ptr) MPU9250(Wire,0x68);
    auto status = m_imu->begin();
    if(status<0)
    {
        DEBUG_MSG("IMU initialization unsuccessful, %d\n",status);
    }
    else
    {
        DEBUG_MSG("IMU initialization successful, %d\n",status);
        m_isConnected = true;
        if(m_calibration.signature!=CAL_SIG)
        {
            calibrate();
        }
        else
        {
            DEBUG_MSG("Use  calibration data\n");
            m_imu->setGyroBiasX_rads(m_calibration.gyroBiasX);
            m_imu->setGyroBiasY_rads(m_calibration.gyroBiasY);
            m_imu->setGyroBiasZ_rads(m_calibration.gyroBiasZ);
            DEBUG_MSG("Mag X: %0.2f/%0.2f, Y: %0.2f/%0.2f, Z: %0.2f/%0.2f\n",m_calibration.magBiasX,m_calibration.magScaleX,m_calibration.magBiasY,m_calibration.magScaleY,m_calibration.magBiasZ,m_calibration.magScaleZ);
            DEBUG_MSG("Mag Min/Max X: %0.2f/%0.2f, Y: %0.2f/%0.2f, Z: %0.2f/%0.2f\n",m_calibration.magMinX,m_calibration.magMaxX,m_calibration.magMinY,m_calibration.magMaxY,m_calibration.magMinZ,m_calibration.magMaxZ);
            m_imu->setMagMinMax(m_calibration.magMinX,m_calibration.magMaxX,m_calibration.magMinY,m_calibration.magMaxY,m_calibration.magMinZ,m_calibration.magMaxZ);
            m_imu->setMagCalX(m_calibration.magBiasX,m_calibration.magScaleX);
            m_imu->setMagCalY(m_calibration.magBiasY,m_calibration.magScaleY);
            m_imu->setMagCalZ(m_calibration.magBiasZ,m_calibration.magScaleZ);

            m_imu->setAccelCalX(m_calibration.accBiasX,m_calibration.accScaleX);
            m_imu->setAccelCalY(m_calibration.accBiasY,m_calibration.accScaleY);
            m_imu->setAccelCalZ(m_calibration.accBiasZ,m_calibration.accScaleZ);
        }
        m_imu->setAccelRange(MPU9250::ACCEL_RANGE_2G);
        // setting the gyroscope full scale range to +/-500 deg/s
        m_imu->setGyroRange(MPU9250::GYRO_RANGE_500DPS);
        //m_imu->setGyroRange(MPU9250::GYRO_RANGE_500DPS);
        // setting DLPF bandwidth
        m_imu->setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_20HZ);
        // setting SRD to 19 for a 50 Hz update rate
        m_imu->setSrd(19);
    }
    return m_isConnected;
}

cIMU::~cIMU()
{
}

#define GYRO_SPEED 1

#ifdef IMU_DEBUG
int vvv=0;
#endif
void cIMU::loop()
{
    if(m_isConnected)
    {
        int64_t now = millis();
        if((now-m_lastUpdate)>=20)
        {
            float delta = (now-m_lastUpdate)/1000.0;
            if(delta>0.5f)
                delta = 0.5f;
            m_lastUpdate = now;
            m_imu->readSensor();
            m_filter->update(m_imu->getGyroX_rads()/GYRO_SPEED,m_imu->getGyroY_rads()/GYRO_SPEED,m_imu->getGyroZ_rads()/GYRO_SPEED,m_imu->getAccelX_mss(), m_imu->getAccelY_mss(),m_imu->getAccelZ_mss(),m_imu->getMagX_uT(), m_imu->getMagY_uT(),m_imu->getMagZ_uT(),delta);
/*            
            float xM = m_imu->getMagX_uT();
            float yM = m_imu->getMagY_uT();
            float zM = m_imu->getMagZ_uT();
            
            float g = 9.8;
            float xG = (m_imu->getAccelX_mss()) / g; 
            float yG = (m_imu->getAccelY_mss()) / g; 
            float zG = (m_imu->getAccelZ_mss()) / g;
            float pitch = atan2(-xG, sqrt(yG * yG + zG * zG));
            float roll = atan2(yG, zG);

            //float pitch = m_filter.getPitchRadians();
            //float roll = m_filter.getRollRadians();
            float xM2 = xM * cos(pitch) + zM * sin(pitch);
            float yM2 = xM * sin(roll) * sin(pitch) + yM * cos(roll) - zM * sin(roll) * cos(pitch);
            float compHeading2 = (atan2(yM2, xM2) * 180 / Pi)-90;
            if (compHeading2 < 0) {
                compHeading2 = 360 + compHeading2;
            }
*/            
            float compHeading = 90-m_filter->getYaw();
            if(compHeading<0) compHeading+=360;
            const IMUStatus status =  IMUStatus(true,compHeading*100);
            newStatus.notifyObservers(&status);
#ifdef IMU_DEBUG            
             vvv++;
             if(vvv%5==0)
             {
                m_imu->getMagMinMax(m_calibration.magMinX,m_calibration.magMaxX,m_calibration.magMinY,m_calibration.magMaxY,m_calibration.magMinZ,m_calibration.magMaxZ);
                Serial.print(m_imu->getAccelX_mss(),6);
                Serial.print("\t");
                Serial.print(m_imu->getAccelY_mss(),6);
                Serial.print("\t");
                Serial.print(m_imu->getAccelZ_mss(),6);
                Serial.print("\t");
                Serial.print(m_imu->getGyroX_rads(),6);
                Serial.print("\t");
                Serial.print(m_imu->getGyroY_rads(),6);
                Serial.print("\t");
                Serial.print(m_imu->getGyroZ_rads(),6);
                Serial.print("\t");
                Serial.print(m_imu->getMagX_uT(),6);
                Serial.print("\t");
                Serial.print(m_imu->getMagY_uT(),6);
                Serial.print("\t");
                Serial.print(m_imu->getMagZ_uT(),6);
                Serial.print("\t");
                Serial.print(m_imu->getRawMagX_uT(),6);
                Serial.print("\t");
                Serial.print(m_imu->getRawMagY_uT(),6);
                Serial.print("\t");
                Serial.print(m_imu->getRawMagZ_uT(),6);

                //Serial.print("\t");
                //Serial.print(m_imu->getTemperature_C(),6);                 
                DEBUG_MSG("\tHeading %0.2f %0.2f/%0.2f, Y: %0.2f/%0.2f, Z: %0.2f/%0.2f\n",compHeading,m_calibration.magMinX,m_calibration.magMaxX,m_calibration.magMinY,m_calibration.magMaxY,m_calibration.magMinZ,m_calibration.magMaxZ);
             }
#endif
        }
    }
}
