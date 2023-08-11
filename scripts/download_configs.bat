esptool.exe -b 921600 read_flash 0x330000 61440 config.img
mkspiffs.exe -u config config.img 