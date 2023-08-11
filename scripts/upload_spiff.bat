mkspiffs.exe -c .\spiffs -a -s 0x0C1000 spiffs.img
esptool.exe -b 921600 write_flash 0x33F000 spiffs.img