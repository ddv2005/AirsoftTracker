#pragma once
#include <stdint.h>

#pragma pack(push, 1)

#define NETWORK_NODE_ID_MAX 255
#define TIMESLOTS_PERIOD    5000
#define TIMESLOTS_MAX       64
#define TIMESLOTS_PER_PEER_MAX 20
#define TIMESLOT_MIN_DURATION (TIMESLOTS_PERIOD/TIMESLOTS_MAX)
#define TIMESLOT_TIMEOUT    30000
#define TIMESLOT_LEARNING_TIME (TIMESLOTS_PERIOD+100)
#define TIMESLOT_MAX_SYMBOLS_LEN 40

//Packet types
#define PT_REPORT   0
#define PT_INFO_REQUEST     1
#define PT_INFO_RESPONSE    2
#define PT_REPORT_REMOTE    3
#define PT_REPORT_REMOTE_D1 4

// 4 bytes
struct sNetworkPacketHeader
{
    uint8_t src;
    uint16_t netId:4;
    uint16_t timeslot:6;
    uint16_t timeslotNext:6;
    uint8_t crc;
};

// 1 byte
struct sNetworkPackeItemHeader
{
    uint8_t type:3;
    uint8_t size:5;
};

#define MAX_NETWORK_USER_NAME    16

struct sNpInfoRequest
{
    uint8_t count; //count of sNpInfoRequestItem
};

struct sNpInfoRequestItem
{
    uint8_t id; // src id
};


struct sNpInfoResponse
{
    char name[MAX_NETWORK_USER_NAME];
    char symbol;
};


// 12 bytes
struct sNpReportItem
{
    int32_t lat:28;
    int32_t lng:28;
    int32_t    alt:15;
    uint32_t   heading:9;
    uint32_t   status:4;
    uint32_t   options:4;
    uint8_t    hacc;
};

#define RID1SL   16
#define RID1SA   8
// 9 bytes
struct sNpReportItemD1
{
    int32_t lat:RID1SL;
    int32_t lng:RID1SL;
    int32_t    alt:RID1SA;
    uint32_t   heading:9;
    uint32_t   status:4;
    uint32_t   options:4;
    uint8_t    hacc;
};

// 6 bytes
struct sNpReportRemoteHeader
{
    uint8_t src;
    union
    {
        // 5 bytes
        struct
        {
            uint8_t flag:1; // 0 - d4, 1 - d3
            uint8_t t0:6;
            uint8_t reserved:1;
            uint8_t t1:4;
            uint8_t t2:4;
            uint8_t t3:4;
            uint8_t t4:4;
            uint8_t t5:4;
            uint8_t t6:4;
            uint8_t t7:4;
            uint8_t t8:4;
            void set(uint8_t idx,uint8_t value)
            {
                switch(idx)
                {
                    case 1: t1 = value;break;
                    case 2: t2 = value;break;
                    case 3: t3 = value;break;
                    case 4: t4 = value;break;
                    case 5: t5 = value;break;
                    case 6: t6 = value;break;
                    case 7: t7 = value;break;
                    case 8: t8 = value;break;
                }
            }
            uint8_t get(uint8_t idx) const
            {
                switch(idx)
                {
                    case 1: return t1;
                    case 2: return t2;
                    case 3: return t3;
                    case 4: return t4;
                    case 5: return t5;
                    case 6: return t6;
                    case 7: return t7;
                    case 8: return t8;
                }
                return 0;
            }
            uint8_t size() const { return 8; }
        }d4;
        struct
        {
            uint8_t flag:1;
            uint8_t t0:6;
            uint8_t t1:3;
            uint8_t t2:3;
            uint8_t t3:3;
            uint8_t t4:3;
            uint8_t t5:3;
            uint8_t t6:3;
            uint8_t t7:3;
            uint8_t t8:3;
            uint8_t t9:3;
            uint8_t t10:3;
            uint8_t t11:3;
            void set(uint8_t idx,uint8_t value)
            {
                switch(idx)
                {
                    case 1: t1 = value;break;
                    case 2: t2 = value;break;
                    case 3: t3 = value;break;
                    case 4: t4 = value;break;
                    case 5: t5 = value;break;
                    case 6: t6 = value;break;
                    case 7: t7 = value;break;
                    case 8: t8 = value;break;
                    case 9: t9 = value;break;
                    case 10: t10 = value;break;
                    case 11: t11 = value;break;
                }
            }
            uint8_t get(uint8_t idx) const
            {
                switch(idx)
                {
                    case 1: return t1;
                    case 2: return t2;
                    case 3: return t3;
                    case 4: return t4;
                    case 5: return t5;
                    case 6: return t6;
                    case 7: return t7;
                    case 8: return t8;
                    case 9: return t9;
                    case 10: return t10;
                    case 11: return t11;
                }
                return 0;
            }
            uint8_t size() const { return 11; }            
        }d3;
    }ts;  
};


#pragma pack(pop)
