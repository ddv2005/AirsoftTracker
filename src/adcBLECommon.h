#pragma once

#define SERVICE_UUID_ADC              "e9f2ee69-ee2e-49d2-a763-458db734b546"
#define CHARACTERISTIC_UUID_ADC_POSITION_REPORT              "697f4ebc-4a7e-4782-b15e-31dbee61b3e2"


#pragma pack(push, 1)
struct sADCPositionReport
{
    unsigned long now;
    float latitude;
    float longitude;
    float altitude;
    bool  hasLock;
    int   sat;
    unsigned long lastSeen;
    float SNR;
    float RSSI;
    char  status[32];
    uint32_t hacc;
};
#pragma pack(pop)