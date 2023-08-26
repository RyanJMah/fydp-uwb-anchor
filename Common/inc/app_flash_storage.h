#pragma once

#include "sdk_errors.h"
#include "flash_memory_map.h"

/*************************************************************
 * TYPE DEFINITIONS
 ************************************************************/
typedef struct __attribute__((packed))
{
    /*
     * The program will load this struct into RAM from FLASH at startup.
     *
     * When we want to write-back to FLASH, we will write the entire struct
     * to the swap page.
     *
     * Before writing, the swap_count field will be incremented.
     *
     * The page with the highest swap_count will be the considered the
     * "active" page, and the other page will be considered the "swap" page.
     *
     * It's a uint32_t, so the flash will likely physically fail before we overflow it.
     */
    uint32_t swap_count;

    uint8_t fw_update_pending;  // signal to bootloader that we want to update the firmware

    uint8_t anchor_id;

    uint8_t mac_addr[6];
    uint8_t using_dhcp;


    uint32_t crc32;             // CRC32 of the entire struct, not including this field
} AppFlashStorage_t;


/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
extern AppFlashStorage_t g_config;

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
ret_code_t AppFlashStorage_Init(void);

ret_code_t AppFlashStorage_WriteBack(void);

ret_code_t AppFlashStorage_RestoreFromSwap(void);
