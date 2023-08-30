#pragma once

#include <stdio.h>
#include "sdk_errors.h"

#include "flash_memory_map.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define DFU_PACKET_SIZE     sizeof(DFU_PayloadChunk_t)
#define DFU_SERVER_PORT     ( 6900 )

/*************************************************************
 * TYPE DEFINITIONS
 ************************************************************/
typedef struct __attribute__((packed))
{
    uint32_t crc32;
    uint32_t payload_num_bytes;
    uint8_t  update_provisioning_data;
} DFU_Metadata_t;

typedef struct __attribute__((packed))
{
    uint32_t chunk_num;
    uint8_t  payload[FLASH_PAGE_SIZE];
    uint32_t crc32;
} DFU_PayloadChunk_t;

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
ret_code_t DFU_Init(void);

ret_code_t DFU_EraseAppCode(void);

ret_code_t DFU_WriteChunk(DFU_PayloadChunk_t* chunk);

void DFU_RegisterMetadata(DFU_Metadata_t* metadata);

void DFU_JumpToApp(void);


