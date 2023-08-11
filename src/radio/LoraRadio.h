#pragma once

#include <stdint.h>
#include <RadioLib.h>

struct sRFModuleConfig
{
    RADIOLIB_PIN_TYPE cs;
    RADIOLIB_PIN_TYPE irq;
    RADIOLIB_PIN_TYPE rst;
    RADIOLIB_PIN_TYPE busy;
    SPIClass *spi;
    SPISettings *spiSettings;
};


#define LR_SYNC 0x31
class cLoraChannelCondfig
{
public:    
    float   freq;
    float   bw;
    uint8_t sf;
    uint8_t cr;
    uint16_t preambleLength;
    uint8_t syncWord;
    int8_t power;
    uint8_t currentLimit;

    cLoraChannelCondfig()
    {
        freq = 915.0;
        bw = 125.0;
        sf = 9;
        cr = 7;
        preambleLength = 8;
        syncWord = LR_SYNC;
        power = 17;
        currentLimit = 140;
    }
};

class cLoraDevice
{
protected:
    PhysicalLayer *m_intf;
    cLoraChannelCondfig m_config;
public:
    cLoraDevice(cLoraChannelCondfig &channelConfig):m_intf(NULL),m_config(channelConfig)
    {

    }

    virtual ~cLoraDevice(){};

    virtual int16_t applyConfig()=0;
    virtual void applyConfig(const cLoraChannelCondfig &channelConfig)=0;
    virtual int16_t transmit(uint8_t* data, size_t len)
    {
        return m_intf->transmit(data,len);
    }

    virtual int16_t receive(uint8_t* data, size_t len)
    {
        return m_intf->receive(data, len);
    }

    virtual size_t getPacketLength(bool update = true)
    {
        return m_intf->getPacketLength(update);
    }

    virtual float getRSSI()=0;
    virtual float getSNR()=0;

    virtual int16_t startReceive(uint8_t len = 0) = 0;
    virtual void setDio0Action(void (*func)(void)) = 0;
    virtual void clearDio0Action() = 0;
    virtual int16_t readData(uint8_t* data, size_t len)
    {
        return m_intf->readData(data,len);
    }

    virtual void reset() = 0;
    virtual uint32_t getIrqStatus() { return 0; }
    virtual void clearIrqStatus(uint16_t s) {}
    virtual void clearPreambleIrq() {}
    virtual bool isPreambleIrq(uint16_t status) { return false; }
    virtual bool isRxDoneIrq(uint16_t status) { return true; }
    PhysicalLayer *getIntf()
    {
        return m_intf;
    }
};