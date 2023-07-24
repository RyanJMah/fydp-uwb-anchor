/**
 * @file      custom board.h
 * 
 * @brief     pin mapping description corresponding to DWM30001_A1 module
 *
 * @author    Decawave Applications
 *
 * @attention Copyright (c) 2021 - 2022, Qorvo US, Inc.
 * All rights reserved
 * Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this
 *  list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 * 3. You may only use this software, with or without any modification, with an
 *  integrated circuit developed by Qorvo US, Inc. or any of its affiliates
 *  (collectively, "Qorvo"), or any module that contains such integrated circuit.
 * 4. You may not reverse engineer, disassemble, decompile, decode, adapt, or
 *  otherwise attempt to derive or gain access to the source code to any software
 *  distributed under this license in binary or object code form, in whole or in
 *  part.
 * 5. You may not use any Qorvo name, trademarks, service marks, trade dress,
 *  logos, trade names, or other symbols or insignia identifying the source of
 *  Qorvo's products or services, or the names of any of Qorvo's developers to
 *  endorse or promote products derived from this software without specific prior
 *  written permission from Qorvo US, Inc. You must not call products derived from
 *  this software "Qorvo", you must not have "Qorvo" appear in their name, without
 *  the prior permission from Qorvo US, Inc.
 * 6. Qorvo may publish revised or new version of this license from time to time.
 *  No one other than Qorvo US, Inc. has the right to modify the terms applicable
 *  to the software provided under this license.
 * THIS SOFTWARE IS PROVIDED BY QORVO US, INC. "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. NEITHER
 *  QORVO, NOR ANY PERSON ASSOCIATED WITH QORVO MAKES ANY WARRANTY OR
 *  REPRESENTATION WITH RESPECT TO THE COMPLETENESS, SECURITY, RELIABILITY, OR
 *  ACCURACY OF THE SOFTWARE, THAT IT IS ERROR FREE OR THAT ANY DEFECTS WILL BE
 *  CORRECTED, OR THAT THE SOFTWARE WILL OTHERWISE MEET YOUR NEEDS OR EXPECTATIONS.
 * IN NO EVENT SHALL QORVO OR ANYBODY ASSOCIATED WITH QORVO BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 *
 */

#ifndef BOARD_DWM3001_A1_H
#define BOARD_DWM3001_A1_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"
#include "nrf_spim.h"

#define PIN_DISCONNECTED    -1

// LEDs definitions for PCA10056
#define LEDS_NUMBER    2

//If AOA chip is populated on the PCB without second antenna, this shall be set to (1)
#define AOA_CHIP_ON_NON_AOA_PCB     ( 0 )

// We fucked up soldering the LED Pins
#define FUCKED_UP_LEDS              ( 1 )

#if FUCKED_UP_LEDS
    #define LED_1          PIN_DISCONNECTED
    #define LED_2          PIN_DISCONNECTED
    #define LED_START      LED_1
    #define LED_STOP       LED_2
    
    #define LEDS_ACTIVE_STATE 0
    
    #define LEDS_LIST { LED_1, LED_2 }
    
    #define LEDS_INV_MASK  LEDS_MASK
#else
    #define LED_1          NRF_GPIO_PIN_MAP(0, 30)  //D9 on the schematics
    #define LED_2          NRF_GPIO_PIN_MAP(0, 2)  //D10 on the schematics
    #define LED_START      LED_1
    #define LED_STOP       LED_2
    
    #define LEDS_ACTIVE_STATE 0
    
    #define LEDS_LIST { LED_1, LED_2 }
    
    #define LEDS_INV_MASK  LEDS_MASK
#endif

#if FUCKED_UP_LEDS
    #define BSP_LED_0       PIN_DISCONNECTED
    #define BSP_LED_1       PIN_DISCONNECTED
#else
    #define BSP_LED_0      LED_1
    #define BSP_LED_1      LED_2
#endif

#define BSP_LED_2      PIN_DISCONNECTED
#define BSP_LED_3      PIN_DISCONNECTED

#define BUTTONS_NUMBER 1

#define BUTTON_1       NRF_GPIO_PIN_MAP(0, 2)

#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST { BUTTON_1 }

#define BSP_BUTTON_0   BUTTON_1

#define RX_PIN_NUMBER  NRF_GPIO_PIN_MAP(0, 15)
#define TX_PIN_NUMBER  NRF_GPIO_PIN_MAP(0, 19)
#define CTS_PIN_NUMBER (PIN_DISCONNECTED)
#define RTS_PIN_NUMBER (PIN_DISCONNECTED)
#define HWFC           false

#define DW3000_RST_Pin      NRF_GPIO_PIN_MAP(0, 25)
#define DW3000_IRQ_Pin      NRF_GPIO_PIN_MAP(1, 2)
#define DW3000_WUP_Pin      NRF_GPIO_PIN_MAP(1, 19)

// SPI defs
#define DW3000_CS_Pin       NRF_GPIO_PIN_MAP(1, 6)
#define DW3000_CLK_Pin      NRF_GPIO_PIN_MAP(0, 3)      // DWM3000 shield SPIM1 sck connected to DW1000
#define DW3000_MOSI_Pin     NRF_GPIO_PIN_MAP(0, 8)      // DWM3000 shield SPIM1 mosi connected to DW1000
#define DW3000_MISO_Pin     NRF_GPIO_PIN_MAP(0, 29)     // DWM3000 shield SPIM1 miso connected to DW1000

#define DW3000_SPI_IRQ_PRIORITY APP_IRQ_PRIORITY_LOW    // 

// Wiznet defs
#define W5500_SPI_CS_PIN        NRF_GPIO_PIN_MAP(0, 21)
#define W5500_SPI_CLK_PIN       NRF_GPIO_PIN_MAP(0, 4)
#define W5500_SPI_MOSI_PIN      NRF_GPIO_PIN_MAP(0, 14)
#define W5500_SPI_MISO_PIN      NRF_GPIO_PIN_MAP(0, 22)
#define W5500_INTERRUPT_PIN     NRF_GPIO_PIN_MAP(1, 8)
#define W5500_RESET_PIN         NRF_GPIO_PIN_MAP(0, 6)

// UART symbolic constants
#define UART_0_TX_PIN           TX_PIN_NUMBER
#define UART_0_RX_PIN           RX_PIN_NUMBER
#define DW3000_RTS_PIN_NUM      UART_PIN_DISCONNECTED
#define DW3000_CTS_PIN_NUM      UART_PIN_DISCONNECTED

#define LED_ERROR BSP_LED_0

#define DW3000_MIN_SPI_FREQ (NRF_SPIM_FREQ_4M)
#define DW3000_MAX_SPI_FREQ (NRF_SPIM_FREQ_32M)
#define DW3000_SPI_IDX	(3)

#ifdef __cplusplus
}
#endif

#endif // PCA10100_H
