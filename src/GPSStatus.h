#pragma once
#include <Arduino.h>
#include "Status.h"
#include "config.h"
#include "atcommon.h"
#include "AP_Declination/AP_Declination.h"

class GPSStatus : public Status
{

private:
    CallbackObserver<GPSStatus, const GPSStatus *> statusObserver = CallbackObserver<GPSStatus, const GPSStatus *>(this, &GPSStatus::updateStatus);

    bool hasLock = false;                // default to false, until we complete our first read
    bool isConnected = false;            // Do we have a GPS we are talking to
    int32_t latitude = 0, longitude = 0; // as an int mult by 1e-7 to get value as double
    int32_t altitude = 0;
    uint32_t hacc = 0;
    uint32_t dop = 0; // Diminution of position; PDOP where possible (UBlox), HDOP otherwise (TinyGPS) in 10^2 units (needs scaling before use)
    uint32_t heading = 0;
    uint32_t numSatellites = 0;
    uint32_t sequence = 0;
    double x = 0;
    double y = 0;
    float magDec = 0;

public:
    GPSStatus()
    {
        statusType = STATUS_TYPE_GPS;
    }
    GPSStatus(bool hasLock, bool isConnected, int32_t latitude, int32_t longitude, int32_t altitude, uint32_t dop, uint32_t hacc, uint32_t heading, uint32_t numSatellites) : Status()
    {
        this->hasLock = hasLock;
        this->isConnected = isConnected;
        this->latitude = latitude;
        this->longitude = longitude;
        this->altitude = altitude;
        this->dop = dop;
        this->heading = heading;
        this->numSatellites = numSatellites;
        this->hacc = hacc;
        if(hasLock)
        {
            magDec = AP_Declination::get_declination(latitude*1e-7,longitude*1e-7);
            //DEBUG_MSG("D %f\n",magDec);
        }
        this->updateXY();
    }

    GPSStatus(const GPSStatus &);
    GPSStatus &operator=(const GPSStatus &);

    float getMagDec()
    {
        return magDec;
    }
    void updateXY()
    {
        convertDegreeToMeter(longitude*1e-07,latitude*1e-07,x,y);
    }

    void observe(Observable<const GPSStatus *> *source)
    {
        statusObserver.observe(source);
    }

    bool getHasLock() const
    {
        return hasLock;
    }

    bool getIsConnected() const
    {
        return isConnected;
    }

    int32_t getLatitude() const
    {
        return latitude;
    }

    int32_t getLongitude() const
    {
        return longitude;
    }

    int32_t getAltitude() const
    {
        return altitude;
    }

    uint32_t getDOP() const
    {
        return dop;
    }

    uint32_t getHAcc() const
    {
        return hacc;
    }

    uint32_t getHeading() const
    {
        return heading;
    }

    uint32_t getNumSatellites() const
    {
        return numSatellites;
    }

    uint32_t getSequence() const
    {
        return sequence;
    }

    double getX() const
    {
        return x;
    }

    double getY() const
    {
        return y;
    }

    bool matches(const GPSStatus *newStatus) const
    {
        return (
            newStatus->hasLock != hasLock ||
            newStatus->isConnected != isConnected ||
            newStatus->latitude != latitude ||
            newStatus->longitude != longitude ||
            newStatus->altitude != altitude ||
            newStatus->dop != dop ||
            newStatus->hacc != hacc ||
            newStatus->heading != heading ||
            newStatus->numSatellites != numSatellites);
    }
    int updateStatus(const GPSStatus *newStatus)
    {
        // Only update the status if values have actually changed
        bool isDirty;
        {
            isDirty = matches(newStatus);
            initialized = true;
            hasLock = newStatus->hasLock;
            isConnected = newStatus->isConnected;
            latitude = newStatus->latitude;
            longitude = newStatus->longitude;
            altitude = newStatus->altitude;
            dop = newStatus->dop;
            hacc = newStatus->hacc;
            heading = newStatus->heading;
            numSatellites = newStatus->numSatellites;
        }
        if (isDirty)
        {
            //DEBUG_MSG("New GPS pos lat=%f, lon=%f, alt=%d, pdop=%f, heading=%f, sats=%d\n", latitude * 1e-7, longitude * 1e-7, altitude, dop * 1e-2, heading * 1e-5, numSatellites);
            updateXY();
            sequence++;
            onNewStatus.notifyObservers(this);
        }
        return 0;
    }
};
