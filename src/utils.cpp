#include <Arduino.h>
#include "utils.h"

void getMacAddr(uint8_t *dmac)
{
    assert(esp_efuse_mac_get_default(dmac) == ESP_OK);
}

const char *getDeviceName()
{
    uint8_t dmac[6];

    getMacAddr(dmac);

    static char name[20];
    sprintf(name, "BOMB_%02x%02x", dmac[4], dmac[5]);
    return name;
}
