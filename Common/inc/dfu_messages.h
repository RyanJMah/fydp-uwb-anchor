#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "flash_memory_map.h"

/*
 * Sequence diagram:
 * 
 *       Anchor                             Server
 *         |                                  |
 *  (running app code)                        |
 *         |                                  |
 *         |<------------- REQ ---------------|
 *         |                                  |
 * (running bootloader)                       |
 *         |                                  |
 *         |------------- READY ------------->|
 *         |                                  |
 *         |                                  |
 *         |<----------- METADATA ------------|
 *         |                                  |
 *         |                                  |
 *         |------------- BEGIN ------------->|
 *         |                                  |
 *         |                                  |
 *         |<----------- CHUNK 0 -------------|
 *         |                                  |
 *         |-------------- OK --------------->|
 *         |                                  |
 *         |             ........             |
 *         |                                  |
 *         |<---------- CHUNK N-1 ------------|
 *         |                                  |
 *         |-------------- OK --------------->|
 *         |                                  |
 *         |                                  |
 *         |------------ CONFIRM ------------>|
 *         |                                  |
 *  (running app code)                     (exit)
 */

/*************************************************************
 * MACROS
 ************************************************************/

/*
 * Making these #defines so they get picked up by ./Scripts/h2py.py
 *
 * Less code duplication, and we can use the same
 * names in the Python code.
 */

#define DFU_SERVER_PORT     ( 6900 )

#define DFU_MSG_TYPE_REQ        ( 0x00 )
#define DFU_MSG_TYPE_READY      ( 0x01 )
#define DFU_MSG_TYPE_METADATA   ( 0x02 )
#define DFU_MSG_TYPE_BEGIN      ( 0x03 )
#define DFU_MSG_TYPE_CHUNK      ( 0x04 )
#define DFU_MSG_TYPE_OK         ( 0x05 )
#define DFU_MSG_TYPE_CONFIRM    ( 0x06 )
#define DFU_MSG_TYPE_INVALID    ( 0x07 )
#define DFU_MSG_IS_VALID(msg)   ( (msg) < DFU_MSG_TYPE_INVALID )

#define DFU_CHUNK_SIZE          ( FLASH_PAGE_SIZE )

/*************************************************************
 * TYPE DEFINITIONS
 ************************************************************/
typedef uint8_t DFU_MsgType_t;

typedef struct __attribute__((packed))
{
    DFU_MsgType_t msg_type;
} DFU_RequestMsg_t;

typedef struct __attribute__((packed))
{
    DFU_MsgType_t msg_type;
} DFU_ReadyMsg_t;

typedef struct __attribute__((packed))
{
    DFU_MsgType_t msg_type;
    uint32_t      img_crc;
    uint32_t      img_num_chunks;
    uint8_t       update_provisioning_data; // 0 or 1
} DFU_MetadataMsg_t;

typedef struct __attribute__((packed))
{
    DFU_MsgType_t msg_type;
} DFU_BeginMsg_t;

typedef struct __attribute__((packed))
{
    DFU_MsgType_t msg_type;
    uint32_t      chunk_num;
    uint32_t      chunk_num_bytes;
    uint8_t       chunk_data[ DFU_CHUNK_SIZE ];
} DFU_ChunkMsg_t;

typedef struct __attribute__((packed))
{
    DFU_MsgType_t msg_type;
    uint8_t       ok;           // 0 or 1
} DFU_OkMsg_t;

typedef struct __attribute__((packed))
{
    DFU_MsgType_t msg_type;
    uint8_t       ok;           // 0 or 1
} DFU_ConfirmMsg_t;

