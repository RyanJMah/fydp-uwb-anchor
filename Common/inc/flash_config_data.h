#pragma once

#include "sdk_errors.h"

#include "lan.h"
#include "flash_memory_map.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define NUM_FALLACK_SERVERS     ( 10 )

/*************************************************************
 * TYPE DEFINITIONS
 ************************************************************/

// IMPORTANT: Modify ./Scripts/generate_flash_config_bin.py if you change this struct.

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
     * It's a int32_t, so the flash will likely physically fail before we overflow it.
     */
    uint32_t swap_count;

    uint8_t fw_update_pending;  // signal to bootloader that we want to update the firmware

    uint8_t anchor_id;

    uint32_t socket_recv_timeout_ms;

    uint8_t mac_addr[6];
    uint8_t using_dhcp;

    ipv4_addr_t static_ip_addr;
    ipv4_addr_t static_netmask;
    ipv4_addr_t static_gateway;

    /*
     * Hostname, IP address, and port of server, will try the 0th
     * one first, then the 1st, etc.
     */
    hostname_t  server_hostname[10];
    ipv4_addr_t server_ip_addr[10];
    uint32_t    server_port[10];

    // CRC32 of this struct, not including this field
    uint32_t crc32;
} FlashConfigData_t;


/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
extern FlashConfigData_t* gp_persistent_conf;

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
ret_code_t FlashConfigData_Init(void);
uint8_t FlashConfigData_IsInitalized(void);

// Only ever used by bootloader right before jumping to app code
ret_code_t FlashConfigData_Deinit(void);

// Returns 1 if the data is valid, 0 otherwise
uint8_t FlashConfigData_Validate(void);

void FlashConfigData_Print(void);

ret_code_t FlashConfigData_WriteBack(void);

ret_code_t FlashConfigData_RestoreFromSwap(void);
