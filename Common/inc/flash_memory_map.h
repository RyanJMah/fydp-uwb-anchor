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
 *  |          Bootloader          |    20 kB   (0x0000 0000 - 0x0000 4FFF)
 *  |          (5 pages)           |
 *  |                              |
 *  --------------------------------
 *  |                              |
 *  |   Application Data Storage   |    8 kB    (0x0000 5000 - 0x0000 6FFF)
 *  |         (2 pages)            |
 *  |                              |
 *  --------------------------------
 *  |                              |
 *  |         Application          |    484 kB  (0x0000 7000 - 0x0007 FFFF)
 *  |         ( pages)             |
 *  |                              |
 *  --------------------------------   -------
 *                               Total: 512 kB
 */

#define FLASH_BOOTLOADER_SIZE           ( 10 * FLASH_PAGE_SIZE )
#define FLASH_APP_DATA_SIZE             ( 2 * FLASH_PAGE_SIZE )
#define FLASH_APP_SIZE                  ( MCU_FLASH_SIZE - FLASH_BOOTLOADER_SIZE - FLASH_APP_DATA_SIZE )

#define FLASH_BOOTLOADER_START_ADDR     ( 0x00000000 )
#define FLASH_BOOTLOADER_END_ADDR       ( FLASH_BOOTLOADER_START_ADDR + FLASH_BOOTLOADER_SIZE - 1 )

#define FLASH_APP_DATA_START_ADDR       ( FLASH_BOOTLOADER_END_ADDR + 1 )
#define FLASH_APP_DATA_END_ADDR         ( FLASH_APP_DATA_START_ADDR + FLASH_APP_DATA_SIZE - 1 )

#define FLASH_APP_START_ADDR            ( FLASH_APP_DATA_END_ADDR + 1 )
#define FLASH_APP_END_ADDR              ( FLASH_APP_START_ADDR + FLASH_APP_SIZE - 1 )

// Check if sizes are multiples of page size

#if ( FLASH_BOOTLOADER_SIZE % FLASH_PAGE_SIZE ) != 0
#error "FLASH_BOOTLOADER_SIZE is not a multiple of FLASH_PAGE_SIZE"
#endif


#if ( FLASH_APP_DATA_SIZE % FLASH_PAGE_SIZE ) != 0
#error "FLASH_APP_DATA_SIZE is not a multiple of FLASH_PAGE_SIZE"
#endif

#if ( FLASH_APP_SIZE % FLASH_PAGE_SIZE ) != 0
#error "FLASH_APP_SIZE is not a multiple of FLASH_PAGE_SIZE"
#endif

// Make sure that we are using all of the flash
#if ( FLASH_BOOTLOADER_SIZE + FLASH_APP_DATA_SIZE + FLASH_APP_SIZE ) != MCU_FLASH_SIZE
#error "wasted flash space in memory map"
#endif

// Make sure that the bootloader and app data are not overlapping
#if !( FLASH_BOOTLOADER_END_ADDR < FLASH_APP_DATA_START_ADDR )
#error "FLASH_BOOTLOADER_END_ADDR overlaps FLASH_APP_DATA_START_ADDR"
#endif

// Make sure that the app data and app are not overlapping
#if !( FLASH_APP_DATA_END_ADDR < FLASH_APP_START_ADDR )
#error "FLASH_APP_DATA_END_ADDR overlaps FLASH_APP_START_ADDR"
#endif

// Make sure that the app is not overlapping the bootloader
#if !( FLASH_APP_START_ADDR > FLASH_BOOTLOADER_END_ADDR )
#error "FLASH_APP_START_ADDR overlaps FLASH_BOOTLOADER_END_ADDR"
#endif

// Make sure that the app is not overlapping the app data
#if !( FLASH_APP_START_ADDR > FLASH_APP_DATA_END_ADDR )
#error "FLASH_APP_START_ADDR overlaps FLASH_APP_DATA_END_ADDR"
#endif

// Make sure that the app is not overlapping the end of flash
#if !( FLASH_APP_END_ADDR < MCU_FLASH_SIZE )
#error "FLASH_APP_END_ADDR overlaps MCU_FLASH_SIZE"
#endif



