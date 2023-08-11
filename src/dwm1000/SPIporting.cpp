/*
 * MIT License
 * 
 * Copyright (c) 2018 Michele Biondi, Andrea Salvatori
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * @file SPIporting.hpp
 * Arduino porting for the SPI interface.
*/

#include <Arduino.h>
#include <SPI.h>
#include "SPIporting.hpp"
#include "DW1000NgConstants.hpp"
#include "DW1000NgRegisters.hpp"

uint32_t m_spiClock = 0;
spi_host_device_t m_spiHost;
spi_device_handle_t m_spiHandle = 0;
Adafruit_MCP23017 *m_expander = NULL;	

namespace SPIporting {
	
	void SPIInitWithClock(uint32_t clock)
	{
		if(m_spiHandle)
		{
			spi_bus_remove_device(m_spiHandle);
			m_spiHandle = 0;
		}
		spi_device_interface_config_t devcfg={
			.command_bits = 0,
			.address_bits = 0,
			.dummy_bits = 0,
			.mode=0,                     
			.clock_speed_hz=clock,
			.spics_io_num=-1,
			.flags = 0,
			.queue_size=7,
			.pre_cb=NULL,
			.post_cb=NULL,
		};	
		m_spiClock = clock;
		spi_bus_add_device(m_spiHost, &devcfg, &m_spiHandle);
	}
	
	namespace {

		constexpr uint32_t EspSPImaximumSpeed = 20000000; //20MHz
		constexpr uint32_t SPIminimumSpeed = 2000000; //2MHz

		const SPISettings _fastSPI = SPISettings(EspSPImaximumSpeed, MSBFIRST, SPI_MODE0);
		const SPISettings _slowSPI = SPISettings(SPIminimumSpeed, MSBFIRST, SPI_MODE0);
		const SPISettings* _currentSPI = &_fastSPI;

		void _openSPI(uint8_t slaveSelectPIN) {
			if(_currentSPI->_clock!=m_spiClock)
			{
				SPIInitWithClock(_currentSPI->_clock);
			}
			m_expander->digitalWrite(slaveSelectPIN, LOW);
		}

    	void _closeSPI(uint8_t slaveSelectPIN) {
			m_expander->digitalWrite(slaveSelectPIN, HIGH);
		}
	}


	void SPIinit(spi_host_device_t spihost, Adafruit_MCP23017 *expander) {
		m_spiHost = spihost;
		m_expander = expander;
		SPIInitWithClock(EspSPImaximumSpeed);
	}

	void SPIend() {
		spi_bus_remove_device(m_spiHandle);
		m_spiHandle = 0;
	}

	void SPIselect(uint8_t slaveSelectPIN, uint8_t irq) {
		m_expander->pinMode(slaveSelectPIN, OUTPUT);
		m_expander->digitalWrite(slaveSelectPIN, HIGH);
	}

	uint8_t spi_transfer(uint8_t data)
	{
		esp_err_t ret;
		spi_transaction_t t;
		memset(&t, 0, sizeof(t));
		t.flags = SPI_TRANS_USE_RXDATA;
		t.length=8;        
		t.tx_buffer=&data;
		ret=spi_device_transmit(m_spiHandle, &t);
		assert(ret==ESP_OK);
		return t.rx_data[0];
	}


	void writeToSPI(uint8_t slaveSelectPIN, uint8_t headerLen, byte header[], uint16_t dataLen, byte data[]) {
		_openSPI(slaveSelectPIN);
		for(auto i = 0; i < headerLen; i++) {
			spi_transfer(header[i]); // send header
		}
		for(auto i = 0; i < dataLen; i++) {
			spi_transfer(data[i]); // write values
		}
		delayMicroseconds(5);
		_closeSPI(slaveSelectPIN);
	}

    void readFromSPI(uint8_t slaveSelectPIN, uint8_t headerLen, byte header[], uint16_t dataLen, byte data[]){
		_openSPI(slaveSelectPIN);
		for(auto i = 0; i < headerLen; i++) {
			spi_transfer(header[i]); // send header
		}
		for(auto i = 0; i < dataLen; i++) {
			data[i] = spi_transfer(0x00); // read values
		}
		delayMicroseconds(5);
		_closeSPI(slaveSelectPIN);
	}

	void setSPIspeed(SPIClock speed) {
		if(speed == SPIClock::FAST) {
			_currentSPI = &_fastSPI;
		 } else if(speed == SPIClock::SLOW) {
			_currentSPI = &_slowSPI;
		 }
	}
}