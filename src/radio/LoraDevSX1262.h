#pragma once
#include "LoraRadio.h"
#include <modules\SX126x\SX1262.h>

class cLoraDeviceSX1262:public cLoraDevice
{
protected:
    Module m_module;
    SX1262 m_lora;
public:
    cLoraDeviceSX1262(sRFModuleConfig &moduleConfig,cLoraChannelCondfig &channelConfig):cLoraDevice(channelConfig),
        m_module(moduleConfig.cs, moduleConfig.irq, moduleConfig.rst, moduleConfig.busy, *moduleConfig.spi, *moduleConfig.spiSettings),
        m_lora(&m_module)
    {
        m_intf = &m_lora;
    }

    virtual int16_t applyConfig()
    {
#ifndef SX1262_E22
    float tcxoVoltage = 0; // None - we use an XTAL
#else
    float tcxoVoltage =
        1.8; // E22 uses DIO3 to power tcxo per https://github.com/jgromes/RadioLib/issues/12#issuecomment-520695575
#endif
        bool useRegulatorLDO = false; // Seems to depend on the connection to pin 9/DCC_SW - if an inductor DCDC?

        if (m_config.power == 0)
            m_config.power = 22;

        if (m_config.power > 22) // This chip has lower power limits than some
            m_config.power = 22;

        int16_t result = m_lora.begin(m_config.freq, m_config.bw, m_config.sf, m_config.cr, m_config.syncWord, m_config.power, m_config.currentLimit, m_config.preambleLength, tcxoVoltage,useRegulatorLDO);
        m_lora.setRxGain(true);
        return result;
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
        uint16_t result = m_lora.startReceive();
        m_lora.setDioIrqParams(SX126X_IRQ_PREAMBLE_DETECTED | SX126X_IRQ_HEADER_VALID | SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CRC_ERR | SX126X_IRQ_HEADER_ERR, 
            SX126X_IRQ_PREAMBLE_DETECTED | SX126X_IRQ_RX_DONE | SX126X_IRQ_CRC_ERR | SX126X_IRQ_HEADER_ERR);
        return result;
    }

    virtual void setDio0Action(void (*func)(void))
    {
        m_lora.setDio1Action(func);
    }

    virtual void clearIrqStatus(uint16_t s)
    {
        m_lora.clearIrqStatus(s);
    }

    virtual void clearPreambleIrq()
    {
        clearIrqStatus(SX126X_IRQ_PREAMBLE_DETECTED);
    }

    virtual bool isPreambleIrq(uint16_t status)
    {
        return status & SX126X_IRQ_PREAMBLE_DETECTED;
    }

    virtual bool isRxDoneIrq(uint16_t status)
    {
        return status & SX126X_IRQ_RX_DONE;
    }


    virtual void clearDio0Action()
    {
        m_lora.clearDio1Action();
    }

    virtual void reset()
    {
        m_lora.reset();
    }

    virtual uint32_t getIrqStatus() { return m_lora.getIrqStatus(); }
};
