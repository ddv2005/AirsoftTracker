#ifndef ATCOMPONENTS_CPP_UTILS_BLEADVERTISEDDEVICE_H_
#define ATCOMPONENTS_CPP_UTILS_BLEADVERTISEDDEVICE_H_
#include "sdkconfig.h"
#if defined(CONFIG_BLUEDROID_ENABLED)
#include <esp_gattc_api.h>

#include <map>
#include <vector>

#include "BLEAddress.h"
#include "BLEUUID.h"


class atBLEAdvertisedDevice {
public:
	atBLEAdvertisedDevice();

	BLEAddress  getAddress();
	uint16_t    getAppearance();
	std::string getManufacturerData();
	std::string getName();
	int         getRSSI();
	std::string getServiceData();
	std::string getServiceData(int i);
	BLEUUID     getServiceDataUUID();
	BLEUUID     getServiceDataUUID(int i);
	BLEUUID     getServiceUUID();
	BLEUUID     getServiceUUID(int i);
	int         getServiceDataCount();
	int8_t      getTXPower();
	uint8_t* 	getPayload();
	size_t		getPayloadLength();
	esp_ble_addr_type_t getAddressType();
	void setAddressType(esp_ble_addr_type_t type);


	bool		isAdvertisingService(BLEUUID uuid);
	bool        haveAppearance();
	bool        haveManufacturerData();
	bool        haveName();
	bool        haveRSSI();
	bool        haveServiceData();
	bool        haveServiceUUID();
	bool        haveTXPower();

	std::string toString();

	void parseAdvertisement(uint8_t* payload, size_t total_len=62);
	void setPayload(uint8_t* payload, size_t total_len=62);
	void setAddress(BLEAddress address);
	void setAdFlag(uint8_t adFlag);
	void setAdvertizementResult(uint8_t* payload);
	void setAppearance(uint16_t appearance);
	void setManufacturerData(std::string manufacturerData);
	void setName(std::string name);
	void setRSSI(int rssi);
	void setServiceData(std::string data);
	void setServiceDataUUID(BLEUUID uuid);
	void setServiceUUID(const char* serviceUUID);
	void setServiceUUID(BLEUUID serviceUUID);
	void setTXPower(int8_t txPower);

protected:
	bool m_haveAppearance;
	bool m_haveManufacturerData;
	bool m_haveName;
	bool m_haveRSSI;
	bool m_haveServiceData;
	bool m_haveServiceUUID;
	bool m_haveTXPower;


	BLEAddress  m_address = BLEAddress((uint8_t*)"\0\0\0\0\0\0");
	uint8_t     m_adFlag;
	uint16_t    m_appearance;
	int         m_deviceType;
	std::string m_manufacturerData;
	std::string m_name;
	int         m_rssi;
	std::vector<BLEUUID> m_serviceUUIDs;
	int8_t      m_txPower;
	std::vector<std::string> m_serviceData;
	std::vector<BLEUUID> m_serviceDataUUIDs;
	uint8_t*	m_payload;
	size_t		m_payloadLength = 0;
	esp_ble_addr_type_t m_addressType;
};
#endif 
#endif
