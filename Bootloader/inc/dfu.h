#pragma once

#include <stdio.h>
#include <stdbool.h>
#include "sdk_errors.h"

#include "flash_memory_map.h"
#include "dfu_messages.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define DFU_PACKET_SIZE     sizeof(DFU_PayloadChunk_t)
#define DFU_SERVER_PORT     ( 6900 )

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
ret_code_t DFU_Init(void);
ret_code_t DFU_Deinit(void);

void DFU_SetUpdateType(DFU_UpdateType_t update_type);

ret_code_t DFU_EraseCurrentImg(void);

bool DFU_ValidateChunk(DFU_ChunkMsg_t* chunk);
ret_code_t DFU_ValidateImage(DFU_MetadataMsg_t* p_metadata, bool* out_ok);

ret_code_t DFU_WriteChunk(DFU_ChunkMsg_t* chunk);

void DFU_JumpToApp(void);


