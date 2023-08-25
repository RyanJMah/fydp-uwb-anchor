#include "user_ethernet.h"
#include "user_spi.h"
#include "socket.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "gl_log.h"
#include "custom_board.h"

#include "anchor_config.h"

///////////////////////////////////
// Default Network Configuration //
///////////////////////////////////

wiz_NetInfo gWIZNETINFO =
{
    .mac = ANCHOR_MAC_ADDR,
    .sn = ANCHOR_LAN_SUBNET_MASK,
    .gw   = ANCHOR_LAN_GW_ADDR,

#if !ANCHOR_LAN_USING_DHCP
    .ip = ANCHOR_LAN_IP_ADDR,
#endif

#if ANCHOR_USE_DHCP
    .dhcp = NETINFO_DHCP
#else
    .dhcp = NETINFO_STATIC
#endif
};

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


void user_ethernet_init()
{
    uint8_t tmp;
	  uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2}};
		wiz_NetTimeout timeout_info;

    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
    reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);

    /* WIZCHIP SOCKET Buffer initialize */
	
    GL_LOG("W5500 memory init\r\n");

    if(ctlwizchip(CW_INIT_WIZCHIP,(void*)memsize) == -1)
    {
    	GL_LOG("WIZCHIP Initialized fail.\r\n");
       while(1);
    }

    /* PHY link status check */
    GL_LOG("W5500 PHY Link Status Check\r\n");
    do
    {
       if(ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1)
    	   GL_LOG("Unknown PHY Link stauts.\r\n");
    }while(tmp == PHY_LINK_OFF);

    timeout_info.retry_cnt = 1;
    timeout_info.time_100us = 0x3E8;	// timeout value = 10ms

    wizchip_settimeout(&timeout_info);
    /* Network initialization */
    network_init();
}

void network_init(void)
{
  uint8_t tmpstr[6];
	ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);
	ctlnetwork(CN_GET_NETINFO, (void*)&gWIZNETINFO);

	// Display Network Information
	ctlwizchip(CW_GET_ID,(void*)tmpstr);
	GL_LOG("\r\n=== %s NET CONF ===\r\n",(char*)tmpstr);
	GL_LOG("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",gWIZNETINFO.mac[0],gWIZNETINFO.mac[1],gWIZNETINFO.mac[2],
		  gWIZNETINFO.mac[3],gWIZNETINFO.mac[4],gWIZNETINFO.mac[5]);
	GL_LOG("SIP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0],gWIZNETINFO.ip[1],gWIZNETINFO.ip[2],gWIZNETINFO.ip[3]);
	GL_LOG("GAR: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0],gWIZNETINFO.gw[1],gWIZNETINFO.gw[2],gWIZNETINFO.gw[3]);
	GL_LOG("SUB: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0],gWIZNETINFO.sn[1],gWIZNETINFO.sn[2],gWIZNETINFO.sn[3]);
	GL_LOG("DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0],gWIZNETINFO.dns[1],gWIZNETINFO.dns[2],gWIZNETINFO.dns[3]);
	GL_LOG("======================\r\n");
}
