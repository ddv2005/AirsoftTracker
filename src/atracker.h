#pragma once
#include <Arduino.h>
#include <power.h>
#include <Wire.h>
#include "power.h"
#include "atglobal.h"
#include "concurrency/Thread.h"
#include "utils.h"
#include "gps/UBloxGPS.h"
#include "radio/LoraRadio.h"
#include "atcommon.h"
#include "atscreen.h"
#include "Adafruit_MCP23017.h"
#include "imu.h"
#include "atNetworkProcessor.h"
#include "comm_prot_parser.h"
#include "atble.h"

#define BA_NONE     0
#define BA_STATUS   1
#define BA_RADIUS   2
#define BA_BLE      3
#define BA_MENU     4
#define BA_MAP      5
#define BA_MAPLEFT  6
#define BA_MAPRIGHT 7
#define BA_MAPDOWN  8
#define BA_MAPUP    9
#define BA_MAPEXIT  10
#define BA_MAPRESET 11
#define BA_EXIT     12
#define BA_TIMER    13
#define BA_TIMERS   14
#define BA_TIMERS_RST   15
#define BA_AGD_UPDATE   16
#define BA_SLEEP        17

struct sCommand
{
    uint8_t id;
    uint8_t cmd;
    int64_t param1;
    int64_t param2;
    int64_t param3;
};

typedef cProtectedQueue<sCommand> cCommands;

struct sLoraPacket
{
    std::vector<byte> data;
    float SNR;
    float RSSI;
    int64_t time;
    int64_t preambleTime;
};

struct sLoraPacketToSend
{
    sNetworkPacket packet;
};


typedef cProtectedQueue<sLoraPacket> cLoraPackets;
typedef cProtectedQueue<sLoraPacketToSend> cLoraPacketsToSend;

class cVMotor
{
protected:
    int64_t m_stopTime;
    int8_t  m_pin;    
    int8_t  m_pin2;
    Adafruit_MCP23017 *m_expander;
    void setValue(bool b)
    {
        if(m_pin>=0)
            m_expander->digitalWrite(m_pin, b?HIGH:LOW);
        if(m_pin2>=0)
            m_expander->digitalWrite(m_pin2, b?HIGH:LOW);           
    }
public:    
    cVMotor(int8_t pin, int8_t pin2, Adafruit_MCP23017* expander):m_stopTime(0),m_pin(pin), m_pin2(pin2),m_expander(expander)
    {
    }

    void loop(int64_t now)
    {
        if(m_stopTime>0)
        {
            if(now>=m_stopTime)
            {
                m_stopTime = 0;
                setValue(false);
            }
        }
    }

    void loop()
    {
        loop(millis());
    }

    void vibrate(uint32_t duration)
    {
        m_stopTime = millis()+duration;
        setValue(true);
    }

    void stop()
    {
        m_stopTime = 0;
        setValue(false);
    }
};


#define AT_BUTTONS_COUNT 6
//Click type
#define ATCT_NORMAL 1
#define ATCT_LONG   2

#define AT_LONG_CLICK_DURATION 1000

#define ATSBS_MAIN  0
#define ATSBS_MAP   1
#define ATSBS_TIMERS   2

class cAirsoftTracker:public atGlobal, public cNetworkProcessorInterface
{
protected:
    typedef concurrency::ThreadFunctionTemplate<cAirsoftTracker> cThreadFunction;
    UBloxGPS m_gps;
    cLoraDevice *m_lora;
    cThreadFunction *m_loraThread;
    cLoraPackets m_loraPackets;
    cLoraPacketsToSend m_loraPacketsToSend;
    cATScreen *m_screen;
    cThreadFunction *m_core0Thread;
    bool m_core0ThreadReady;
    cCommands m_commands;
    cIMU *m_imu;
    Adafruit_MCP23017 *m_expander;
    cVMotor *m_vmotor;
    uint16_t m_gpioExpander = 0;
    int64_t m_timer1 = 0;
    int64_t m_timer2 = 0;
    uint16_t m_ps = 0;
    int64_t m_idleTimer = 0;
    int64_t m_screenTimer = 0;
    bool m_displaySleep = false;
    std::string m_bootMsg1;
    std::string m_bootMsg2;
    cNetworkProcessor m_networkProcessor;
    uint32_t m_gpsSeq = 0xFFFFFFFF;
    uint8_t m_serialSignatureState=0;
    atBLE m_bleClient;
    int64_t m_buttonsTime[AT_BUTTONS_COUNT];
    int8_t  m_buttonsIdx[AT_BUTTONS_COUNT];
    uint8_t m_sideButtonsState = ATSBS_MAIN;
    int64_t m_agdUpdateTimer = 0;
    uint16_t m_agdPOI=0;
    int64_t m_sideButtonsUpdateTime = 0;

    void scanI2Cdevice(TwoWire &wd)
    {
        byte err, addr;
        int nDevices = 0;
        for (addr = 1; addr < 127; addr++)
        {
            wd.beginTransmission(addr);
            err = wd.endTransmission();
            if (err == 0)
            {
                DEBUG_MSG("I2C device found at address 0x%x\n", addr);

                nDevices++;

                if (addr == SSD1306_ADDRESS)
                {
                    m_has_oled = true;
                    DEBUG_MSG("ssd1306 display found\n");
                }
#ifdef AXP192_SLAVE_ADDRESS
                if (addr == AXP192_SLAVE_ADDRESS)
                {
                    m_has_axp192 = true;
                    DEBUG_MSG("axp192 PMU found\n");
                }
#endif
            }
            else if (err == 4)
            {
                DEBUG_MSG("Unknow error at address 0x%x\n", addr);
            }
        }
        if (nDevices == 0)
            DEBUG_MSG("No I2C devices found\n");
        else
            DEBUG_MSG("done\n");
    }

    void loraThread(cThreadFunction *thread);
    void core0Thread(cThreadFunction *thread);
    void mainThread(cThreadFunction *thread);
    void processCommand(const sCommand &cmd);
    void processIMU();
    void processExpander();
    void updateBootScreen();
    void updateDistances(bool force=false);
    void updateSideButtons();
    void onButtonClick(uint8_t idx,uint8_t type);
    void programMode();
    void vibration();
    void startAGDUpdate();
    void stopAGDUpdate();
    bool isAGDUpdate()
    {
        return m_agdUpdateTimer!=0;
    }
    void processLoraPackets();
    void processLoraAGDPackets();
public:
    cAirsoftTracker();
    virtual ~cAirsoftTracker(){};
    void init();
    void mainInit();
    void mainLoop();
    virtual void sendPacket(const sNetworkPacket &packet);
    cIMU *getIMU() { return m_imu; }
    virtual void onScreenStateChange();
};
