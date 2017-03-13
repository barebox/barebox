/*
 *  On-Chip devices setup code for the AT91SAM9x5 family
 *
 *  Copyright (C) 2010 Atmel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <gpio.h>
#include <asm/armlinux.h>
#include <mach/hardware.h>
#include <mach/board.h>
#include <mach/at91_pmc.h>
#include <mach/at91sam9x5_matrix.h>
#include <mach/at91sam9_ddrsdr.h>
#include <mach/iomux.h>
#include <mach/io.h>
#include <mach/cpu.h>
#include <i2c/i2c-gpio.h>

#include "generic.h"

void at91_add_device_sdram(u32 size)
{
	if (!size)
		size = at91sama5_get_ddram_size();

	arm_add_mem_device("ram0", SAMA5_DDRCS, size);
	add_mem_device("sram0", SAMA5D3_SRAM_BASE,
			SAMA5D3_SRAM_SIZE, IORESOURCE_MEM_WRITEABLE);
}

/* --------------------------------------------------------------------
 *  NAND / SmartMedia
 * -------------------------------------------------------------------- */

#if defined(CONFIG_NAND_ATMEL)
static struct resource nand_resources[] = {
	[0] = {
		.start	= SAMA5_CHIPSELECT_3,
		.end	= SAMA5_CHIPSELECT_3 + SZ_256M - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SAMA5D3_BASE_PMECC,
		.end	= SAMA5D3_BASE_PMECC + 0x490 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= SAMA5D3_BASE_PMERRLOC,
		.end	= SAMA5D3_BASE_PMERRLOC + 0x100 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[3] = {
		.start	= SAMA5D3_ROM_BASE,
		.end	= SAMA5D3_ROM_BASE + SAMA5D3_ROM_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

void __init at91_add_device_nand(struct atmel_nand_data *data)
{
	if (!data)
		return;

	switch (data->pmecc_sector_size) {
	case 512:
		data->pmecc_lookup_table_offset = 0x10000;
		break;
	case 1024:
		data->pmecc_lookup_table_offset = 0x18000;
		break;
	default:
		pr_err("%s: invalid pmecc_sector_size (%d)\n", __func__,
			data->pmecc_sector_size);
		return;
	}

	at91_set_A_periph(AT91_PIN_PE21, 1);		/* ALE */
	at91_set_A_periph(AT91_PIN_PE22, 1);		/* CLE */

	/* enable pin */
	if (gpio_is_valid(data->enable_pin))
		at91_set_gpio_output(data->enable_pin, 1);

	/* ready/busy pin */
	if (gpio_is_valid(data->rdy_pin))
		at91_set_gpio_input(data->rdy_pin, 1);

	/* card detect pin */
	if (gpio_is_valid(data->det_pin))
		at91_set_gpio_input(data->det_pin, 1);

	add_generic_device_res("atmel_nand", 0, nand_resources,
			       ARRAY_SIZE(nand_resources), data);
}
#else
void __init at91_add_device_nand(struct atmel_nand_data *data) {}
#endif

#if defined(CONFIG_DRIVER_NET_MACB)
void at91_add_device_eth(int id, struct macb_platform_data *data)
{
	if (!data)
		return;

	switch (id) {
	case 0:
		if (cpu_is_sama5d31()) {
			pr_warn("AT91: no gmac on sama5d31\n");
			return;
		}

		at91_set_A_periph(AT91_PIN_PB16, 0);	/* GMDC */
		at91_set_A_periph(AT91_PIN_PB17, 0);	/* GMDIO */

		at91_set_A_periph(AT91_PIN_PB9, 0);	/* GTXEN */
		at91_set_A_periph(AT91_PIN_PB11, 0);	/* GRXCK */
		at91_set_A_periph(AT91_PIN_PB13, 0);	/* GRXER */

		switch (data->phy_interface) {
		case PHY_INTERFACE_MODE_GMII:
			at91_set_B_periph(AT91_PIN_PB19, 0);	/* GTX4 */
			at91_set_B_periph(AT91_PIN_PB20, 0);	/* GTX5 */
			at91_set_B_periph(AT91_PIN_PB21, 0);	/* GTX6 */
			at91_set_B_periph(AT91_PIN_PB22, 0);	/* GTX7 */
			at91_set_B_periph(AT91_PIN_PB23, 0);	/* GRX4 */
			at91_set_B_periph(AT91_PIN_PB24, 0);	/* GRX5 */
			at91_set_B_periph(AT91_PIN_PB25, 0);	/* GRX6 */
			at91_set_B_periph(AT91_PIN_PB26, 0);	/* GRX7 */
		case PHY_INTERFACE_MODE_MII:
		case PHY_INTERFACE_MODE_RGMII:
			at91_set_A_periph(AT91_PIN_PB0, 0);	/* GTX0 */
			at91_set_A_periph(AT91_PIN_PB1, 0);	/* GTX1 */
			at91_set_A_periph(AT91_PIN_PB2, 0);	/* GTX2 */
			at91_set_A_periph(AT91_PIN_PB3, 0);	/* GTX3 */
			at91_set_A_periph(AT91_PIN_PB4, 0);	/* GRX0 */
			at91_set_A_periph(AT91_PIN_PB5, 0);	/* GRX1 */
			at91_set_A_periph(AT91_PIN_PB6, 0);	/* GRX2 */
			at91_set_A_periph(AT91_PIN_PB7, 0);	/* GRX3 */
			break;
		default:
			return;
		}

		switch (data->phy_interface) {
		case PHY_INTERFACE_MODE_MII:
			at91_set_A_periph(AT91_PIN_PB8, 0);	/* GTXCK */
			at91_set_A_periph(AT91_PIN_PB10, 0);	/* GTXER */
			at91_set_A_periph(AT91_PIN_PB12, 0);	/* GRXDV */
			at91_set_A_periph(AT91_PIN_PB14, 0);	/* GCRS */
			at91_set_A_periph(AT91_PIN_PB15, 0);	/* GCOL */
			break;
		case PHY_INTERFACE_MODE_RGMII:
			at91_set_A_periph(AT91_PIN_PB8, 0);	/* GTXCK */
			at91_set_A_periph(AT91_PIN_PB18, 0);	/* G125CK */
			break;
		case PHY_INTERFACE_MODE_GMII:
			at91_set_A_periph(AT91_PIN_PB10, 0);	/* GTXER */
			at91_set_A_periph(AT91_PIN_PB12, 0);	/* GRXDV */
			at91_set_A_periph(AT91_PIN_PB14, 0);	/* GCRS */
			at91_set_A_periph(AT91_PIN_PB15, 0);	/* GCOL */
			at91_set_A_periph(AT91_PIN_PB27, 0);	/* G125CK0 */
			break;
		default:
			return;
		}

		add_generic_device("macb", id, NULL, SAMA5D3_BASE_GMAC, SZ_16K,
			   IORESOURCE_MEM, data);
		break;
	case 1:
		if (cpu_is_sama5d33() || cpu_is_sama5d34()) {
			pr_warn("AT91: no macb on sama5d33/d34\n");
			return;
		}

		if (data->phy_interface != PHY_INTERFACE_MODE_RMII) {
			pr_warn("AT91: Only RMII available on interfacr macb%d.\n", id);
			return;
		}

		at91_set_A_periph(AT91_PIN_PC7, 0);	/* ETXCK_EREFCK */
		at91_set_A_periph(AT91_PIN_PC5, 0);	/* ERXDV */
		at91_set_A_periph(AT91_PIN_PC2, 0);	/* ERX0 */
		at91_set_A_periph(AT91_PIN_PC3, 0);	/* ERX1 */
		at91_set_A_periph(AT91_PIN_PC6, 0);	/* ERXER */
		at91_set_A_periph(AT91_PIN_PC4, 0);	/* ETXEN */
		at91_set_A_periph(AT91_PIN_PC0, 0);	/* ETX0 */
		at91_set_A_periph(AT91_PIN_PC1, 0);	/* ETX1 */
		at91_set_A_periph(AT91_PIN_PC9, 0);	/* EMDIO */
		at91_set_A_periph(AT91_PIN_PC8, 0);	/* EMDC */
		add_generic_device("macb", id, NULL, SAMA5D3_BASE_EMAC, SZ_16K,
			   IORESOURCE_MEM, data);
		break;
	default:
		return;
	}

}
#else
void at91_add_device_eth(int id, struct macb_platform_data *data) {}
#endif

#if defined(CONFIG_MCI_ATMEL)
/* Consider only one slot : slot 0 */
void __init at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data)
{
	resource_size_t start = ~0;

	if (!data)
		return;

	/* Must have at least one usable slot */
	if (!data->bus_width)
		return;

	/* input/irq */
	if (gpio_is_valid(data->detect_pin)) {
		at91_set_gpio_input(data->detect_pin, 1);
		at91_set_deglitch(data->detect_pin, 1);
	}
	if (gpio_is_valid(data->wp_pin))
		at91_set_gpio_input(data->wp_pin, 1);

	switch (mmc_id) {
	case 0:		/* MCI0 */
		start = SAMA5D3_BASE_HSMCI0;

		/* CLK */
		at91_set_A_periph(AT91_PIN_PD9, 0);

		/* CMD */
		at91_set_A_periph(AT91_PIN_PD0, 1);

		/* DAT0, maybe DAT1..DAT3 */
		at91_set_A_periph(AT91_PIN_PD1, 1);
		switch (data->bus_width) {
		case 8:
			at91_set_A_periph(AT91_PIN_PD5, 1);
			at91_set_A_periph(AT91_PIN_PD6, 1);
			at91_set_A_periph(AT91_PIN_PD7, 1);
			at91_set_A_periph(AT91_PIN_PD8, 1);
		case 4:
			at91_set_A_periph(AT91_PIN_PD2, 1);
			at91_set_A_periph(AT91_PIN_PD3, 1);
			at91_set_A_periph(AT91_PIN_PD4, 1);
		};

		break;
	case 1:			/* MCI1 */
		start = SAMA5D3_BASE_HSMCI1;

		/* CLK */
		at91_set_A_periph(AT91_PIN_PB24, 0);

		/* CMD */
		at91_set_A_periph(AT91_PIN_PB19, 1);

		/* DAT0, maybe DAT1..DAT3 */
		at91_set_A_periph(AT91_PIN_PB20, 1);
		if (data->bus_width == 4) {
			at91_set_A_periph(AT91_PIN_PB21, 1);
			at91_set_A_periph(AT91_PIN_PB22, 1);
			at91_set_A_periph(AT91_PIN_PB23, 1);
		}
		break;
	case 2:			/* MCI2 */
		start = SAMA5D3_BASE_HSMCI2;

		/* CLK */
		at91_set_A_periph(AT91_PIN_PC15, 0);

		/* CMD */
		at91_set_A_periph(AT91_PIN_PC10, 1);

		/* DAT0, maybe DAT1..DAT3 */
		at91_set_A_periph(AT91_PIN_PC11, 1);
		if (data->bus_width == 4) {
			at91_set_A_periph(AT91_PIN_PC12, 1);
			at91_set_A_periph(AT91_PIN_PC13, 1);
			at91_set_A_periph(AT91_PIN_PC14, 1);
		}
	}

	add_generic_device("atmel_mci", mmc_id, NULL, start, SZ_16K,
			   IORESOURCE_MEM, data);
}
#else
void __init at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data) {}
#endif

#if defined(CONFIG_I2C_GPIO)
static struct i2c_gpio_platform_data pdata_i2c [] = {
	{
		.sda_pin		= AT91_PIN_PA30,
		.sda_is_open_drain	= 1,
		.scl_pin		= AT91_PIN_PA31,
		.scl_is_open_drain	= 1,
		.udelay			= 5,		/* ~100 kHz */
	}, {
		.sda_pin		= AT91_PIN_PC26,
		.sda_is_open_drain	= 1,
		.scl_pin		= AT91_PIN_PC27,
		.scl_is_open_drain	= 1,
		.udelay			= 5,		/* ~100 kHz */
	}, {
		.sda_pin		= AT91_PIN_PA18,
		.sda_is_open_drain	= 1,
		.scl_pin		= AT91_PIN_PA19,
		.scl_is_open_drain	= 1,
		.udelay			= 5,		/* ~100 kHz */
	}
};

void at91_add_device_i2c(short i2c_id, struct i2c_board_info *devices, int nr_devices)
{
	struct i2c_gpio_platform_data *pdata;

	if (i2c_id > ARRAY_SIZE(pdata_i2c))
		return;

	i2c_register_board_info(i2c_id, devices, nr_devices);

	pdata = &pdata_i2c[i2c_id];

	at91_set_GPIO_periph(pdata->sda_pin, 1);		/* TWD (SDA) */
	at91_set_multi_drive(pdata->sda_pin, 1);

	at91_set_GPIO_periph(pdata->scl_pin, 1);		/* TWCK (SCL) */
	at91_set_multi_drive(pdata->scl_pin, 1);

	add_generic_device_res("i2c-gpio", i2c_id, NULL, 0, pdata);
}
#else
void at91_add_device_i2c(short i2c_id, struct i2c_board_info *devices, int nr_devices) {}
#endif

/* --------------------------------------------------------------------
 *  SPI
 * -------------------------------------------------------------------- */

#if defined(CONFIG_DRIVER_SPI_ATMEL)
static unsigned spi0_standard_cs[4] = { AT91_PIN_PD13, AT91_PIN_PD14, AT91_PIN_PD15, AT91_PIN_PD16 };

static unsigned spi1_standard_cs[4] = { AT91_PIN_PC25, AT91_PIN_PC26, AT91_PIN_PC27, AT91_PIN_PC28 };

static struct at91_spi_platform_data spi_pdata[] = {
	[0] = {
		.chipselect = spi0_standard_cs,
		.num_chipselect = ARRAY_SIZE(spi0_standard_cs),
	},
	[1] = {
		.chipselect = spi1_standard_cs,
		.num_chipselect = ARRAY_SIZE(spi1_standard_cs),
	},
};

void at91_add_device_spi(int spi_id, struct at91_spi_platform_data *pdata)
{
	int i;
	int cs_pin;
	resource_size_t start = ~0;

	BUG_ON(spi_id > 1);

	if (!pdata)
		pdata = &spi_pdata[spi_id];

	for (i = 0; i < pdata->num_chipselect; i++) {
		cs_pin = pdata->chipselect[i];

		/* enable chip-select pin */
		if (gpio_is_valid(cs_pin))
			at91_set_gpio_output(cs_pin, 1);
	}

	/* Configure SPI bus(es) */
	switch (spi_id) {
	case 0:
		start = SAMA5D3_BASE_SPI0;
		at91_set_A_periph(AT91_PIN_PD10, 0);	/* SPI0_MISO */
		at91_set_A_periph(AT91_PIN_PD11, 0);	/* SPI0_MOSI */
		at91_set_A_periph(AT91_PIN_PD12, 0);	/* SPI0_SPCK */
		break;
	case 1:
		start = SAMA5D3_BASE_SPI1;
		at91_set_B_periph(AT91_PIN_PC22, 0);	/* SPI1_MISO */
		at91_set_B_periph(AT91_PIN_PC23, 0);	/* SPI1_MOSI */
		at91_set_B_periph(AT91_PIN_PC24, 0);	/* SPI1_SPCK */
		break;
	}

	add_generic_device("atmel_spi", spi_id, NULL, start, SZ_16K,
			   IORESOURCE_MEM, pdata);
}
#else
void at91_add_device_spi(int spi_id, struct at91_spi_platform_data *pdata) {}
#endif

/* --------------------------------------------------------------------
 *  LCD Controller
 * -------------------------------------------------------------------- */

#if defined(CONFIG_DRIVER_VIDEO_ATMEL_HLCD)
void __init at91_add_device_lcdc(struct atmel_lcdfb_platform_data *data)
{
	BUG_ON(!data);

	if (cpu_is_sama5d35()) {
		pr_warn("AT91: no lcd on sama5d35\n");
		return;
	}

	at91_set_A_periph(AT91_PIN_PA24, 0);	/* LCDPWM */
	at91_set_A_periph(AT91_PIN_PA25, 0);	/* LCDDISP */
	at91_set_A_periph(AT91_PIN_PA26, 0);	/* LCDVSYNC */
	at91_set_A_periph(AT91_PIN_PA27, 0);	/* LCDHSYNC */
	at91_set_A_periph(AT91_PIN_PA28, 0);	/* LCDDOTCK */
	at91_set_A_periph(AT91_PIN_PA29, 0);	/* LCDDEN */

	at91_set_A_periph(AT91_PIN_PA0, 0);	/* LCDD0 */
	at91_set_A_periph(AT91_PIN_PA1, 0);	/* LCDD1 */
	at91_set_A_periph(AT91_PIN_PA2, 0);	/* LCDD2 */
	at91_set_A_periph(AT91_PIN_PA3, 0);	/* LCDD3 */
	at91_set_A_periph(AT91_PIN_PA4, 0);	/* LCDD4 */
	at91_set_A_periph(AT91_PIN_PA5, 0);	/* LCDD5 */
	at91_set_A_periph(AT91_PIN_PA6, 0);	/* LCDD6 */
	at91_set_A_periph(AT91_PIN_PA7, 0);	/* LCDD7 */
	at91_set_A_periph(AT91_PIN_PA8, 0);	/* LCDD8 */
	at91_set_A_periph(AT91_PIN_PA9, 0);	/* LCDD9 */
	at91_set_A_periph(AT91_PIN_PA10, 0);	/* LCDD10 */
	at91_set_A_periph(AT91_PIN_PA11, 0);	/* LCDD11 */
	at91_set_A_periph(AT91_PIN_PA12, 0);	/* LCDD12 */
	at91_set_A_periph(AT91_PIN_PA13, 0);	/* LCDD13 */
	at91_set_A_periph(AT91_PIN_PA14, 0);	/* LCDD14 */
	at91_set_A_periph(AT91_PIN_PA15, 0);	/* LCDD15 */
	at91_set_C_periph(AT91_PIN_PC14, 0);	/* LCDD16 */
	at91_set_C_periph(AT91_PIN_PC13, 0);	/* LCDD17 */
	at91_set_C_periph(AT91_PIN_PC12, 0);	/* LCDD18 */
	at91_set_C_periph(AT91_PIN_PC11, 0);	/* LCDD19 */
	at91_set_C_periph(AT91_PIN_PC10, 0);	/* LCDD20 */
	at91_set_C_periph(AT91_PIN_PC15, 0);	/* LCDD21 */
	at91_set_C_periph(AT91_PIN_PE27, 0);	/* LCDD22 */
	at91_set_C_periph(AT91_PIN_PE28, 0);	/* LCDD23 */

	add_generic_device("atmel_hlcdfb", DEVICE_ID_SINGLE, NULL, SAMA5D3_BASE_LCDC, SZ_4K,
			   IORESOURCE_MEM, data);
}
#else
void __init at91_add_device_lcdc(struct atmel_lcdfb_platform_data *data) {}
#endif

/* --------------------------------------------------------------------
 *  UART
 * -------------------------------------------------------------------- */

#if defined(CONFIG_DRIVER_SERIAL_ATMEL)
resource_size_t __init at91_configure_dbgu(void)
{
	at91_set_A_periph(AT91_PIN_PB30, 1);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PB31, 0);		/* DTXD */

	return AT91_BASE_DBGU1;
}

resource_size_t __init at91_configure_usart0(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PD18, 0);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PD17, 1);		/* RXD0 */

	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PD16, 0);	/* RTS0 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PD15, 0);	/* CTS0 */

	return SAMA5D3_BASE_USART0;
}

resource_size_t __init at91_configure_usart1(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB29, 0);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PB28, 1);		/* RXD1 */

	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PB27, 0);	/* RTS1 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PB26, 0);	/* CTS1 */

	return SAMA5D3_BASE_USART1;
}
#endif
