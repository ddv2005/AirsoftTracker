#pragma once
#include "LoraRadio.h"
#include "RadioLibRF95.h"


class cLoraDeviceRF95:public cLoraDevice
{
protected:
    Module m_module;
    RadioLibRF95 m_lora;
public:
    cLoraDeviceRF95(sRFModuleConfig &moduleConfig,cLoraChannelCondfig &channelConfig):cLoraDevice(channelConfig),
        m_module(moduleConfig.cs, moduleConfig.irq, moduleConfig.rst, moduleConfig.busy, *moduleConfig.spi, *moduleConfig.spiSettings),
        m_lora(&m_module)
    {
        m_intf = &m_lora;
    }

    virtual int16_t applyConfig()
    {
        return m_lora.begin(m_config.freq, m_config.bw, m_config.sf, m_config.cr, m_config.syncWord, m_config.power, m_config.currentLimit, m_config.preambleLength, 0);
    }

    virtual void applyConfig(const cLoraChannelCondfig &channelConfig)
    {
        m_lora.setFrequency(channelConfig.freq);
        m_lora.setBandwidth(channelConfig.bw);
        m_lora.setSpreadingFactor(channelConfig.sf);
        m_lora.setCodingRate(channelConfig.cr);
        m_lora.setSyncWord(channelConfig.syncWord);
        m_lora.setPreambleLength(channelConfig.preambleLength);
    }

    virtual float getRSSI()
    {
        return m_lora.getRSSI();
    }

    virtual float getSNR()
    {
        return m_lora.getSNR();
    }


    virtual int16_t startReceive(uint8_t len = 0)
    {
        return m_lora.startReceive(len, SX127X_RXCONTINUOUS);
    }

    virtual void setDio0Action(void (*func)(void))
    {
        m_lora.setDio0Action(func);
    }

    virtual void clearDio0Action()
    {
        m_lora.clearDio0Action();
    }

    virtual void reset()
    {
        m_lora.reset();
    }
};
