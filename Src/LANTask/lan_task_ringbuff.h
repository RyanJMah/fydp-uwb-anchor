#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "macros.h"
#include "lan_task.h"

/*
 *
 * Implementation of a fast ringbuffer, size must be a power of two.
 *
 * For example, take size = 8...
 * |
 * |--> 8 - 1 = 7 = 0b111
 *
 *
 * Can just & index with (size-1) and it'll wrap around
 */

/*************************************************************
 * TYPE DEFINITIONS
 ************************************************************/
typedef struct
{
    TelemetryData_t* arr;
    uint32_t arr_size_mask;

    uint32_t head;
    uint32_t tail;

    uint32_t len;
} RingBuff_t;

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/

// size must be a power of two
extern void RingBuff_Init(RingBuff_t* ringbuff, TelemetryData_t* arr, uint32_t arr_size);
ALWAYS_INLINE void RingBuff_Init(RingBuff_t* ringbuff, TelemetryData_t* arr, uint32_t arr_size)
{
    ringbuff->arr = arr;
    ringbuff->arr_size_mask = arr_size - 1;

    ringbuff->head = 0;
    ringbuff->tail = 0;

    ringbuff->len = 0;
}

extern bool RingBuff_IsEmpty(RingBuff_t* ringbuff);
ALWAYS_INLINE bool RingBuff_IsEmpty(RingBuff_t* ringbuff)
{
    return (ringbuff->len == 0);
}

extern void RingBuff_Push(RingBuff_t* ringbuff, struct ranging_measurements* rm);
ALWAYS_INLINE void RingBuff_Push( RingBuff_t* ringbuff,
                                  struct ranging_measurements* rm )
{
    TelemetryData_t* telem_data = &ringbuff->arr[ringbuff->tail];

    telem_data->rssi               = rm->rssi;
    telem_data->status             = rm->status;
    telem_data->distance_mm        = rm->distance_mm;
    telem_data->iphone_aoa_2pi_q16 = rm->remote_aoa_azimuth_2pi;
    telem_data->iphone_aoa_fom     = rm->remote_aoa_azimuth_fom;
    telem_data->anchor_aoa_2pi_q16 = rm->local_aoa_measurements[0].aoa_2pi;
    telem_data->anchor_aoa_fom     = rm->local_aoa_measurements[0].aoa_fom;

    ringbuff->tail = (ringbuff->tail + 1) & ringbuff->arr_size_mask;
    ringbuff->len++;
}

extern int RingBuff_Pop(RingBuff_t* ringbuff, TelemetryData_t* out_data);
ALWAYS_INLINE int RingBuff_Pop(RingBuff_t* ringbuff, TelemetryData_t* out_data)
{
    if ( RingBuff_IsEmpty(ringbuff) )
    {
        return -1;
    }

    memcpy( &ringbuff->arr[ringbuff->head], out_data, sizeof(TelemetryData_t) );

    ringbuff->head = (ringbuff->head + 1) & ringbuff->arr_size_mask;
    ringbuff->len--;

    return 0;
}
