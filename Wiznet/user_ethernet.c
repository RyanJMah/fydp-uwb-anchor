#include "user_ethernet.h"
#include "gl_error.h"
#include "user_spi.h"
#include "socket.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "gl_log.h"
#include "custom_board.h"

///////////////////////////////////
// Default Network Configuration //
///////////////////////////////////

void wizchip_select(void)
{
	nrf_gpio_pin_clear(W5500_SPI_CS_PIN);
}

void wizchip_deselect(void)
{
	nrf_gpio_pin_set(W5500_SPI_CS_PIN);
}


uint8_t wizchip_read()
{
	uint8_t recv_data;

	spi_master_rx(SPI1,1,&recv_data);

	return recv_data;
}

void wizchip_write(uint8_t wb)
{
	spi_master_tx(SPI1, 1, &wb);
}


void user_ethernet_init(void)
{
    uint8_t tmp;
	uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2}};

    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
    reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);

    /* WIZCHIP SOCKET Buffer initialize */
	
    GL_LOG("W5500 memory init\r\n");

    if(ctlwizchip(CW_INIT_WIZCHIP,(void*)memsize) == -1)
    {
    	GL_LOG("WIZCHIP Initialized fail.\r\n");
        GL_FATAL_ERROR();
    }

    /* PHY link status check */
    GL_LOG("W5500 PHY Link Status Check\r\n");
    do
    {
        if(ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1)
        {
            GL_LOG("Unknown PHY Link stauts.\r\n");
        }
    }
    while(tmp == PHY_LINK_OFF);
}
