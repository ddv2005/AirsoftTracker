mkspiffs.exe -c .\config -a -s 61440 config.img
esptool.exe -b 921600 write_flash 0x330000 config.img