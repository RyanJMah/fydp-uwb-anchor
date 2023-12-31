#pragma once

#include "macros.h"

/*************************************************************
 * MACROS
 ************************************************************/

#define MCU_FLASH_SIZE      ( 512 * 1024 )  // 512KB
#define FLASH_PAGE_SIZE     ( 4096 )

/*
 * 
 *  Memory map of ALL flash in MCU looks like this:
 *
 *  --------------------------------
 *  |                              |
 *  |          SoftDevice          |    112 kB  (0x0000 0000 - 0x0001 BFFF)
 *  |          (28 pages)          |
 *  |                              |
 *  --------------------------------
 *  |                              |
 *  |         Application          |    352 kB  (0x0001 C000 - 0x0007 3FFF)
 *  |         (88 pages)           |
 *  |                              |
 *  --------------------------------
 *  |                              |
 *  |      Free Page for FDS       |    4 kB    (0x0007 4000 - 0x0007 4FFF)
 *  |         (1 pages)            |
 *  |                              |
 *  --------------------------------
 *  |                              |
 *  |      Config Data Storage     |    4 kB    (0x0007 5000 - 0x0007 5FFF)
 *  |         (1 pages)            |
 *  |                              |
 *  --------------------------------
 *  |                              |
 *  |          Bootloader          |    36 kB   (0x0007 6000 - 0x0007 EFFF)
 *  |          (9 pages)           |
 *  |                              |
 *  --------------------------------
 *  |                              |
 *  |         MBR Storage          |    4 kB    (0x0007 F000 - 0x0007 FFFF)
 *  |          (1 page)            |
 *  |                              |
 *  --------------------------------   -------
 *                               Total: 512 kB
 */

#define FLASH_SOFTDEVICE_SIZE           ( 28 * FLASH_PAGE_SIZE )
#define FLASH_CONFIG_DATA_SIZE          ( 1 * FLASH_PAGE_SIZE )
#define FLASH_FREE_FDS_PAGE_SIZE        ( 1 * FLASH_PAGE_SIZE )
#define FLASH_BOOTLOADER_SIZE           ( 9 * FLASH_PAGE_SIZE )
#define FLASH_MBR_STORAGE_SIZE          ( 1 * FLASH_PAGE_SIZE )
#define FLASH_APP_SIZE                  ( MCU_FLASH_SIZE - FLASH_MBR_STORAGE_SIZE - FLASH_BOOTLOADER_SIZE - FLASH_CONFIG_DATA_SIZE - FLASH_FREE_FDS_PAGE_SIZE - FLASH_SOFTDEVICE_SIZE )

#define FLASH_APP_START_ADDR            ( 0x0001c000 )
#define FLASH_APP_END_ADDR              ( 0x00073FFF )

#define FLASH_FREE_FDS_PAGE_START_ADDR  ( 0x00074000 )
#define FLASH_FREE_FDS_PAGE_END_ADDR    ( 0x00074FFF )

#define FLASH_CONFIG_DATA_START_ADDR    ( 0x00075000 )
#define FLASH_CONFIG_DATA_END_ADDR      ( 0x00075FFF )

#define FLASH_BOOTLOADER_START_ADDR     ( 0x00076000 )
#define FLASH_BOOTLOADER_END_ADDR       ( 0x0007EFFF )

#define FLASH_MBR_STORAGE_START_ADDR    ( 0x0007F000 )
#define FLASH_MBR_STORAGE_END_ADDR      ( 0x0007FFFF )

// Check if sizes are multiples of page size

#if ( FLASH_BOOTLOADER_SIZE % FLASH_PAGE_SIZE ) != 0
#error "FLASH_BOOTLOADER_SIZE is not a multiple of FLASH_PAGE_SIZE"
#endif

#if ( FLASH_CONFIG_DATA_SIZE % FLASH_PAGE_SIZE ) != 0
#error "FLASH_CONFIG_DATA_SIZE is not a multiple of FLASH_PAGE_SIZE"
#endif

#if ( FLASH_APP_SIZE % FLASH_PAGE_SIZE ) != 0
#error "FLASH_APP_SIZE is not a multiple of FLASH_PAGE_SIZE"
#endif

#if ( FLASH_MBR_STORAGE_SIZE % FLASH_PAGE_SIZE ) != 0
#error "FLASH_MBR_STORAGE_SIZE is not a multiple of FLASH_PAGE_SIZE"
#endif

// Make sure that we are using all of the flash
#if ( FLASH_BOOTLOADER_SIZE + FLASH_CONFIG_DATA_SIZE + FLASH_FREE_FDS_PAGE_SIZE + FLASH_APP_SIZE + FLASH_SOFTDEVICE_SIZE + FLASH_MBR_STORAGE_SIZE ) != MCU_FLASH_SIZE
#error "wasted flash space in memory map"
#endif

// Make sure that the app code is not overlapping the fds page
#if !( FLASH_FREE_FDS_PAGE_START_ADDR > FLASH_APP_END_ADDR )
#error "FLASH_FREE_FDS_PAGE_ADDR overlaps FLASH_APP_END_ADDR"
#endif

// Make sure the config data is not overlapping the fds page
#if !( FLASH_CONFIG_DATA_START_ADDR > FLASH_FREE_FDS_PAGE_END_ADDR )
#error "FLASH_CONFIG_DATA_START_ADDR overlaps FLASH_CONFIG_DATA_END_ADDR"
#endif

// Make sure the config data is not overlapping the bootloader
#if !( FLASH_BOOTLOADER_START_ADDR > FLASH_CONFIG_DATA_END_ADDR )
#error "FLASH_BOOTLOADER_START_ADDR overlaps FLASH_CONFIG_DATA_END_ADDR"
#endif

// Make sure that the MBR storage is not overlapping bootloader
#if !( FLASH_MBR_STORAGE_START_ADDR > FLASH_BOOTLOADER_END_ADDR )
#error "FLASH_MBR_STORAGE_START_ADDR overlaps FLASH_BOOTLOADER_END_ADDR"
#endif

// Make sure MBR is not overlapping end of flash
#if !( FLASH_MBR_STORAGE_END_ADDR < MCU_FLASH_SIZE )
#error "FLASH_MBR_STORAGE_END_ADDR overlaps MCU_FLASH_SIZE"
#endif
