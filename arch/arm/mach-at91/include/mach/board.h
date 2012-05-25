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

#include <mach/hardware.h>
#include <sizes.h>
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

 /* USB Device */
struct at91_udc_data {
	int	vbus_pin;		/* high == host powering us */
	u8	vbus_active_low;	/* vbus polarity */
	u8	vbus_polled;		/* Use polling, not interrupt */
	int	pullup_pin;		/* active == D+ pulled up */
	u8	pullup_active_low;	/* true == pullup_pin is active low */
};
extern void __init at91_add_device_udc(struct at91_udc_data *data);

 /* NAND / SmartMedia */
struct atmel_nand_data {
	u8		enable_pin;	/* chip enable */
	u8		det_pin;	/* card detect */
	u8		rdy_pin;	/* ready/busy */
	u8		ale;		/* address line number connected to ALE */
	u8		cle;		/* address line number connected to CLE */
	u8		bus_width_16;	/* buswidth is 16 bit */
	u8		ecc_mode;	/* NAND_ECC_* */
	u8		on_flash_bbt;	/* Use flash based bbt */
};

void at91_add_device_nand(struct atmel_nand_data *data);

 /* Ethernet (EMAC & MACB) */
#define AT91SAM_ETHER_MII		(0 << 0)
#define AT91SAM_ETHER_RMII		(1 << 0)
#define AT91SAM_ETHER_FORCE_LINK	(1 << 1)
#define AT91SAM_ETX2_ETX3_ALTERNATIVE	(1 << 2)

struct at91_ether_platform_data {
	unsigned int flags;
	int phy_addr;
	int (*get_ethaddr)(struct eth_device*, unsigned char *adr);
};

void at91_add_device_eth(int id, struct at91_ether_platform_data *data);

/* SDRAM */
void at91_add_device_sdram(u32 size);

 /* Serial */
#define ATMEL_UART_CTS	0x01
#define ATMEL_UART_RTS	0x02
#define ATMEL_UART_DSR	0x04
#define ATMEL_UART_DTR	0x08
#define ATMEL_UART_DCD	0x10
#define ATMEL_UART_RI	0x20

resource_size_t __init at91_configure_dbgu(void);
resource_size_t __init at91_configure_usart0(unsigned pins);
resource_size_t __init at91_configure_usart1(unsigned pins);
resource_size_t __init at91_configure_usart2(unsigned pins);
resource_size_t __init at91_configure_usart3(unsigned pins);
resource_size_t __init at91_configure_usart4(unsigned pins);
resource_size_t __init at91_configure_usart5(unsigned pins);

#if defined(CONFIG_DRIVER_SERIAL_ATMEL)
static inline struct device_d * at91_register_uart(unsigned id, unsigned pins)
{
	resource_size_t start;
	resource_size_t size = SZ_16K;

	if (id >= AT91_NB_USART)
		return NULL;

	switch (id) {
		case 0:		/* DBGU */
			start = at91_configure_dbgu();
			size = 512;
			break;
		case 1:
			start = at91_configure_usart0(pins);
			break;
		case 2:
			start = at91_configure_usart1(pins);
			break;
		case 3:
			start = at91_configure_usart2(pins);
			break;
		case 4:
			start = at91_configure_usart3(pins);
			break;
		case 5:
			start = at91_configure_usart4(pins);
			break;
		case 6:
			start = at91_configure_usart5(pins);
			break;
		default:
			return NULL;
	}

	return add_generic_device("atmel_usart", id, NULL, start, size,
			   IORESOURCE_MEM, NULL);
}
#else
static inline struct device_d * at91_register_uart(unsigned id, unsigned pins)
{
	return NULL;
}
#endif

/* Multimedia Card Interface */
struct atmel_mci_platform_data {
	unsigned slot_b;
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
