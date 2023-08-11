#include "atble.h"
#include "BLEDevice.h"
#include "global.h"
#include <nvs_flash.h>

static BLEUUID adcServiceUUID(SERVICE_UUID_ADC);
static BLEUUID adcCharasteristicUUID(CHARACTERISTIC_UUID_ADC_POSITION_REPORT);

#define PROFILE_A_APP_ID 0

atBLE *bleDevice = NULL;

#define BS_IDLE         0
#define BS_SCANING      1
#define BS_CONNECTING   2
#define BS_CONNECTING2          3
#define BS_DISCOVER_SERVICES   4
#define BS_CONNECTED    5
#define BS_DICONNECTED  100
#define BS_ERROR       101

#ifndef AD_BLE_NO_CLIENT
static void _gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    if(bleDevice)
        bleDevice->gattc_profile_event_handler(event,gattc_if,param);
};

void atBLE::gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    m_lock.lock();
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
    switch (event) {
    case ESP_GATTC_REG_EVT:
    {
//        DEBUG_MSG("REG_EVT\n");
        static esp_ble_scan_params_t ble_scan_params = {
            .scan_type              = BLE_SCAN_TYPE_ACTIVE,
            .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
            .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
            .scan_interval          = 0x4000,
            .scan_window            = 312,
            .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
        };        
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            DEBUG_MSG("set scan params error, error code = %x\n", scan_ret);
        }
        break;
    }
    case ESP_GATTC_CONNECT_EVT:{
        if(m_bleState==BS_CONNECTING)
        {
            //DEBUG_MSG("ESP_GATTC_CONNECT_EVT conn_id %d, if %d\n", p_data->connect.conn_id, gattc_if);
            m_glProfileTab[PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
            memcpy(m_glProfileTab[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
            esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
            if (mtu_ret){
                DEBUG_MSG("config MTU error, error code = %x\n", mtu_ret);
            }
            m_bleState = BS_CONNECTING2;
        }
        break;
    }

    case ESP_GATTC_CFG_MTU_EVT:
    {
        //DEBUG_MSG("ESP_GATTC_CFG_MTU_EVT\n");
        if(m_bleState==BS_CONNECTING2)
        {
            m_bleState = BS_DISCOVER_SERVICES;
            esp_ble_gattc_search_service(gattc_if, param->open.conn_id, adcServiceUUID.getNative());
        }
        else
            DEBUG_MSG("BLE Wrong State\n");
        break;
    }

    case ESP_GATTC_OPEN_EVT:
        if(m_bleState==BS_CONNECTING2)
        {
            if (param->open.status != ESP_GATT_OK){
                DEBUG_MSG("open failed, status %d\n", p_data->open.status);
                m_bleState = BS_ERROR;
                break;
            }
            //DEBUG_MSG("open success\n");
        }
        break;    

    case ESP_GATTC_SEARCH_RES_EVT: {
        //DEBUG_MSG("SEARCH RES: conn_id = %x is primary service %d\n", p_data->search_res.conn_id, p_data->search_res.is_primary);
        //DEBUG_MSG("start handle %d end handle %d current handle value %d\n", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        BLEUUID remoteService(p_data->search_res.srvc_id);
        if (remoteService.equals(adcServiceUUID)) {
            //DEBUG_MSG("service found\n");
            m_hasServer = true;
            m_glProfileTab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
            m_glProfileTab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
        }
        break;
    }        

    case ESP_GATTC_SEARCH_CMPL_EVT:
    {
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            DEBUG_MSG("search service failed, error status = %x\n", p_data->search_cmpl.status);
            m_bleState = BS_ERROR;
            break;
        }
        if (m_hasServer){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     m_glProfileTab[PROFILE_A_APP_ID].service_start_handle,
                                                                     m_glProfileTab[PROFILE_A_APP_ID].service_end_handle,
                                                                     0,
                                                                     &count);
            if (status != ESP_GATT_OK){
                DEBUG_MSG("esp_ble_gattc_get_attr_count error\n");
            }

            if (count > 0)
            {
                esp_gattc_char_elem_t *char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (char_elem_result)
                {
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             m_glProfileTab[PROFILE_A_APP_ID].service_start_handle,
                                                             m_glProfileTab[PROFILE_A_APP_ID].service_end_handle,
                                                             *adcCharasteristicUUID.getNative(),
                                                             char_elem_result,
                                                             &count);
                    if (status != ESP_GATT_OK){
                        DEBUG_MSG("esp_ble_gattc_get_char_by_uuid error\n");
                    }

                    if(count>0)
                    {
                        m_glProfileTab[PROFILE_A_APP_ID].char_handle = char_elem_result[0].char_handle;
                        if (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
                            esp_ble_gattc_register_for_notify(gattc_if, m_glProfileTab[PROFILE_A_APP_ID].remote_bda, char_elem_result[0].char_handle);
                        }
                        m_bleState = BS_CONNECTED;
                    }
                    else
                    {
                        m_bleState = BS_ERROR;
                    }
                    free(char_elem_result);
                }
                else
                    m_bleState = BS_ERROR;
            }else{
                DEBUG_MSG("no char found\n");
                m_bleState = BS_ERROR;
            }
        }
         break;    
    }

    case ESP_GATTC_WRITE_DESCR_EVT:
    {
        if (p_data->write.status != ESP_GATT_OK){
            DEBUG_MSG("write descr failed, error status = %x\n", p_data->write.status);
            break;
        }
        //DEBUG_MSG("write descr success\n");
        uint8_t write_char_data[35];
        for (int i = 0; i < sizeof(write_char_data); ++i)
        {
            write_char_data[i] = i % 256;
        }
        esp_ble_gattc_write_char( gattc_if,
                                  m_glProfileTab[PROFILE_A_APP_ID].conn_id,
                                  m_glProfileTab[PROFILE_A_APP_ID].char_handle,
                                  sizeof(write_char_data),
                                  write_char_data,
                                  ESP_GATT_WRITE_TYPE_RSP,
                                  ESP_GATT_AUTH_REQ_NONE);
        break;
    }

    case ESP_GATTC_REG_FOR_NOTIFY_EVT: 
    {
        //DEBUG_MSG("ESP_GATTC_REG_FOR_NOTIFY_EVT\n");
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            DEBUG_MSG("REG FOR NOTIFY failed: error status = %d\n", p_data->reg_for_notify.status);
        }else{
            uint16_t count = 0;
            uint16_t notify_en = 1;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                         m_glProfileTab[PROFILE_A_APP_ID].conn_id,
                                                                         ESP_GATT_DB_DESCRIPTOR,
                                                                         m_glProfileTab[PROFILE_A_APP_ID].service_start_handle,
                                                                         m_glProfileTab[PROFILE_A_APP_ID].service_end_handle,
                                                                         m_glProfileTab[PROFILE_A_APP_ID].char_handle,
                                                                         &count);
            if (ret_status != ESP_GATT_OK){
                DEBUG_MSG("esp_ble_gattc_get_attr_count error\n");
            }
            if (count > 0){
                esp_gattc_descr_elem_t *descr_elem_result = (esp_gattc_descr_elem_t *)malloc(sizeof(esp_gattc_descr_elem_t) * count);
                if (descr_elem_result)
                {
                    esp_bt_uuid_t notify_descr_uuid = {
                        .len = ESP_UUID_LEN_16,
                        .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
                    };
                    ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                         m_glProfileTab[PROFILE_A_APP_ID].conn_id,
                                                                         p_data->reg_for_notify.handle,
                                                                         notify_descr_uuid,
                                                                         descr_elem_result,
                                                                         &count);
                    if (ret_status != ESP_GATT_OK){
                        DEBUG_MSG("esp_ble_gattc_get_descr_by_char_handle error\n");
                    }
                    if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                        esp_err_t errRc = esp_ble_gattc_write_char_descr( gattc_if,
                                                                     m_glProfileTab[PROFILE_A_APP_ID].conn_id,
                                                                     descr_elem_result[0].handle,
                                                                     sizeof(notify_en),
                                                                     (uint8_t *)&notify_en,
                                                                     ESP_GATT_WRITE_TYPE_RSP,
                                                                     ESP_GATT_AUTH_REQ_NONE);
                        if (errRc != ESP_OK)
                            DEBUG_MSG("esp_ble_gattc_write_char_descr error\n");
                    }
                    free(descr_elem_result);
                }
            }
            else{
                DEBUG_MSG("decsr not found\n");
            }
        }
        break;
    }    

    case ESP_GATTC_NOTIFY_EVT:
    {
        //DEBUG_MSG("ESP_GATTC_NOTIFY_EVT LEN %d\n", (int)p_data->notify.value_len);
        if (p_data->notify.handle != m_glProfileTab[PROFILE_A_APP_ID].char_handle) break;
        m_newDataAvailable = true;
        m_data.resize(param->notify.value_len);
 		memcpy(m_data.data(), param->notify.value, param->notify.value_len);
        break;
    }

	case ESP_GATTC_READ_CHAR_EVT: 
    {
        //DEBUG_MSG("ESP_GATTC_READ_CHAR_EVT %d %d\n",p_data->read.status,param->read.value_len);
		if (p_data->read.handle != m_glProfileTab[PROFILE_A_APP_ID].char_handle) break;

			// At this point, we have determined that the event is for us, so now we save the value
			// and unlock the semaphore to ensure that the requestor of the data can continue.
		if (p_data->read.status == ESP_GATT_OK) {
            m_lastReadRequest = 0;
            m_data.resize(param->read.value_len);
    		memcpy(m_data.data(), param->read.value, param->read.value_len);
		}
        else
            m_bleState = BS_ERROR;
		break;
	} 


    case ESP_GATTC_DISCONNECT_EVT:
        if(p_data->disconnect.conn_id==m_glProfileTab[PROFILE_A_APP_ID].conn_id)
        {
            m_hasServer = false;
            m_bleState = BS_DICONNECTED;
        }
        
        //DEBUG_MSG("ESP_GATTC_DISCONNECT_EVT, reason = %d\n", p_data->disconnect.reason);
        break;

    default:
        break;
    };  
    m_lock.unlock();
}
#endif

static void _esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    if(bleDevice)
        bleDevice->esp_gap_cb(event,param);
}
#ifndef AD_BLE_NO_CLIENT
static void _esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    if(bleDevice)
        bleDevice->esp_gattc_cb(event,gattc_if,param);
}

void atBLE::onDeviceFound(atBLEAdvertisedDevice &device)
{
    if (device.haveServiceUUID() && device.isAdvertisingService(adcServiceUUID))
    {
        DEBUG_MSG("BLE device found. Connecting...\n");
        m_lastDeviceFound = millis();
        ::esp_ble_gap_stop_scanning();
        m_bleState = BS_CONNECTING;
        esp_ble_gattc_open(m_glProfileTab[PROFILE_A_APP_ID].gattc_if, (uint8_t*) device.getAddress().getNative(), device.getAddressType(), true);
    }
}
#endif
void atBLE::esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    m_lock.lock();
    switch (event) {
#ifndef AD_BLE_NO_CLIENT        
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
        {
            if(m_bleState == BS_SCANING)
            {
                BLEAddress advertisedAddress(param->scan_rst.bda);
                atBLEAdvertisedDevice advertisedDevice;
                advertisedDevice.setAddress(advertisedAddress);
                advertisedDevice.setRSSI(scan_result->scan_rst.rssi);
                advertisedDevice.setAdFlag(scan_result->scan_rst.flag);
                advertisedDevice.parseAdvertisement((uint8_t*)scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len);
                advertisedDevice.setAddressType(scan_result->scan_rst.ble_addr_type);

                onDeviceFound(advertisedDevice);
            }
            break;
        }
        default:
            break;
        }
        break;
    }
#endif
    default:
        break;
    }
    m_lock.unlock();
}
#ifndef AD_BLE_NO_CLIENT
void atBLE::esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            m_glProfileTab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            DEBUG_MSG("reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == m_glProfileTab[idx].gattc_if) {
                if (m_glProfileTab[idx].gattc_cb) {
                    m_glProfileTab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}
#endif

atBLE::atBLE()
{
    m_enableScan = false;
#ifndef AD_BLE_NO_CLIENT    
    m_glProfileTab[PROFILE_A_APP_ID] = {
        .gattc_cb = _gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
        };
#endif        
}

void atBLE::begin(uint16_t poi)
{
    m_lastReadRequest = 0;
    m_lastDeviceFound = 0;
    m_lastDisconnect = 0;
    m_hasServer = false;
    m_bleState = BS_IDLE;
    bleDevice = this;
    m_lastUpdate = 0;
    m_poi = poi;
    esp_err_t ret;    
/*

#ifdef ARDUINO_ARCH_ESP32
	if (!btStart()) {
		DEBUG_MSG("BLE Start failed\n");
		return;
	}
#else
    ret = ::nvs_flash_init();
	if (ret != ESP_OK) {
		DEBUG_MSG("nvs_flash_init: rc=%d\n", ret);
		return;
	}

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        DEBUG_MSG("%s initialize controller failed, error code = %x\n", __func__, ret);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        DEBUG_MSG("%s enable controller failed, error code = %x\n", __func__, ret);
        return;
    }
#endif
    ret = esp_bluedroid_init();
    if (ret) {
        DEBUG_MSG("%s init bluetooth failed, error code = %x\n", __func__, ret);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        DEBUG_MSG("%s enable bluetooth failed, error code = %x\n", __func__, ret);
        return;
    }

    //register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(_esp_gap_cb);
    if (ret){
        DEBUG_MSG("%s gap register failed, error code = %x\n", __func__, ret);
        return;
    }

    //register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(_esp_gattc_cb);
    if(ret){
        DEBUG_MSG("%s gattc register failed, error code = %x\n", __func__, ret);
        return;
    }
*/
    char bleName[32];
    snprintf(bleName,sizeof(bleName),"TTracker %0X (%s)",global->getConfig().id,global->getConfig().name.c_str());
    BLEDevice::init(bleName);
    BLEDevice::setCustomGapHandler(_esp_gap_cb);
#ifndef AD_BLE_NO_CLIENT    
    BLEDevice::setCustomGattcHandler(_esp_gattc_cb);
    ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
    if (ret){
        DEBUG_MSG("%s gattc app register failed, error code = %x\n", __func__, ret);
    }
#endif
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        DEBUG_MSG("set local  MTU failed, error code = %x\n", local_mtu_ret);
    }
}
#ifndef AD_BLE_NO_CLIENT
void atBLE::internalEnableScan()
{
    m_lock.lock();
    static esp_ble_scan_params_t ble_scan_params = {
        .scan_type              = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval          = 0x4000,
        .scan_window            = 312,
        .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
    };        
    esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
    if (scan_ret){
       DEBUG_MSG("set scan params error, error code = %x\n", scan_ret);
    }
    ::esp_ble_gap_start_scanning(0);
    if(m_bleState == BS_IDLE)
        m_bleState = BS_SCANING;
    m_lock.unlock();
}

void atBLE::enableScan(bool en)
{
    if(m_enableScan!=en)
    {
        m_enableScan = en;
        if(m_enableScan)
        {
            internalEnableScan();
        }
        else
        {
            ::esp_ble_gap_stop_scanning();
        }
    }
}
#endif

void atBLE::loop()
{
#ifndef AD_BLE_NO_CLIENT    
    m_lock.lock();

    if(m_bleState==BS_IDLE && m_enableScan && (millis()-m_lastDisconnect)>5000)
    {
        m_bleState=BS_SCANING;
        m_lock.unlock();
        internalEnableScan();
        m_lock.lock();
    }

    if(m_bleState==BS_DICONNECTED || m_bleState==BS_ERROR)
    {
        //DEBUG_MSG("BLE Deleting %d\n",m_bleState);
        m_data.clear();
        m_lastReadRequest = 0;
        m_lastDeviceFound = 0;
        //global->removePOI(m_poi);
        auto conn_id = m_glProfileTab[PROFILE_A_APP_ID].conn_id;
        m_glProfileTab[PROFILE_A_APP_ID].conn_id = 0xFFFF;
        m_lock.unlock();
        if(conn_id!=0xFFFF)
            ::esp_ble_gattc_close(m_glProfileTab[PROFILE_A_APP_ID].gattc_if, conn_id);
        m_lock.lock();
        delay(10);
        m_bleState=BS_IDLE;
        m_lastDisconnect = millis();
    }
    else
    if(m_bleState==BS_CONNECTED)
    {
        if(m_data.size()>0)
        {
            if(m_data.size()==sizeof(sADCPositionReport))
            {
                m_lastUpdate = millis();
                m_newDataAvailable = false;
                sADCPositionReport *report = (sADCPositionReport *)m_data.data();
                sPOI poi;
                poi.lng = report->longitude;
                poi.lat = report->latitude;
                poi.alt = report->altitude;
                poi.lastUpdate = (int64_t)millis() - ((int64_t)report->now - (int64_t) report->lastSeen);
                //DEBUG_MSG("R %d %d %d\n",(int)poi.lastUpdate,(int)report->now,(int)report->lastSeen);
                poi.RSSI = report->RSSI;
                poi.SNR = report->SNR;
                poi.updateXY();
                poi.name = report->status;
                poi.symbol = 'D';
                poi.type = POIT_DYNAMIC;
                poi.positionChanged = true;
                poi.hacc = report->hacc;
                global->setPOI(m_poi,poi);
            }
            m_data.clear();
        }
        else
        if(m_lastReadRequest==0 && (m_newDataAvailable || (millis()-m_lastUpdate)>5500))
        {
            m_newDataAvailable = false;
            m_lastUpdate = millis();
            m_lastReadRequest = millis();
            esp_err_t errRc = ::esp_ble_gattc_read_char(
                m_glProfileTab[PROFILE_A_APP_ID].gattc_if,
                m_glProfileTab[PROFILE_A_APP_ID].conn_id,
                m_glProfileTab[PROFILE_A_APP_ID].char_handle,
                ESP_GATT_AUTH_REQ_NONE);                                         // Security

            if (errRc != ESP_OK) {
                DEBUG_MSG("esp_ble_gattc_read_char: rc=%d\n", errRc);
            }
        }
        if(m_lastReadRequest!=0 && (millis()-m_lastReadRequest)>5000)
        {
            m_bleState = BS_ERROR;
            m_lastReadRequest = 0;
            DEBUG_MSG("BLE read timeout\n");
        }
    }
    if(m_lastDeviceFound!=0 && m_bleState<BS_DICONNECTED && m_bleState!=BS_CONNECTED && m_bleState>BS_SCANING && (millis()-m_lastDeviceFound)>10000)
    {
        m_bleState = BS_ERROR;
        m_lastDeviceFound = 0;
        DEBUG_MSG("BLE connect timeout\n");
    }
    m_lock.unlock();
#endif    
}