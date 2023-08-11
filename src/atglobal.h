#pragma once
#include <Arduino.h>
#include <power.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include "power.h"
#include "GPSStatus.h"
#include "IMUStatus.h"
#include <ArduinoJson.h>
#include "radio/LoraRadio.h"
#include "mapTiles.h"
#include "atstrings.h"
#include <vector>

#define MAX_CONFIG_ID   0xFFFF
#define U_METRIC    0
#define U_US        1

#define POIT_PEER       0
#define POIT_STATIC     1
#define POIT_DYNAMIC    2
#define POIT_ENEMY      3
#define POIT_SELF       10

#define POIS_NORMAL 0
#define POIS_DEAD   1

struct sPOI
{
    bool positionChanged=false;
    uint8_t type=POIT_PEER;
    char symbol=0;
    std::string name;
    uint8_t status=0;
    uint8_t options=0;
    double lat=0;
    double lng=0;
    uint32_t hacc=0;
    double x=0;
    double y=0;
    int32_t alt=0;
    int16_t heading=0;
    double distance=0;
    double realDistance=0;
    double courseTo=0;
    double RSSI=0;
    double SNR=0;
    int32_t retransmitRating = 0;
    int64_t lastUpdate=0;
    uint8_t lastLabelIdx=0;
    uint32_t color=0;

    sPOI()
    {
    }

    bool isValidLocation() const
    {
        return (lat != 0) && (lng != 0) && (lat <= 90 && lat >= -90);
    }

    void updateXY()
    {
        if(isValidLocation())
        {
            convertDegreeToMeter(lng,lat,x,y);
        }
        else
        {
            x = y = 0;
        }
    }
    bool loadFromJson(const JsonVariant &json);
};

struct sSideButton
{
    uint8_t     idx=0;
    std::string caption;
    uint8_t     action=0;
    uint8_t     actionLong=0;
};

class cSideButtons
{
protected:
    sSideButton m_buttons[SIDE_BUTTONS_COUNT];

public:
    cSideButtons();
    sSideButton *getButtons()
    {
        return m_buttons;
    }

    uint8_t getButtonsCount()
    {
        return SIDE_BUTTONS_COUNT;
    }
};

#define AT_MAX_TIMERS 4
class cConfig
{
protected:
    virtual void initDefaults()=0;
    virtual bool checkConfig()=0;
    virtual void saveConfigValues(JsonDocument &json)=0;
    virtual void loadConfigValues(JsonDocument &json)=0;
public:
    cConfig();
    virtual bool loadConfig(std::istream &stream,bool binary=false);
    virtual void saveConfig(std::ostream &stream,bool binary=false);

    virtual void loadConfig(const char * filename);
    virtual void saveConfig(const char * filename);
};

class atMainConfig:public cConfig
{
public:
    uint16_t    id;
    std::string name;
    char        symbol;
    uint16_t    radius;
    uint8_t     units;
    uint16_t    reportInterval;
    std::string timeZone;
    uint32_t    screenIdleTimeout;
    bool        useVibration;
    uint32_t    vibrationTime;
    uint32_t    maxPeerTimeout;
    uint32_t    timers[AT_MAX_TIMERS];
    int32_t     testLat;
    int32_t     testLng;
protected:
    virtual void initDefaults();
    virtual bool checkConfig();
    virtual void saveConfigValues(JsonDocument &json);
    virtual void loadConfigValues(JsonDocument &json);
public:
    atMainConfig();
};

class atNetworkConfig:public cConfig
{
public:
    cLoraChannelCondfig channelConfig;
    uint8_t networkId;
    std::string password;
    int32_t requestInfoInterval;
protected:
    virtual void initDefaults();
    virtual bool checkConfig();
    virtual void saveConfigValues(JsonDocument &json);
    virtual void loadConfigValues(JsonDocument &json);
public:
    atNetworkConfig();
};

class atAGDNetworkConfig:public cConfig
{
public:
    cLoraChannelCondfig channelConfig;
    uint32_t updateTimeout;
protected:
    virtual void initDefaults();
    virtual bool checkConfig();
    virtual void saveConfigValues(JsonDocument &json);
    virtual void loadConfigValues(JsonDocument &json);
public:
    atAGDNetworkConfig();
};
//Player Status
#define PS_NORMAL   0
#define PS_DEAD     1

struct sPlayerStatus
{
    uint8_t status;
    uint8_t options;

    sPlayerStatus()
    {
        status = PS_NORMAL;
        options = 0;
    }
};

typedef std::map<uint16_t,sPOI> cPOIMap;
typedef std::list<sPOI> cPOIList;
class cTimersChain;
class cTimer
{
friend cTimersChain;    
protected:
    int32_t m_duration;
    int64_t m_startTime;
    int64_t m_pauseTime;
    std::string m_name;
public:    
    cTimer():m_duration(0),m_startTime(0),m_pauseTime(0)
    {
    }
    ~cTimer(){}
    int32_t getDuration() { return m_duration; }
    void setDuration(uint32_t value) { m_duration = value; }
    std::string getName() { return m_name; }
    void setName(const char *name) { m_name = name; }
    bool isEnabled() { return m_duration>0; }
    bool isStarted() { return m_startTime>0; }
    bool isPaused() { return isStarted() && m_pauseTime>0; }
    void start(int64_t now) { m_startTime = now; m_pauseTime = 0; }
    void reset() { m_startTime = m_pauseTime = 0; }
    void pause(int64_t now) { if(isStarted() && m_pauseTime==0) m_pauseTime = now; }
    void resume(int64_t now) { 
        if(isPaused())
        {
            m_startTime = now-(m_pauseTime-m_startTime);
            m_pauseTime = 0;
        }
    }
    int64_t getCurrentDuration(int64_t now)
    {
        if(isStarted())
        {
            int64_t duration;
            if(m_pauseTime>0)
                duration = m_pauseTime-m_startTime;
            else
                duration = now-m_startTime;
            if(duration>=m_duration)
                return m_duration;
            else
                return duration;
        }
        else
            return 0;
    }

    bool isFinished(int64_t now) {
        if(isStarted())
            return getCurrentDuration(now)>=m_duration;
        else
            return false;
    }
};

typedef std::vector<cTimer*> cTimersVector;
typedef std::map<uint8_t,cTimer*> cTimersMap;

class cTimersChain
{
protected:
    cTimersVector m_timers;
    uint8_t m_currentIdx;
    std::string m_name;

    void processTimers(int64_t now)
    {
        while(m_currentIdx<m_timers.size())
        {
            if(isStarted()&&!isPaused())
            {
                if(m_timers[m_currentIdx]->isFinished(now))
                {
                    int64_t nowNew = m_timers[m_currentIdx]->m_startTime+m_timers[m_currentIdx]->m_duration;
                    m_currentIdx++;
                    if(m_currentIdx<m_timers.size())
                    {
                        m_timers[m_currentIdx]->start(nowNew);
                    }
                }
                else
                    break;
            }
            else
                break;
        }
    }
public:
    cTimersChain():m_currentIdx(0){};
    ~cTimersChain(){}
    std::string getName() { return m_name; }
    void setName(const char *name) { m_name = name; }
    cTimersVector &getTimers() { return m_timers; }
    void addTimer(cTimer* timer) {
        m_timers.push_back(timer);
    }

    bool isEnabled()
    {
        for(auto itr=m_timers.begin(); itr!=m_timers.end(); itr++)
            if(!(*itr)->isEnabled())
                return false;
        return m_timers.size();
    }

    bool isStarted()
    {
        if(m_currentIdx<m_timers.size())
        {
            return m_timers[m_currentIdx]->isStarted();
        }
        return true;
    }

    bool isPaused()
    {
        if(m_currentIdx<m_timers.size())
        {
            return m_timers[m_currentIdx]->isPaused();
        }
        return false;
    }

    void reset()
    {
        m_currentIdx = 0;
        for(auto itr=m_timers.begin(); itr!=m_timers.end(); itr++)
            (*itr)->reset();
    }

    void start(int64_t now)
    {
        reset();
        if(m_timers.size())
            m_timers.front()->start(now);
    }

    void pause(int64_t now)
    {
        processTimers(now);
        if(m_currentIdx<m_timers.size())
        {
            m_timers[m_currentIdx]->pause(now);
        }
    }

    void resume(int64_t now)
    {
        if(m_currentIdx<m_timers.size())
        {
            m_timers[m_currentIdx]->resume(now);
        }
    }

    bool isFinished(int64_t now)
    {
        processTimers(now);
        return m_currentIdx>=m_timers.size();
    }

    void loop(int64_t now)
    {
        processTimers(now);
    }

    uint8_t getCurrentIdx() { return m_currentIdx; }
};

typedef std::list<cTimersChain*> cTimersChains;

class atGlobal:public cMapProcessorCallbacks
{
protected:
    atMainConfig m_config;
    atNetworkConfig m_networkConfig;
    atAGDNetworkConfig m_agdNetworkConfig;
    Power *m_power;

    virtual void initPower()
    {
        m_mapCenterX = m_mapCenterY = 0;
        m_power = new Power();
        m_power->setup();
        m_power->setStatusHandler(m_powerStatus);
        m_powerStatus->observe(&m_power->newStatus);
    }
    uint16_t m_lastPOIID;
public:
    bool m_has_axp192;
    bool m_has_oled;
    PowerStatus *m_powerStatus;
    GPSStatus *m_gpsStatus;
    IMUStatus *m_imuStatus;
    sPlayerStatus m_playerStatus;
    cSideButtons m_sideButtons; 
    cMaps m_maps;
    cMapProcessor m_mapsProcessor;
    cPOIMap m_poiMap;
    cPOIList m_poiMapStatic;
    double m_mapCenterX,m_mapCenterY;
    cTimersMap m_timers;
    cTimersChains m_timersChains;

    atGlobal():m_mapsProcessor(m_maps.getMaps(),this)
    {
        m_lastPOIID = 0;
        m_has_axp192 = m_has_oled = false;
        m_powerStatus = new PowerStatus();
        m_power = NULL;
        m_gpsStatus = new GPSStatus();
        m_imuStatus = new IMUStatus();
        m_config.loadConfig(MAIN_CONFIG_FILENAME);
        m_networkConfig.loadConfig(NET_CONFIG_FILENAME);
        m_agdNetworkConfig.loadConfig(AGD_NET_CONFIG_FILENAME);
        for(int i=0; i<AT_MAX_TIMERS; i++)
        {
            char buf[8];
            m_timers[i] = new cTimer();
            m_timers[i]->setDuration(m_config.timers[i]);
            snprintf(buf,sizeof(buf),"T%d",i+1);
            m_timers[i]->setName(buf);
        }
        cTimersChain *tc = new cTimersChain();
        tc->setName("T1->T2");
        tc->addTimer(m_timers[0]);
        tc->addTimer(m_timers[1]);
        m_timersChains.push_back(tc);

#ifndef AD_NOSCREEN        
        m_maps.loadFromBinary(SCREEN_FLASH_META_BINARY_FILE);
#endif        
    }

    virtual ~atGlobal()
    {
        delete m_imuStatus;
        delete m_gpsStatus;
        delete m_powerStatus;
        if(m_power)
            delete m_power;
    }

    virtual void onMapLoad(cMap *map);

    void updateTimers()
    {
        for(int i=0; i<AT_MAX_TIMERS; i++)
        {
            m_timers[i]->setDuration(m_config.timers[i]);
        }
    }

    float getDirection()
    {
        return (18000-m_imuStatus->getHeading()+m_gpsStatus->getMagDec()*100)*PI/18000.0;
    }

    float getDirectionDeg()
    {
        float result = m_imuStatus->getHeading()/100.0+m_gpsStatus->getMagDec();
        if(result<0)
            result += 360;
        if(result>=360)
            result -= 360;
        return result;
    }

    uint16_t createPOI()
    {
        return ++m_lastPOIID;
    }

    void setPOI(uint16_t id,const sPOI &poi)
    {
        m_poiMap[id] = poi;
    }

    void removePOI(uint16_t id)
    {
        m_poiMap.erase(id);
    }

    virtual void init()
    {
        initPower();
    }

    virtual void loop()
    {
        concurrency::periodicScheduler.loop();
    }

    Power *getPower() {
        return m_power;
    }

    atMainConfig  &getConfig()
    {
        return m_config;
    }

    atNetworkConfig &getNetworkConfig()
    {
        return m_networkConfig;
    }

    virtual void onScreenStateChange(){};

    void processTimers(int64_t now)
    {
        for(auto itr=m_timersChains.begin(); itr!= m_timersChains.end(); itr++)
            (*itr)->loop(now);
    }
};
