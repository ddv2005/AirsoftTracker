#pragma once

#include "GPS.h"
#include "Observer.h"
#include "../concurrency/PeriodicTask.h"
#include "SparkFun_Ublox_Arduino_Library.h"

/**
 * A gps class that only reads from the GPS periodically (and FIXME - eventually keeps the gps powered down except when reading)
 *
 * When new data is available it will notify observers.
 */
class UBloxGPS : public GPS, public concurrency::PeriodicTask
{
    SFE_UBLOX_GPS ublox;
    uint32_t lastPvtSeqId=0xFFFFFFFF;
    bool wantNewLocation = true;
    bool internalSetup();
    bool serialInitialized = false;
    bool waitPVT=false;
    uint8_t missPVT = 0;
    void processPVT();
  public:
    UBloxGPS();

    /**
     * Returns true if we succeeded
     */
    virtual bool setup();

    virtual void doTask();

    /**
     * Restart our lock attempt - try to get and broadcast a GPS reading ASAP
     * called after the CPU wakes from light-sleep state */
    virtual void startLock();
    virtual void reset();
    virtual void backupDatabase(const char *filename);
    virtual void restoreDatabase(const char *filename);
    virtual bool canSleep();
    virtual void loop();
    virtual void shutdown();
};
