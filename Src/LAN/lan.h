#pragma once

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/

int16_t LAN_Init();

int16_t LAN_Update();

int16_t LAN_Send(uint8_t* data, uint32_t len);

