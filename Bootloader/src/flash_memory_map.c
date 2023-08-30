#include "flash_memory_map.h"

/*
 * This variable ensures that the linker script will write the bootloader start address to the UICR
 * register. This value will be written in the HEX file and thus written UICR when the bootloader
 * is flashed into the chip.
 */
// volatile uint32_t m_uicr_bootloader_start_address  __attribute__ ((section(".uicr_bootloader_start_address"))) = FLASH_BOOTLOADER_START_ADDR;
