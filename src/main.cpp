#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <Wire.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <FS.h>
#include "config.h"
#include "global.h"
#include "esp_task_wdt.h"
#include <OneButton.h>
#include "power.h"
#include "atracker.h"
#include <esp_spiffs.h>
#include <EEPROM.h>

cAirsoftTracker *global = NULL;
SemaphoreHandle_t debugLock = xSemaphoreCreateMutex();
void setup() {
    //Hardware init
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
#ifdef AD_NOSCREEN    
//    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
#endif
    Serial.setRxBufferSize(2048);
    Serial.begin(115200);
    Serial.setDebugOutput(false);
    EEPROM.begin(256);

    FS.begin(true);

    uint32_t seed = esp_random();
    DEBUG_MSG("Setting random seed %u\n", seed);
    randomSeed(seed); // ESP docs say this is fairly random

    DEBUG_MSG("CPU Clock: %d\n", getCpuFrequencyMhz());
    DEBUG_MSG("Total heap: %d\n", ESP.getHeapSize());
    DEBUG_MSG("Free heap: %d\n", ESP.getFreeHeap());
    DEBUG_MSG("Total PSRAM: %d\n", ESP.getPsramSize());
    DEBUG_MSG("Free PSRAM: %d\n", ESP.getFreePsram());


    auto res = esp_task_wdt_init(APP_WATCHDOG_SECS_INIT, true);
    assert(res == ESP_OK);

    res = esp_task_wdt_add(NULL);
    assert(res == ESP_OK);

    esp_vfs_spiffs_conf_t spiffs_1 = {
      .base_path = "/spiffs",
      .partition_label = "spiffs",
      .max_files = 32,
      .format_if_mount_failed = true
    };

    esp_vfs_spiffs_conf_t spiffs_2 = {
      .base_path = "/config",
      .partition_label = "config",
      .max_files = 32,
      .format_if_mount_failed = true
    };  
    SPIFFS.end();
    if(esp_vfs_spiffs_register(&spiffs_1)!=ESP_OK || esp_vfs_spiffs_register(&spiffs_2)!=ESP_OK){
        DEBUG_MSG("An Error has occurred while mounting SPIFFS\n");
    }

    size_t total = 0;
    size_t used = 0;
    if(esp_spiffs_info(spiffs_1.partition_label, &total, &used) == ESP_OK)
    {
        DEBUG_MSG("SPIFFS %s used %d, total %d\n",spiffs_1.partition_label, used, total);
    }
    if(esp_spiffs_info(spiffs_2.partition_label, &total, &used) == ESP_OK)
    {
        DEBUG_MSG("SPIFFS %s used %d, total %d\n",spiffs_2.partition_label, used, total);
    }

    global = new cAirsoftTracker();
    global->init();
    esp_task_wdt_delete(NULL);
}

void loop() 
{
   delay(1000);
}