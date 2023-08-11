#include "sdkconfig.h"
#if defined(CONFIG_BLUEDROID_ENABLED)
#include <sstream>
#include "atBLEAdvertisedDevice.h"
#include "BLEUtils.h"
#include "esp32-hal-log.h"

atBLEAdvertisedDevice::atBLEAdvertisedDevice() {
	m_adFlag           = 0;
	m_appearance       = 0;
	m_deviceType       = 0;
	m_manufacturerData = "";
	m_name             = "";
	m_rssi             = -9999;
	m_serviceData      = {};
	m_serviceDataUUIDs = {};
	m_txPower          = 0;

	m_haveAppearance       = false;
	m_haveManufacturerData = false;
	m_haveName             = false;
	m_haveRSSI             = false;
	m_haveServiceData      = false;
	m_haveServiceUUID      = false;
	m_haveTXPower          = false;

} // atBLEAdvertisedDevice


/**
 * @brief Get the address.
 *
 * Every %BLE device exposes an address that is used to identify it and subsequently connect to it.
 * Call this function to obtain the address of the advertised device.
 *
 * @return The address of the advertised device.
 */
BLEAddress atBLEAdvertisedDevice::getAddress() {
	return m_address;
} // getAddress


/**
 * @brief Get the appearance.
 *
 * A %BLE device can declare its own appearance.  The appearance is how it would like to be shown to an end user
 * typcially in the form of an icon.
 *
 * @return The appearance of the advertised device.
 */
uint16_t atBLEAdvertisedDevice::getAppearance() {
	return m_appearance;
} // getAppearance


/**
 * @brief Get the manufacturer data.
 * @return The manufacturer data of the advertised device.
 */
std::string atBLEAdvertisedDevice::getManufacturerData() {
	return m_manufacturerData;
} // getManufacturerData


/**
 * @brief Get the name.
 * @return The name of the advertised device.
 */
std::string atBLEAdvertisedDevice::getName() {
	return m_name;
} // getName


/**
 * @brief Get the RSSI.
 * @return The RSSI of the advertised device.
 */
int atBLEAdvertisedDevice::getRSSI() {
	return m_rssi;
} // getRSSI


/**
 * @brief Get the number of service data.
 * @return Number of service data discovered.
 */
int atBLEAdvertisedDevice::getServiceDataCount() {
	if (m_haveServiceData)
		return m_serviceData.size();
	else
		return 0;

} //getServiceDataCount

/**
 * @brief Get the service data.
 * @return The ServiceData of the advertised device.
 */
std::string atBLEAdvertisedDevice::getServiceData() {
	return m_serviceData[0];
} //getServiceData

/**
 * @brief Get the service data.
 * @return The ServiceData of the advertised device.
 */
std::string atBLEAdvertisedDevice::getServiceData(int i) {
	return m_serviceData[i];
} //getServiceData

/**
 * @brief Get the service data UUID.
 * @return The service data UUID.
 */
BLEUUID atBLEAdvertisedDevice::getServiceDataUUID() {
	return m_serviceDataUUIDs[0];
} // getServiceDataUUID

/**
 * @brief Get the service data UUID.
 * @return The service data UUID.
 */
BLEUUID atBLEAdvertisedDevice::getServiceDataUUID(int i) {
	return m_serviceDataUUIDs[i];
} // getServiceDataUUID

/**
 * @brief Get the Service UUID.
 * @return The Service UUID of the advertised device.
 */
BLEUUID atBLEAdvertisedDevice::getServiceUUID() {
	return m_serviceUUIDs[0];
} // getServiceUUID

/**
 * @brief Get the Service UUID.
 * @return The Service UUID of the advertised device.
 */
BLEUUID atBLEAdvertisedDevice::getServiceUUID(int i) {
	return m_serviceUUIDs[i];
} // getServiceUUID

/**
 * @brief Check advertised serviced for existence required UUID
 * @return Return true if service is advertised
 */
bool atBLEAdvertisedDevice::isAdvertisingService(BLEUUID uuid){
	for (int i = 0; i < m_serviceUUIDs.size(); i++) {
		if (m_serviceUUIDs[i].equals(uuid)) return true;
	}
	return false;
}

/**
 * @brief Get the TX Power.
 * @return The TX Power of the advertised device.
 */
int8_t atBLEAdvertisedDevice::getTXPower() {
	return m_txPower;
} // getTXPower



/**
 * @brief Does this advertisement have an appearance value?
 * @return True if there is an appearance value present.
 */
bool atBLEAdvertisedDevice::haveAppearance() {
	return m_haveAppearance;
} // haveAppearance


/**
 * @brief Does this advertisement have manufacturer data?
 * @return True if there is manufacturer data present.
 */
bool atBLEAdvertisedDevice::haveManufacturerData() {
	return m_haveManufacturerData;
} // haveManufacturerData


/**
 * @brief Does this advertisement have a name value?
 * @return True if there is a name value present.
 */
bool atBLEAdvertisedDevice::haveName() {
	return m_haveName;
} // haveName


/**
 * @brief Does this advertisement have a signal strength value?
 * @return True if there is a signal strength value present.
 */
bool atBLEAdvertisedDevice::haveRSSI() {
	return m_haveRSSI;
} // haveRSSI


/**
 * @brief Does this advertisement have a service data value?
 * @return True if there is a service data value present.
 */
bool atBLEAdvertisedDevice::haveServiceData() {
	return m_haveServiceData;
} // haveServiceData


/**
 * @brief Does this advertisement have a service UUID value?
 * @return True if there is a service UUID value present.
 */
bool atBLEAdvertisedDevice::haveServiceUUID() {
	return m_haveServiceUUID;
} // haveServiceUUID


/**
 * @brief Does this advertisement have a transmission power value?
 * @return True if there is a transmission power value present.
 */
bool atBLEAdvertisedDevice::haveTXPower() {
	return m_haveTXPower;
} // haveTXPower


/**
 * @brief Parse the advertising pay load.
 *
 * The pay load is a buffer of bytes that is either 31 bytes long or terminated by
 * a 0 length value.  Each entry in the buffer has the format:
 * [length][type][data...]
 *
 * The length does not include itself but does include everything after it until the next record.  A record
 * with a length value of 0 indicates a terminator.
 *
 * https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile
 */
void atBLEAdvertisedDevice::parseAdvertisement(uint8_t* payload, size_t total_len) {
	uint8_t length;
	uint8_t ad_type;
	uint8_t sizeConsumed = 0;
	bool finished = false;
	m_payload = payload;
	m_payloadLength = total_len;

	while(!finished) {
		length = *payload;          // Retrieve the length of the record.
		payload++;                  // Skip to type
		sizeConsumed += 1 + length; // increase the size consumed.

		if (length != 0) { // A length of 0 indicates that we have reached the end.
			ad_type = *payload;
			payload++;
			length--;

			char* pHex = BLEUtils::buildHexData(nullptr, payload, length);
			log_d("Type: 0x%.2x (%s), length: %d, data: %s",
					ad_type, BLEUtils::advTypeToString(ad_type), length, pHex);
			free(pHex);

			switch(ad_type) {
				case ESP_BLE_AD_TYPE_NAME_CMPL: {   // Adv Data Type: 0x09
					setName(std::string(reinterpret_cast<char*>(payload), length));
					break;
				} // ESP_BLE_AD_TYPE_NAME_CMPL

				case ESP_BLE_AD_TYPE_TX_PWR: {      // Adv Data Type: 0x0A
					setTXPower(*payload);
					break;
				} // ESP_BLE_AD_TYPE_TX_PWR

				case ESP_BLE_AD_TYPE_APPEARANCE: { // Adv Data Type: 0x19
					setAppearance(*reinterpret_cast<uint16_t*>(payload));
					break;
				} // ESP_BLE_AD_TYPE_APPEARANCE

				case ESP_BLE_AD_TYPE_FLAG: {        // Adv Data Type: 0x01
					setAdFlag(*payload);
					break;
				} // ESP_BLE_AD_TYPE_FLAG

				case ESP_BLE_AD_TYPE_16SRV_CMPL:
				case ESP_BLE_AD_TYPE_16SRV_PART: {   // Adv Data Type: 0x02
					for (int var = 0; var < length/2; ++var) {
						setServiceUUID(BLEUUID(*reinterpret_cast<uint16_t*>(payload + var * 2)));
					}
					break;
				} // ESP_BLE_AD_TYPE_16SRV_PART

				case ESP_BLE_AD_TYPE_32SRV_CMPL:
				case ESP_BLE_AD_TYPE_32SRV_PART: {   // Adv Data Type: 0x04
					for (int var = 0; var < length/4; ++var) {
						setServiceUUID(BLEUUID(*reinterpret_cast<uint32_t*>(payload + var * 4)));
					}
					break;
				} // ESP_BLE_AD_TYPE_32SRV_PART

				case ESP_BLE_AD_TYPE_128SRV_CMPL: { // Adv Data Type: 0x07
					setServiceUUID(BLEUUID(payload, 16, false));
					break;
				} // ESP_BLE_AD_TYPE_128SRV_CMPL

				case ESP_BLE_AD_TYPE_128SRV_PART: { // Adv Data Type: 0x06
					setServiceUUID(BLEUUID(payload, 16, false));
					break;
				} // ESP_BLE_AD_TYPE_128SRV_PART

				// See CSS Part A 1.4 Manufacturer Specific Data
				case ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE: {
					setManufacturerData(std::string(reinterpret_cast<char*>(payload), length));
					break;
				} // ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE

				case ESP_BLE_AD_TYPE_SERVICE_DATA: {  // Adv Data Type: 0x16 (Service Data) - 2 byte UUID
					if (length < 2) {
						log_e("Length too small for ESP_BLE_AD_TYPE_SERVICE_DATA");
						break;
					}
					uint16_t uuid = *(uint16_t*)payload;
					setServiceDataUUID(BLEUUID(uuid));
					if (length > 2) {
						setServiceData(std::string(reinterpret_cast<char*>(payload + 2), length - 2));
					}
					break;
				} //ESP_BLE_AD_TYPE_SERVICE_DATA

				case ESP_BLE_AD_TYPE_32SERVICE_DATA: {  // Adv Data Type: 0x20 (Service Data) - 4 byte UUID
					if (length < 4) {
						log_e("Length too small for ESP_BLE_AD_TYPE_32SERVICE_DATA");
						break;
					}
					uint32_t uuid = *(uint32_t*) payload;
					setServiceDataUUID(BLEUUID(uuid));
					if (length > 4) {
						setServiceData(std::string(reinterpret_cast<char*>(payload + 4), length - 4));
					}
					break;
				} //ESP_BLE_AD_TYPE_32SERVICE_DATA

				case ESP_BLE_AD_TYPE_128SERVICE_DATA: {  // Adv Data Type: 0x21 (Service Data) - 16 byte UUID
					if (length < 16) {
						log_e("Length too small for ESP_BLE_AD_TYPE_128SERVICE_DATA");
						break;
					}

					setServiceDataUUID(BLEUUID(payload, (size_t)16, false));
					if (length > 16) {
						setServiceData(std::string(reinterpret_cast<char*>(payload + 16), length - 16));
					}
					break;
				} //ESP_BLE_AD_TYPE_32SERVICE_DATA

				default: {
					log_d("Unhandled type: adType: %d - 0x%.2x", ad_type, ad_type);
					break;
				}
			} // switch
			payload += length;
		} // Length <> 0


		if (sizeConsumed >= total_len)
			finished = true;

	} // !finished
} // parseAdvertisement

/**
 * @brief Parse the advertising payload.
 * @param [in] payload The payload of the advertised device.
 * @param [in] total_len The length of payload
 */
void atBLEAdvertisedDevice::setPayload(uint8_t* payload, size_t total_len) {
	m_payload = payload;
	m_payloadLength = total_len;
} // setPayload

/**
 * @brief Set the address of the advertised device.
 * @param [in] address The address of the advertised device.
 */
void atBLEAdvertisedDevice::setAddress(BLEAddress address) {
	m_address = address;
} // setAddress


/**
 * @brief Set the adFlag for this device.
 * @param [in] The discovered adFlag.
 */
void atBLEAdvertisedDevice::setAdFlag(uint8_t adFlag) {
	m_adFlag = adFlag;
} // setAdFlag


/**
 * @brief Set the appearance for this device.
 * @param [in] The discovered appearance.
 */
void atBLEAdvertisedDevice::setAppearance(uint16_t appearance) {
	m_appearance     = appearance;
	m_haveAppearance = true;
	log_d("- appearance: %d", m_appearance);
} // setAppearance


/**
 * @brief Set the manufacturer data for this device.
 * @param [in] The discovered manufacturer data.
 */
void atBLEAdvertisedDevice::setManufacturerData(std::string manufacturerData) {
	m_manufacturerData     = manufacturerData;
	m_haveManufacturerData = true;
	char* pHex = BLEUtils::buildHexData(nullptr, (uint8_t*) m_manufacturerData.data(), (uint8_t) m_manufacturerData.length());
	log_d("- manufacturer data: %s", pHex);
	free(pHex);
} // setManufacturerData


/**
 * @brief Set the name for this device.
 * @param [in] name The discovered name.
 */
void atBLEAdvertisedDevice::setName(std::string name) {
	m_name     = name;
	m_haveName = true;
	log_d("- setName(): name: %s", m_name.c_str());
} // setName


/**
 * @brief Set the RSSI for this device.
 * @param [in] rssi The discovered RSSI.
 */
void atBLEAdvertisedDevice::setRSSI(int rssi) {
	m_rssi     = rssi;
	m_haveRSSI = true;
	log_d("- setRSSI(): rssi: %d", m_rssi);
} // setRSSI


/**
 * @brief Set the Service UUID for this device.
 * @param [in] serviceUUID The discovered serviceUUID
 */
void atBLEAdvertisedDevice::setServiceUUID(const char* serviceUUID) {
	return setServiceUUID(BLEUUID(serviceUUID));
} // setServiceUUID


/**
 * @brief Set the Service UUID for this device.
 * @param [in] serviceUUID The discovered serviceUUID
 */
void atBLEAdvertisedDevice::setServiceUUID(BLEUUID serviceUUID) {
	m_serviceUUIDs.push_back(serviceUUID);
	m_haveServiceUUID = true;
	log_d("- addServiceUUID(): serviceUUID: %s", serviceUUID.toString().c_str());
} // setServiceUUID


/**
 * @brief Set the ServiceData value.
 * @param [in] data ServiceData value.
 */
void atBLEAdvertisedDevice::setServiceData(std::string serviceData) {
	m_haveServiceData = true;         // Set the flag that indicates we have service data.
	m_serviceData.push_back(serviceData); // Save the service data that we received.
} //setServiceData


/**
 * @brief Set the ServiceDataUUID value.
 * @param [in] data ServiceDataUUID value.
 */
void atBLEAdvertisedDevice::setServiceDataUUID(BLEUUID uuid) {
	m_haveServiceData = true;         // Set the flag that indicates we have service data.
	m_serviceDataUUIDs.push_back(uuid);
	log_d("- addServiceDataUUID(): serviceDataUUID: %s", uuid.toString().c_str());
} // setServiceDataUUID


/**
 * @brief Set the power level for this device.
 * @param [in] txPower The discovered power level.
 */
void atBLEAdvertisedDevice::setTXPower(int8_t txPower) {
	m_txPower     = txPower;
	m_haveTXPower = true;
	log_d("- txPower: %d", m_txPower);
} // setTXPower


/**
 * @brief Create a string representation of this device.
 * @return A string representation of this device.
 */
std::string atBLEAdvertisedDevice::toString() {
	std::string res = "Name: " + getName() + ", Address: " + getAddress().toString();
	if (haveAppearance()) {
		char val[6];
		snprintf(val, sizeof(val), "%d", getAppearance());
		res += ", appearance: ";
		res += val;
	}
	if (haveManufacturerData()) {
		char *pHex = BLEUtils::buildHexData(nullptr, (uint8_t*)getManufacturerData().data(), getManufacturerData().length());
		res += ", manufacturer data: ";
		res += pHex;
		free(pHex);
	}
	if (haveServiceUUID()) {
		for (int i=0; i < m_serviceUUIDs.size(); i++) {
		    res += ", serviceUUID: " + getServiceUUID(i).toString();
		}
	}
	if (haveTXPower()) {
		char val[6];
		snprintf(val, sizeof(val), "%d", getTXPower());
		res += ", txPower: ";
		res += val;
	}
	return res;
} // toString

uint8_t* atBLEAdvertisedDevice::getPayload() {
	return m_payload;
}

esp_ble_addr_type_t atBLEAdvertisedDevice::getAddressType() {
	return m_addressType;
}

void atBLEAdvertisedDevice::setAddressType(esp_ble_addr_type_t type) {
	m_addressType = type;
}

size_t atBLEAdvertisedDevice::getPayloadLength() {
	return m_payloadLength;
}

#endif /* CONFIG_BLUEDROID_ENABLED */

