/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Network initialization for MCF V4E FEC support code
 * @todo Obsolete this file
 */
#include <common.h>
#include <malloc.h>
#include <linux/types.h>

#include <mach/mcf54xx-regs.h>
#include <proc/mcdapi/MCD_dma.h>
#include <proc/net/net.h>
#include <proc/fecbd.h>
#include <proc/fec.h>
#include <proc/dma_utils.h>

#include <proc/processor.h> //FIXME - move to other file

int netif_init(int channel)
{
	uint8_t* board_get_ethaddr(uint8_t*);

#ifdef CONFIG_USE_IRQ
	int vector;
	int (*handler)(void *, void *);

	disable_interrupts();

	/*
	 * Register the FEC0 interrupt handler
	 */
	handler = (channel == 0) ? fec0_interrupt_handler
	                         : fec1_interrupt_handler;
	vector =  (channel == 0) ? 103 : 102;

	if (!mcf_interrupts_register_handler(
		vector,handler, NULL,(void *)0xdeadbeef))
	{
	    printf("Error: Unable to register handler\n");
	    return 0;
	}

	/*
	 * Register the DMA interrupt handler
	 */
	handler = dma_interrupt_handler;
	vector = 112;

	if (!mcf_interrupts_register_handler(
		vector,handler, NULL,NULL))
	{
	    printf("Error: Unable to register handler\n");
	    return 0;
	}
#endif
	/*
	 * Enable interrupts
	 */
	enable_interrupts();

	return 1;
}

int netif_setup(int channel)
{
	uint8_t mac[6];
	/*
	 * Get user programmed MAC address
	 */
//	board_get_ethaddr(mac);


	/*
	 * Initialize the network interface structure
	 */
//	nif_init(&nif1);
//	nif1.mtu = ETH_MTU;
//	nif1.send = (DBUG_ETHERNET_PORT == 0) ? fec0_send : fec1_send;

	/*
	 * Initialize the dBUG Ethernet port
	 */
	fec_eth_setup(channel,        /* Which FEC to use */
	              FEC_MODE_MII,         /* Use MII mode */
	              FEC_MII_100BASE_TX,   /* Allow 10 and 100Mbps */
	              FEC_MII_FULL_DUPLEX,  /* Allow Full and Half Duplex */
	              mac);

	/*
	 * Copy the Ethernet address to the NIF structure
	 */
//	memcpy(nif1.hwa, mac, 6);

	#ifdef DEBUG
	    printf("Ethernet Address is %02X:%02X:%02X:%02X:%02X:%02X\n",\
	            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	#endif

	return 1;
}

int netif_done(int channel)
{
	/*
	 * Download complete, clean up
	 */
#ifdef CONFIG_USE_IRQ
    	int (*handler)(void *, void *);
#endif
	/*
	 * Disable interrupts
	 */
	disable_interrupts();

	/*
	 * Disable the Instruction Cache
	 */
	mcf5xxx_wr_cacr(MCF5XXX_CACR_ICINVA);

	/*
	 * Disable the dBUG Ethernet port
	 */
	fec_eth_stop(channel);

	/*
	 * Remove the interrupt handlers
	 */
#ifdef CONFIG_USE_IRQ
	handler = (channel == 0) ? fec0_interrupt_handler
		                 : fec1_interrupt_handler;
	mcf_interrupts_remove_handler(handler);
	mcf_interrupts_remove_handler(dma_interrupt_handler);
#endif
	return 1;
}


