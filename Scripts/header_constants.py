# Generated by h2py from stdin
NUM_FALLBACK_SERVERS = ( 10 )

# Generated by h2py from stdin
MCU_FLASH_SIZE = ( 512 * 1024 )
FLASH_PAGE_SIZE = ( 4096 )
FLASH_SOFTDEVICE_SIZE = ( 28 * FLASH_PAGE_SIZE )
FLASH_CONFIG_DATA_SIZE = ( 2 * FLASH_PAGE_SIZE )
FLASH_BOOTLOADER_SIZE = ( 9 * FLASH_PAGE_SIZE )
FLASH_MBR_STORAGE_SIZE = ( 1 * FLASH_PAGE_SIZE )
FLASH_APP_SIZE = ( MCU_FLASH_SIZE - FLASH_BOOTLOADER_SIZE - FLASH_CONFIG_DATA_SIZE - FLASH_SOFTDEVICE_SIZE - FLASH_MBR_STORAGE_SIZE )
FLASH_APP_START_ADDR = ( 0x0001c000 )
FLASH_APP_END_ADDR = ( FLASH_APP_START_ADDR + FLASH_APP_SIZE - 1 )
FLASH_CONFIG_DATA_START_ADDR = ( FLASH_APP_END_ADDR + 1 )
FLASH_CONFIG_DATA_END_ADDR = ( FLASH_CONFIG_DATA_START_ADDR + FLASH_CONFIG_DATA_SIZE - 1 )
FLASH_BOOTLOADER_START_ADDR = ( FLASH_CONFIG_DATA_END_ADDR + 1 )
FLASH_BOOTLOADER_END_ADDR = ( FLASH_BOOTLOADER_START_ADDR + FLASH_BOOTLOADER_SIZE - 1 )
FLASH_MBR_STORAGE_START_ADDR = ( FLASH_BOOTLOADER_END_ADDR + 1 )
FLASH_MBR_STORAGE_END_ADDR = ( FLASH_MBR_STORAGE_START_ADDR + FLASH_MBR_STORAGE_SIZE - 1 )

# Generated by h2py from stdin
W5500_NUM_SOCKETS = ( 8 )
MQTT_SOCK_NUM = ( 1 )
MDNS_SOCK_NUM = ( 2 )
DHCP_SOCK_NUM = ( 3 )
DFU_SOCK_NUM = ( 4 )
Skipping: MAX_HOSTNAME_CHARS = ( FlashDataConfig_FieldLen_Hostname_len )

