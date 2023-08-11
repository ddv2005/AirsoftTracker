esptool.exe -b 921600 read_flash 0x33F000 0x0C1000 spiffs.img
mkspiffs.exe -u spiffs spiffs.img 