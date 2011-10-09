/*
 * [origin Linux: arch/arm/mach-at91/include/mach/board.h]
 *
 *  Copyright (C) 2005 HP Labs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ASM_ARCH_BOARD_H
#define __ASM_ARCH_BOARD_H

#include <net.h>
#include <spi/spi.h>
#include <linux/mtd/mtd.h>

 /* USB Host */
struct at91_usbh_data {
	u8		ports;		/* number of ports on root hub */
	u8		vbus_pin[2];	/* port power-control pin */
};
extern void __init at91_add_device_usbh_ohci(struct at91_usbh_data *data);

void atmel_nand_load_image(void *dest, int size, int pagesize, int blocksize);

 /* NAND / SmartMedia */
struct atmel_nand_data {
	u8		enable_pin;	/* chip enable */
	u8		det_pin;	/* card detect */
	u8		rdy_pin;	/* ready/busy */
	u8		ale;		/* address line number connected to ALE */
	u8		cle;		/* address line number connected to CLE */
	u8		bus_width_16;	/* buswidth is 16 bit */
	u8		ecc_mode;	/* NAND_ECC_* */
};

void at91_add_device_nand(struct atmel_nand_data *data);

 /* Ethernet (EMAC & MACB) */
#define AT91SAM_ETHER_MII		(0 << 0)
#define AT91SAM_ETHER_RMII		(1 << 0)
#define AT91SAM_ETHER_FORCE_LINK	(1 << 1)

struct at91_ether_platform_data {
	unsigned int flags;
	int phy_addr;
	int (*get_ethaddr)(struct eth_device*, unsigned char *adr);
};

void at91_add_device_eth(struct at91_ether_platform_data *data);

/* SDRAM */
void at91_add_device_sdram(u32 size);

 /* Serial */
#define ATMEL_UART_CTS	0x01
#define ATMEL_UART_RTS	0x02
#define ATMEL_UART_DSR	0x04
#define ATMEL_UART_DTR	0x08
#define ATMEL_UART_DCD	0x10
#define ATMEL_UART_RI	0x20

void at91_register_uart(unsigned id, unsigned pins);

/* Multimedia Card Interface */
struct atmel_mci_platform_data {
	unsigned bus_width;
	unsigned host_caps; /* MCI_MODE_* from mci.h */
	unsigned detect_pin;
	unsigned wp_pin;
};

void at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data);

/* SPI Master platform data */
struct at91_spi_platform_data {
	int *chipselect;	/* array of gpio_pins */
	int num_chipselect;	/* chipselect array entry count */
};

void at91_add_device_spi(int spi_id, struct at91_spi_platform_data *pdata);
#endif
