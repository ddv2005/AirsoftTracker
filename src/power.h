#pragma once
#include "concurrency/PeriodicTask.h"
#include "PowerStatus.h"

#define MIN_BAT_MILLIVOLTS 3250 // millivolts. 10% per https://blog.ampow.com/lipo-voltage-chart/

#define BAT_MILLIVOLTS_FULL 4100
#define BAT_MILLIVOLTS_EMPTY 3500

#ifdef HAS_AXP20X
#include "axp20x.h"
#endif

class Power : public concurrency::PeriodicTask
{
#ifdef HAS_AXP20X
protected:
    AXP20X_Class axp;
#endif    
public:
    Observable<const PowerStatus *> newStatus;

    void readPowerStatus();
    int loop();
    virtual bool setup();
    virtual void doTask();
    void setStatusHandler(PowerStatus *handler)
    {
        statusHandler = handler;
    }

    void gpsOff();
    void gpsOn();
    void shutdown();
#ifdef HAS_AXP20X
    AXP20X_Class &getAXP()
    {
        return axp;
    }
#endif
protected:
    PowerStatus *statusHandler;
#ifdef HAS_AXP20X    
    virtual void axp192Init();
#endif    
};
