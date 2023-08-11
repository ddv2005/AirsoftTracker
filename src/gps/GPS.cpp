
#include "GPS.h"
#include "config.h"
#include "timing.h"
#include <assert.h>
#include <time.h>

#ifdef GPS_RX_PIN
HardwareSerial _serial_gps_real(GPS_SERIAL_NUM);
HardwareSerial &GPS::_serial_gps = _serial_gps_real;
#else
// Assume NRF52
HardwareSerial &GPS::_serial_gps = Serial1;
#endif

bool timeSetFromGPS; // We try to set our time from GPS each time we wake from sleep

GPS *gps;

// stuff that really should be in in the instance instead...
static uint32_t
    timeStartMsec; // Once we have a GPS lock, this is where we hold the initial msec clock that corresponds to that time
static uint64_t zeroOffsetSecs; // GPS based time in secs since 1970 - only updated once on initial lock

void readFromRTC()
{
    struct timeval tv; /* btw settimeofday() is helpfull here too*/

    if (!gettimeofday(&tv, NULL)) {
        uint32_t now = timing::millis();

        DEBUG_MSG("Read RTC time as %ld (cur millis %u) valid=%d\n", tv.tv_sec, now, timeSetFromGPS);
        timeStartMsec = now;
        zeroOffsetSecs = tv.tv_sec;
    }
}

/// If we haven't yet set our RTC this boot, set it from a GPS derived time
void perhapsSetRTC(const struct timeval *tv)
{
    if (!timeSetFromGPS) {
        timeSetFromGPS = true;
        DEBUG_MSG("Setting RTC %ld secs\n", tv->tv_sec);
#ifndef NO_ESP32
        settimeofday(tv, NULL);
#else
        DEBUG_MSG("ERROR TIME SETTING NOT IMPLEMENTED!\n");
#endif
        readFromRTC();
    }
}

time_t mktimegm(struct tm *tm)
{
    time_t t;

    int y = tm->tm_year + 1900, m = tm->tm_mon + 1, d = tm->tm_mday;

    if (m < 3) {
        m += 12;
        y--;
    }

    t = 86400 *
        (d + (153 * m - 457) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 719469);

    t += 3600 * tm->tm_hour + 60 * tm->tm_min + tm->tm_sec;

    return t;
}
void perhapsSetRTC(struct tm &t, uint32_t ms)
{
    /* Convert to unix time
    The Unix epoch (or Unix time or POSIX time or Unix timestamp) is the number of seconds that have elapsed since January 1, 1970
    (midnight UTC/GMT), not counting leap seconds (in ISO 8601: 1970-01-01T00:00:00Z).
    */
    time_t res = mktimegm(&t);
    struct timeval tv;
    tv.tv_sec = res;
    if(ms>0)
        tv.tv_usec = ms*1000;
    else
        tv.tv_usec = 0;

    //DEBUG_MSG("Got time from GPS month=%d, year=%d, unixtime=%ld, ms=%d\n", t.tm_mon, t.tm_year, tv.tv_sec,ms);
    if (t.tm_year < 120 || t.tm_year >= 300)
        DEBUG_MSG("Ignoring invalid GPS time\n");
    else
        perhapsSetRTC(&tv);
}

uint32_t getTime()
{
    return ((timing::millis() - timeStartMsec) / 1000) + zeroOffsetSecs;
}

uint32_t getValidTime()
{
    return timeSetFromGPS ? getTime() : 0;
}
