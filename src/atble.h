#pragma once
#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "adcBLECommon.h"
#include "config.h"
#include "concurrency/Lock.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "BLE/atBLEAdvertisedDevice.h"

#define PROFILE_NUM      1

struct sGattcProfile {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
};

class atBLE
{
protected:
    struct sGattcProfile m_glProfileTab[PROFILE_NUM];
    bool m_enableScan;
    bool m_newDataAvailable;
    uint16_t m_poi;
    int64_t m_lastUpdate;
    concurrency::Lock m_lock;
    int  m_bleState;
    bool m_hasServer;
    std::vector<uint8_t> m_data;
    int64_t m_lastDisconnect;
    int64_t m_lastReadRequest;
    int64_t m_lastDeviceFound;
#ifndef AD_BLE_NO_CLIENT
    void internalEnableScan();
    void onDeviceFound(atBLEAdvertisedDevice &device);
#endif    
public:
    atBLE();
    void begin(uint16_t poi);
#ifndef AD_BLE_NO_CLIENT    
    void enableScan(bool en);
    bool getEnableScan() { return m_enableScan; }    
#endif    
    void loop();

    void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
#ifndef AD_BLE_NO_CLIENT    
    void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
    void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);    
#endif    
};