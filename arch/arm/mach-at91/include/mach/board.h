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
 */

#ifndef __ASM_ARCH_BOARD_H
#define __ASM_ARCH_BOARD_H

#include <mach/hardware.h>
#include <linux/sizes.h>
#include <net.h>
#include <i2c/i2c.h>
#include <spi/spi.h>
#include <linux/mtd/mtd.h>
#include <fb.h>
#include <video/atmel_lcdc.h>
#include <mach/atmel_hlcdc.h>
#include <linux/phy.h>
#include <platform_data/macb.h>

void at91_set_main_clock(unsigned long rate);

#define AT91_MAX_USBH_PORTS	3

 /* USB Host */
struct at91_usbh_data {
	u8		ports;		/* number of ports on root hub */
	int		vbus_pin[AT91_MAX_USBH_PORTS];	/* port power-control pin */
	u8	vbus_pin_active_low[AT91_MAX_USBH_PORTS];	/* vbus polarity */
};
extern void __init at91_add_device_usbh_ohci(struct at91_usbh_data *data);
extern void __init at91_add_device_usbh_ehci(struct at91_usbh_data *data);

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
	int		enable_pin;	/* chip enable */
	int		det_pin;	/* card detect */
	int		rdy_pin;	/* ready/busy */
	u8		ale;		/* address line number connected to ALE */
	u8		cle;		/* address line number connected to CLE */
	u8		bus_width_16;	/* buswidth is 16 bit */
	u8		ecc_mode;	/* NAND_ECC_* */
	u8		on_flash_bbt;	/* Use flash based bbt */
	u8		has_pmecc;	/* Use PMECC */
	u8		bus_on_d0;

	u8		pmecc_corr_cap;
	u16		pmecc_sector_size;
	u32		pmecc_lookup_table_offset;
};

void at91_add_device_nand(struct atmel_nand_data *data);

 /* Ethernet (EMAC & MACB) */
#define AT91SAM_ETX2_ETX3_ALTERNATIVE	(1 << 0)

void at91_add_device_eth(int id, struct macb_platform_data *data);

void at91_add_device_i2c(short i2c_id, struct i2c_board_info *devices, int nr_devices);

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
	int detect_pin;
	int wp_pin;
	char *devname;
};

void at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data);

/* SPI Master platform data */
struct at91_spi_platform_data {
	int *chipselect;	/* array of gpio_pins */
	int num_chipselect;	/* chipselect array entry count */
};

void at91_add_device_spi(int spi_id, struct at91_spi_platform_data *pdata);

void __init at91_add_device_lcdc(struct atmel_lcdfb_platform_data *data);

void at91sam_phy_reset(void __iomem *rstc_base);

void at91sam9_reset(void __iomem *sdram, void __iomem *rstc_cr);
void at91sam9g45_reset(void __iomem *sdram, void __iomem *rstc_cr);

#endif
