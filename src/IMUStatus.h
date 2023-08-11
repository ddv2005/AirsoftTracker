#pragma once
#include <Arduino.h>
#include "Status.h"
#include "config.h"

class IMUStatus : public Status
{

private:
    CallbackObserver<IMUStatus, const IMUStatus *> statusObserver = CallbackObserver<IMUStatus, const IMUStatus *>(this, &IMUStatus::updateStatus);

    bool isConnected = false;            // Do we have a GPS we are talking to
    int32_t heading;

public:
    IMUStatus()
    {
        statusType = STATUS_TYPE_GPS;
    }
    IMUStatus(bool isConnected,int32_t heading) : Status()
    {
        this->isConnected = isConnected;
        this->heading = heading;
    }
    IMUStatus(const IMUStatus &);
    IMUStatus &operator=(const IMUStatus &);

    void observe(Observable<const IMUStatus *> *source)
    {
        statusObserver.observe(source);
    }

    bool getIsConnected() const
    {
        return isConnected;
    }

    int32_t getHeading() const
    {
        return heading;
    }

    bool matches(const IMUStatus *newStatus) const
    {
        return (
            newStatus->isConnected != isConnected ||
            newStatus->heading != heading);
    }
    int updateStatus(const IMUStatus *newStatus)
    {
        // Only update the status if values have actually changed
        bool isDirty;
        {
            isDirty = matches(newStatus);
            initialized = true;
            isConnected = newStatus->isConnected;
            heading = newStatus->heading;
        }
        if (isDirty)
        {
            onNewStatus.notifyObservers(this);
        }
        return 0;
    }
};
