#include "UBloxGPS.h"
#include <assert.h>
#include "global.h"

#define GPS_DEBUG
#define GPS_DISPLAY_HW 

UBloxGPS::UBloxGPS()
{
}

bool UBloxGPS::internalSetup()
{
    bool ok;
    ok = ublox.setUART1Output(COM_TYPE_UBX, 3000); // Use native API
    if(!ok) return false;
    ok = ublox.setNavigationFrequency(GPS_FREQ+1, 3000); // Produce 4x/sec to keep the amount of time we stall in getPVT low
    if(!ok) return false;
    ok = ublox.setDynamicModel(DYN_MODEL_PEDESTRIAN,3000); // probably PEDESTRIAN but just in case assume bike speeds
    if(!ok) return false;
    //ok = ublox.powerSaveMode(false, 2000); // use power save mode, the default timeout (1100ms seems a bit too tight)
    //if(!ok) return false;

    uint8_t payloadGNSS[] = {0x00,0x00,0x20,0x07,
            0x00,0x08,0x10,0x00,0x01,0x00,0x01,0x01,
            0x01,0x01,0x03,0x00,0x01,0x00,0x01,0x01,
            0x02,0x04,0x08,0x00,0x01,0x00,0x01,0x01,
            0x03,0x08,0x10,0x00,0x00,0x00,0x01,0x01,
            0x04,0x00,0x08,0x00,0x00,0x00,0x01,0x01,
            0x05,0x00,0x03,0x00,0x01,0x00,0x01,0x01,
            0x06,0x08,0x0E,0x00,0x01,0x00,0x01,0x01};
    ubxPacket packetCfg = {0, 0, 0, 0, 0, payloadGNSS, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED,SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
    packetCfg.cls = UBX_CLASS_CFG;
    packetCfg.id = UBX_CFG_GNSS;
    packetCfg.len = 0x3C;
    packetCfg.startingSpot = 0;
    ublox.sendCommand(&packetCfg,0);
    delay(50);

    uint8_t payloadNMEA[] = {0x00,0x41,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    packetCfg.payload = payloadNMEA;
    packetCfg.cls = UBX_CLASS_CFG;
    packetCfg.id = UBX_CFG_NMEA;
    packetCfg.len = 0x14;
    packetCfg.startingSpot = 0;
    ublox.sendCommand(&packetCfg,0);
    delay(50);

#ifdef HAS_AXP20X
    ublox.setSerialRate(GPS_BAUDRATE2,1,3000);
    delay(2);
    _serial_gps.updateBaudRate(GPS_BAUDRATE2);
    delay(50);

    restoreDatabase(GNSS_DBD_FILENAME);
#else
    ublox.setSerialRate(GPS_BAUDRATE);
    _serial_gps.setRxBufferSize(800);    
#endif    
    //ok = ublox.saveConfiguration(3000);
    //assert(ok);
    return true;
}

bool UBloxGPS::setup()
{
    if(serialInitialized)
    {
        _serial_gps.end();
        delay(10);
        serialInitialized = false;
    }
    if(!serialInitialized)
    {
        _serial_gps.setRxBufferSize(800);
#ifdef GPS_RX_PIN
        _serial_gps.begin(GPS_BAUDRATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#else
        _serial_gps.begin(GPS_BAUDRATE);
#endif
        serialInitialized = true;
        delay(30);
    }
    //ublox.enableDebugging(Serial);

    // note: the lib's implementation has the wrong docs for what the return val is
    // it is not a bool, it returns zero for success
    isConnected = ublox.begin(_serial_gps);

    // try a second time, the ublox lib serial parsing is buggy?
    if (!isConnected)
    {
        isConnected = ublox.begin(_serial_gps);
    }

    if (isConnected)
    {
        DEBUG_MSG("Connected to UBLOX GPS successfully\n");

        if(!internalSetup())
        {
            DEBUG_MSG("UBLOX Init error\n");
            return false;
        }
        concurrency::PeriodicTask::setup(); // We don't start our periodic task unless we actually found the device

        return true;
    }
    else
    {
        return false;
    }
}

void UBloxGPS::reset()
{
    uint8_t payloadCfg[4];
    ubxPacket packetCfg = {0, 0, 0, 0, 0, payloadCfg, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED,SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
    packetCfg.cls = UBX_CLASS_CFG;
    packetCfg.id = UBX_CFG_RST;
    packetCfg.len = 4;
    packetCfg.startingSpot = 0;
    payloadCfg[0] = 0x1;
    payloadCfg[1] = 0x0;
    payloadCfg[2] = 0;               // 0=HW reset
    payloadCfg[3] = 0;               // reserved
    ublox.sendCommand(&packetCfg, 0); // don't expect ACK
}

void UBloxGPS::loop()
{
    ublox.checkUblox();
    if(lastPvtSeqId!=ublox.pvtSeqId)
    {
        processPVT();
    }
}

void UBloxGPS::processPVT()
{
    uint8_t fixtype;

    missPVT = 0;
    lastPvtSeqId=ublox.pvtSeqId;
    fixtype = ublox.getFixType(0);
#ifdef GPS_DEBUG
    DEBUG_MSG("GPS fix type %d\n", fixtype);
#endif    

    //DEBUG_MSG("lat %d\n", ublox.getLatitude());

    // any fix that has time
    if (ublox.moduleQueried.gpsSecond)
    {
        /* Convert to unix time
The Unix epoch (or Unix time or POSIX time or Unix timestamp) is the number of seconds that have elapsed since January 1, 1970
(midnight UTC/GMT), not counting leap seconds (in ISO 8601: 1970-01-01T00:00:00Z).
*/
        struct tm t;
        t.tm_sec = ublox.getSecond(0);
        t.tm_min = ublox.getMinute(0);
        t.tm_hour = ublox.getHour(0);
        t.tm_mday = ublox.getDay(0);
        t.tm_mon = ublox.getMonth(0) - 1;
        t.tm_year = ublox.getYear(0) - 1900;
        t.tm_isdst = false;
        perhapsSetRTC(t,ublox.getNanosecond(0)/1000000);
    }

    if(ublox.moduleQueried.horizontalAccEst)
        hacc = ublox.getHorizontalAccEst(0);

    if(ublox.moduleQueried.pDOP)
        dop = ublox.getPDOP(0);

    if(ublox.moduleQueried.SIV)
        numSatellites = ublox.getSIV(0);

#ifdef GPS_DEBUG
    DEBUG_MSG("GPS numSatellites %d\n", numSatellites);
#endif    

#ifdef GPS_DISPLAY_HW
    ubxHwState hwState;
    if(ublox.getHwState(hwState))
    {
        DEBUG_MSG("HW State Noise %d, AGC %0.2f (%d)\n", hwState.noisePerMS,hwState.agcCnt*100/8192.0,hwState.agcCnt);
    }
#endif    
    if(global->getConfig().testLat&&global->getConfig().testLng)
    {
        latitude = global->getConfig().testLat;
        longitude = global->getConfig().testLng;
        altitude = 0;
        heading = ublox.getHeading(0);

        // bogus lat lon is reported as 0 or 0 (can be bogus just for one)
        // Also: apparently when the GPS is initially reporting lock it can output a bogus latitude > 90 deg!
        hasValidLocation = (latitude != 0) && (longitude != 0) && (latitude <= 900000000 && latitude >= -900000000);
        if (hasValidLocation)
        {
            wantNewLocation = false;
            notifyObservers(NULL);
        }
    }
    else
    if ((fixtype >= 3 && fixtype <= 4) && ublox.moduleQueried.latitude) // rd fixes only
    {
        // we only notify if position has changed
        latitude = ublox.getLatitude(0);
        longitude = ublox.getLongitude(0);
        //longitude = -74.6294672*10000000; //OTP GPS
        //latitude = 39.9381124*10000000;
        //longitude = -74.41993781254924*10000000; //SANJ GPS
        //latitude = 39.99472549579622*10000000;
        //longitude = -73.98029099999999*10000000; //MSATO GPS
        //latitude = 42.207361*10000000;
        //longitude = -74.55166537552641*10000000; //R14 GPS
        //latitude = 39.9980604701216*10000000;

        //longitude = -76.19249075700209*10000000; //BR2022 GPS
        //latitude = 40.94618445451866*10000000;
        //longitude = -74.9177937*10000000; //Picasso GPS
        //latitude = 39.7361251*10000000;        

        altitude = ublox.getAltitude(0) / 1000; // in mm convert to meters
        heading = ublox.getHeading(0);

        // bogus lat lon is reported as 0 or 0 (can be bogus just for one)
        // Also: apparently when the GPS is initially reporting lock it can output a bogus latitude > 90 deg!
        hasValidLocation = (latitude != 0) && (longitude != 0) && (latitude <= 900000000 && latitude >= -900000000);
        if (hasValidLocation)
        {
            wantNewLocation = false;
            notifyObservers(NULL);
        }
    }
    else // we didn't get a location update, go back to sleep and hope the characters show up
        wantNewLocation = true;

    // Notify any status instances that are observing us
    const GPSStatus status = GPSStatus(hasLock(), isConnected, latitude, longitude, altitude, dop, hacc, heading, numSatellites);
    newStatus.notifyObservers(&status);
    ublox.flushPVT();
    waitPVT = false;
}

void UBloxGPS::doTask()
{
    loop();
    if(waitPVT)
    {
        missPVT++;
        if(missPVT>4)
        {
            missPVT = 0;
        }
    }
    waitPVT = true;
    ublox.getPVT(0);
#ifdef GPS_DISPLAY_HW
    ublox.queryHwState(0);
#endif    

    setPeriod(hasValidLocation && !wantNewLocation ? 1000/GPS_FREQ : 1000);
}

bool UBloxGPS::canSleep()
{
#ifdef AT_CAN_SLEEP    
    return !waitPVT;
#else
    return false;
#endif
}

void UBloxGPS::startLock()
{
    DEBUG_MSG("Looking for GPS lock\n");
    wantNewLocation = true;
    setPeriod(1);
}

void UBloxGPS::backupDatabase(const char *filename)
{
    uint8_t *data = (uint8_t *) malloc(8192);
    int16_t size=8192;
    ublox.pollNavigationDatabase(data,size,5000);
    if(size>0)
    {
        FILE *file;
        file = fopen(filename,"wb+");
        fwrite(data,size,1,file);
        fflush(file);
        fclose(file);
    }
    free(data);
}

void UBloxGPS::restoreDatabase(const char *filename)
{
    FILE *file;
    file = fopen(filename,"rb");
    if(file==NULL)
        return;
    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    if(size>0)
    {
        int ss = 64;
        uint8_t *data = (uint8_t *) malloc(ss);
        while(true)
        {
            int readed = fread(data,ss,1,file);
            if(readed!=0)
                ublox.sendNavigationDatabase(data,readed);
            if(readed!=ss)
                break;
            delay(1);
        }
        free(data);
    }
    fclose(file);    
}

void UBloxGPS::shutdown()
{
    //ublox.enableDebugging(Serial);
    uint8_t *payloadCfg = ublox.payloadCfg;
    ubxPacket packetCfg = {0, 0, 0, 0, 0, payloadCfg, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED,SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
    packetCfg.cls = UBX_CLASS_CFG;
    packetCfg.id = UBX_CFG_RST;
    packetCfg.len = 4;
    packetCfg.startingSpot = 0;
    payloadCfg[0] = 0x0;
    payloadCfg[1] = 0x0;
    payloadCfg[2] = 0x8;               // 0 = Controlled GNSS stop
    payloadCfg[3] = 0;               // reserved
    ublox.sendCommand(&packetCfg,0); // don't expect ACK
}