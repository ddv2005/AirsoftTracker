#pragma once
#include <stdint.h>

#pragma pack(push, 1)

#define PMC_SCREEN_FLASH_START  1
#define PMC_SCREEN_FLASH_START_RESPONSE 2
#define PMC_SCREEN_FLASH_END    3
#define PMC_SCREEN_FLASH_WRITE  4
#define PMC_SCREEN_FLASH_WRITE_COMPRESS  14
#define PMC_SCREEN_FLASH_READ   5
#define PMC_SCREEN_FLASH_READ_RESPONSE   6
#define PMC_SCREEN_FLASH_META_START 7
#define PMC_SCREEN_FLASH_META_DATA  8
#define PMC_SCREEN_FLASH_META_END   9
#define PMC_SCREEN_FLASH_FILE_START 10
#define PMC_SCREEN_FLASH_FILE_DATA  11
#define PMC_SCREEN_FLASH_FILE_END   12
#define PMC_SCREEN_FLASH_EXIT   99
#define PMC_SCREEN_FLASH_ACK    100
#define PMC_SCREEN_FLASH_ERR    101

struct sPMCFlashBlock
{
    uint16_t size;
    uint32_t address;
};

#pragma pack(pop)