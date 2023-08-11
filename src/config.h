#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <freertos\FreeRTOS.h>
#include <freertos\semphr.h>

#define ARDUINOJSON_USE_DOUBLE 1

#define DEBUG_PORT Serial // Serial debug port
#ifdef DEBUG_PORT
extern SemaphoreHandle_t debugLock;
#define DEBUG_MSG(...) ({xSemaphoreTake(debugLock, portMAX_DELAY); DEBUG_PORT.printf(__VA_ARGS__); xSemaphoreGive(debugLock);})
#define LOCK_DEBUG_PORT() (xSemaphoreTake(debugLock, portMAX_DELAY))
#define UNLOCK_DEBUG_PORT() (xSemaphoreGive(debugLock))
#else
#define DEBUG_MSG(...)
#define LOCK_DEBUG_PORT()
#define UNLOCK_DEBUG_PORT()
#endif

#define xstr(s) ssstr(s)
#define ssstr(s) #s

//AD_V100 - Lora RF95
//#define AD_V100

//AD_V100 - Lora SX1262
//#define AD_V101

#define APP_WATCHDOG_SECS_INIT 70
#define APP_WATCHDOG_SECS_WORK 15

#define GPS_BAUDRATE 9600
#define GPS_BAUDRATE2 115200
#define GPS_FREQ 2
#define LORA_SEND_LOCATION_PERIOD 10000

#define AC_WIRE Wire1
#define AC_ADDRESS 0x68

#define FLIP_SCREEN_VERTICALLY
#define FS SPIFFS
#define SCK_GPIO 5
#define MISO_GPIO 19
#define MOSI_GPIO 27
#define NSS_GPIO 18 
#define RESET_GPIO 14

#define GPS_SERIAL_NUM 1
#define GPS_RX_PIN 34
#define GPS_TX_PIN 12

#define RF95_IRQ_GPIO 26
#define PMU_IRQ 35
#define SSD1306_ADDRESS 0x3C

#define I2C_SDA 21
#define I2C_SCL 22

#define I2C2_SDA 0
#define I2C2_SCL 4

#define BUTTON_PIN 38     // The middle button GPIO on the T-Beam
#define VIBRO_PIN   7
#define VIBRO_PIN2  3

#define SX1262_E22

#define SX1262_IRQ  33
#define SX1262_RST  23
#define SX1262_BUSY 32
#define SX1262_MAXPOWER 22

#define LORA_BW 500.0
#define LORA_SF 8
#define LORA_CR 7
#define LORA_FREQ 912.0
#define LORA_PRE 8
#define LORA_SYNC 0x64
#define LORA_POWER 17

#define AGD_LORA_BW 125.0
#define AGD_LORA_SF 10
#define AGD_LORA_CR 7
#define AGD_LORA_FREQ 915.0
#define AGD_LORA_PRE 8
#define AGD_LORA_SYNC 0x31

#ifdef AD_V100
#undef  USE_SX1262
#define HAS_AXP20X 1
#define HAS_GPS 1
#endif

#ifdef AD_V101
#define USE_SX1262
#undef  LORA_POWER
#define LORA_POWER SX1262_MAXPOWER
#define HAS_AXP20X 1
#define HAS_GPS 1
#define LORA_CURRENT 140
#endif

#ifdef AD_LORA32_V1
#undef USE_SX1262
#undef PMU_IRQ
#define HAS_GPS
#define AT_CAN_SLEEP
#undef  GPS_FREQ
#define GPS_FREQ 1

#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_CS 18
#define LORA_RST 14
#define LORA_IRQ 26
#define LORA_CURRENT 240

#define BATTERY_PIN 39
#define LED_PIN -1
#undef  I2C_SCL
#define I2C_SCL 19
#endif

#ifdef  AD_NOSCREEN
#define AD_BLE_NO_CLIENT 
#endif

#define SPI_DISP_SCK    2
#define SPI_DISP_MISO   13
#define SPI_DISP_MOSI   14
#define SPI_DISP_CS     25
#define SPI_DISP_RESET  4

#ifndef AD_NOSCREEN
#define USE_SCREEN_EVE   1
#endif

#define DW1000_IRQ      15
#define DW1000_RST      4
#define DW1000_CS       5

#define PSENSOR_PIN     36

#define BTN1_PIN    0
#define BTN2_PIN    2
#define BTN3_PIN    1
#define BTN4_PIN    15
#define BTN5_PIN    6
#define BTN6_PIN    14

#define SIDE_BUTTONS_COUNT  6

#define GNSS_DBD_FILENAME "/config/gnss.dat"
#define MAIN_CONFIG_FILENAME "/config/main.json"
#define NET_CONFIG_FILENAME "/config/network.json"
#define AGD_NET_CONFIG_FILENAME "/config/agdnet.json"

#define DEFAULT_TZ "EST5EDT,M3.2.0,M11.1.0"

//Packet Prefix
#define  SERIAL_APP_1  0x85
#define  SERIAL_APP_2  0x3B
#define  SERIAL_APP_3  0xDE
#define  SERIAL_APP_4  0x02
#define  MAX_SERIAL_PACKET_SIZE 4200

#define  SERIAL_PROG_MODE_SIG1  0x92
#define  SERIAL_PROG_MODE_SIG2  0x5A

#define  SERIAL_PROG_MODE_RESP  0x07

#define  SERIAL_PROG Serial
#define  SCREEN_FLASH_META_FILE "/spiffs/sflash.json"
#define  SCREEN_FLASH_META_BINARY_FILE "/spiffs/sflash.dat"
#define  SCREEN_FLASH_META_FILE_PATH "/spiffs/"
#define  AT_POINTS_FILE_TEMPLATE "/spiffs/p_%s.json"

#endif