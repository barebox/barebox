/*
 *  On-Chip devices setup code for the SAMA5D4 family
 *
 *  Copyright (C) 2014 Atmel Corporation.
 *		       Bo Shen <voice.shen@atmel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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
	add_mem_device("sram0", SAMA5D4_SRAM_BASE,
		       SAMA5D4_SRAM_SIZE, IORESOURCE_MEM_WRITEABLE);
}

/* --------------------------------------------------------------------
 *  NAND / SmartMedia
 * -------------------------------------------------------------------- */
#if defined(CONFIG_NAND_ATMEL)
static struct resource nand_resources[] = {
	[0] = {
		.start	= SAMA5D4_CHIPSELECT_3,
		.end	= SAMA5D4_CHIPSELECT_3 + SZ_128M - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SAMA5D4_BASE_PMECC,
		.end	= SAMA5D4_BASE_PMECC + 0x490 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= SAMA5D4_BASE_PMERRLOC,
		.end	= SAMA5D4_BASE_PMERRLOC + 0x100 - 1,
		.flags	= IORESOURCE_MEM,
	},
};

void __init at91_add_device_nand(struct atmel_nand_data *data)
{
	if (!data)
		return;

	at91_set_A_periph(AT91_PIN_PC5, 0);	/* D0 */
	at91_set_A_periph(AT91_PIN_PC6, 0);	/* D1 */
	at91_set_A_periph(AT91_PIN_PC7, 0);	/* D2 */
	at91_set_A_periph(AT91_PIN_PC8, 0);	/* D3 */
	at91_set_A_periph(AT91_PIN_PC9, 0);	/* D4 */
	at91_set_A_periph(AT91_PIN_PC10, 0);	/* D5 */
	at91_set_A_periph(AT91_PIN_PC11, 0);	/* D6 */
	at91_set_A_periph(AT91_PIN_PC12, 0);	/* D7 */
	at91_set_A_periph(AT91_PIN_PC13, 0);	/* RE */
	at91_set_A_periph(AT91_PIN_PC14, 0);	/* WE */
	at91_set_A_periph(AT91_PIN_PC15, 1);	/* NCS */
	at91_set_A_periph(AT91_PIN_PC16, 1);	/* RDY */
	at91_set_A_periph(AT91_PIN_PC17, 1);	/* ALE */
	at91_set_A_periph(AT91_PIN_PC18, 1);	/* CLE */

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
		at91_set_A_periph(AT91_PIN_PB16, 0);	/* GMDC */
		at91_set_A_periph(AT91_PIN_PB17, 0);	/* GMDIO */

		at91_set_A_periph(AT91_PIN_PB0, 0);	/* GTXCK */
		at91_set_A_periph(AT91_PIN_PB2, 0);	/* GTXEN */
		at91_set_A_periph(AT91_PIN_PB6, 0);	/* GRXDV */
		at91_set_A_periph(AT91_PIN_PB7, 0);	/* GRXER */

		switch (data->phy_interface) {
		case PHY_INTERFACE_MODE_MII:
			at91_set_A_periph(AT91_PIN_PB4, 0);	/* GCRS */
			at91_set_A_periph(AT91_PIN_PB5, 0);	/* GCOL */
			at91_set_A_periph(AT91_PIN_PB14, 0);	/* GTX2 */
			at91_set_A_periph(AT91_PIN_PB15, 0);	/* GTX3 */
			at91_set_A_periph(AT91_PIN_PB3, 0);	/* GTXER */
			at91_set_A_periph(AT91_PIN_PB1, 0);	/* GRXCK */
			at91_set_A_periph(AT91_PIN_PB10, 0);	/* GRX2 */
			at91_set_A_periph(AT91_PIN_PB11, 0);	/* GRX3 */
		case PHY_INTERFACE_MODE_RMII:
			at91_set_A_periph(AT91_PIN_PB12, 0);	/* GTX0 */
			at91_set_A_periph(AT91_PIN_PB13, 0);	/* GTX1 */
			at91_set_A_periph(AT91_PIN_PB8, 0);	/* GRX0 */
			at91_set_A_periph(AT91_PIN_PB9, 0);	/* GRX1 */
			break;
		default:
			return;
		}

		add_generic_device("macb", id, NULL, SAMA5D4_BASE_GMAC0, SZ_16K,
			   IORESOURCE_MEM, data);
		break;
	case 1:
		at91_set_B_periph(AT91_PIN_PA22, 0);	/* GMDC */
		at91_set_B_periph(AT91_PIN_PA23, 0);	/* GMDIO */

		at91_set_B_periph(AT91_PIN_PA2, 0);	/* GTXCK */
		at91_set_B_periph(AT91_PIN_PA4, 0);	/* GTXEN */
		at91_set_B_periph(AT91_PIN_PA10, 0);	/* GRXDV */
		at91_set_B_periph(AT91_PIN_PA11, 0);	/* GRXER */

		switch (data->phy_interface) {
		case PHY_INTERFACE_MODE_MII:
			at91_set_B_periph(AT91_PIN_PA6, 0);	/* GCRS */
			at91_set_B_periph(AT91_PIN_PA9, 0);	/* GCOL */
			at91_set_B_periph(AT91_PIN_PA20, 0);	/* GTX2 */
			at91_set_B_periph(AT91_PIN_PA21, 0);	/* GTX3 */
			at91_set_B_periph(AT91_PIN_PA5, 0);	/* GTXER */
			at91_set_B_periph(AT91_PIN_PA3, 0);	/* GRXCK */
			at91_set_B_periph(AT91_PIN_PA18, 0);	/* GRX2 */
			at91_set_B_periph(AT91_PIN_PA19, 0);	/* GRX3 */
		case PHY_INTERFACE_MODE_RMII:
			at91_set_B_periph(AT91_PIN_PA12, 0);	/* GTX0 */
			at91_set_B_periph(AT91_PIN_PA13, 0);	/* GTX1 */
			at91_set_B_periph(AT91_PIN_PA8, 0);	/* GRX0 */
			at91_set_B_periph(AT91_PIN_PA9, 0);	/* GRX1 */
			break;
		default:
			return;
		}

		add_generic_device("macb", id, NULL, SAMA5D4_BASE_GMAC1, SZ_16K,
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
void __init at91_add_device_mci(short mmc_id,
				struct atmel_mci_platform_data *data)
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
	/* MCI0 */
	case 0:
		start = SAMA5D4_BASE_HSMCI0;

		/* CLK */
		at91_set_B_periph(AT91_PIN_PC4, 0);

		/* CMD */
		at91_set_B_periph(AT91_PIN_PC5, 1);

		/* DAT0, maybe DAT1..DAT3 */
		at91_set_B_periph(AT91_PIN_PC6, 1);
		switch (data->bus_width) {
		case 8:
			at91_set_B_periph(AT91_PIN_PC10, 1);
			at91_set_B_periph(AT91_PIN_PC11, 1);
			at91_set_B_periph(AT91_PIN_PC12, 1);
			at91_set_B_periph(AT91_PIN_PC13, 1);
		case 4:
			at91_set_B_periph(AT91_PIN_PC7, 1);
			at91_set_B_periph(AT91_PIN_PC8, 1);
			at91_set_B_periph(AT91_PIN_PC9, 1);
		};

		break;
	/* MCI1 */
	case 1:
		start = SAMA5D4_BASE_HSMCI1;

		/*
		 * As the mci1 io internal pull down is to strong,
		 * which cause external pull up doesn't work, so,
		 * disable internal pull down.
		 */

		/* CLK */
		at91_set_C_periph(AT91_PIN_PE18, 0);
		at91_set_pulldown(AT91_PIN_PE18, 0);

		/* CMD */
		at91_set_C_periph(AT91_PIN_PE19, 1);
		at91_set_pulldown(AT91_PIN_PE19, 0);

		/* DAT0, maybe DAT1..DAT3 */
		at91_set_C_periph(AT91_PIN_PE20, 1);
		at91_set_pulldown(AT91_PIN_PE20, 0);
		if (data->bus_width == 4) {
			at91_set_C_periph(AT91_PIN_PE21, 1);
			at91_set_pulldown(AT91_PIN_PE21, 0);
			at91_set_C_periph(AT91_PIN_PE22, 1);
			at91_set_pulldown(AT91_PIN_PE22, 0);
			at91_set_C_periph(AT91_PIN_PE23, 1);
			at91_set_pulldown(AT91_PIN_PE23, 0);
		}

		break;
	}

	add_generic_device("atmel_mci", mmc_id, NULL, start, SZ_16K,
			   IORESOURCE_MEM, data);
}
#else
void __init at91_add_device_mci(short mmc_id,
				struct atmel_mci_platform_data *data) {}
#endif

#if defined(CONFIG_I2C_GPIO)
static struct i2c_gpio_platform_data pdata_i2c[] = {
	{
		.sda_pin		= AT91_PIN_PA30,
		.sda_is_open_drain	= 1,
		.scl_pin		= AT91_PIN_PA31,
		.scl_is_open_drain	= 1,
		.udelay			= 5,		/* ~100 kHz */
	}, {
		.sda_pin		= AT91_PIN_PE29,
		.sda_is_open_drain	= 1,
		.scl_pin		= AT91_PIN_PE30,
		.scl_is_open_drain	= 1,
		.udelay			= 5,		/* ~100 kHz */
	}, {
		.sda_pin		= AT91_PIN_PB29,
		.sda_is_open_drain	= 1,
		.scl_pin		= AT91_PIN_PB30,
		.scl_is_open_drain	= 1,
		.udelay			= 5,		/* ~100 kHz */
	}, {
		.sda_pin		= AT91_PIN_PC25,
		.sda_is_open_drain	= 1,
		.scl_pin		= AT91_PIN_PC26,
		.scl_is_open_drain	= 1,
		.udelay			= 5,		/* ~100 kHz */
	}
};

void at91_add_device_i2c(short i2c_id, struct i2c_board_info *devices,
			 int nr_devices)
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
void at91_add_device_i2c(short i2c_id, struct i2c_board_info *devices,
			 int nr_devices) {}
#endif

/* --------------------------------------------------------------------
 *  SPI
 * -------------------------------------------------------------------- */
#if defined(CONFIG_DRIVER_SPI_ATMEL)
static unsigned spi0_standard_cs[2] = { AT91_PIN_PC3, AT91_PIN_PC4 };
static unsigned spi1_standard_cs[2] = { AT91_PIN_PB21, AT91_PIN_PB22 };

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
		start = SAMA5D4_BASE_SPI0;
		at91_set_A_periph(AT91_PIN_PC0, 0);	/* SPI0_MISO */
		at91_set_A_periph(AT91_PIN_PC1, 0);	/* SPI0_MOSI */
		at91_set_A_periph(AT91_PIN_PC2, 0);	/* SPI0_SPCK */
		break;
	case 1:
		start = SAMA5D4_BASE_SPI1;
		at91_set_A_periph(AT91_PIN_PB18, 0);	/* SPI1_MISO */
		at91_set_A_periph(AT91_PIN_PB19, 0);	/* SPI1_MOSI */
		at91_set_A_periph(AT91_PIN_PB20, 0);	/* SPI1_SPCK */
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

	at91_set_A_periph(AT91_PIN_PA24, 0);	/* LCDPWM */
	at91_set_A_periph(AT91_PIN_PA25, 0);	/* LCDDISP */
	at91_set_A_periph(AT91_PIN_PA26, 0);	/* LCDVSYNC */
	at91_set_A_periph(AT91_PIN_PA27, 0);	/* LCDHSYNC */
	at91_set_A_periph(AT91_PIN_PA28, 0);	/* LCDDOTCK */
	at91_set_A_periph(AT91_PIN_PA29, 0);	/* LCDDEN */

	at91_set_A_periph(AT91_PIN_PA2, 0);	/* LCDD2 */
	at91_set_A_periph(AT91_PIN_PA3, 0);	/* LCDD3 */
	at91_set_A_periph(AT91_PIN_PA4, 0);	/* LCDD4 */
	at91_set_A_periph(AT91_PIN_PA5, 0);	/* LCDD5 */
	at91_set_A_periph(AT91_PIN_PA6, 0);	/* LCDD6 */
	at91_set_A_periph(AT91_PIN_PA7, 0);	/* LCDD7 */

	at91_set_A_periph(AT91_PIN_PA10, 0);	/* LCDD10 */
	at91_set_A_periph(AT91_PIN_PA11, 0);	/* LCDD11 */
	at91_set_A_periph(AT91_PIN_PA12, 0);	/* LCDD12 */
	at91_set_A_periph(AT91_PIN_PA13, 0);	/* LCDD13 */
	at91_set_A_periph(AT91_PIN_PA14, 0);	/* LCDD14 */
	at91_set_A_periph(AT91_PIN_PA15, 0);	/* LCDD15 */

	at91_set_A_periph(AT91_PIN_PA18, 0);	/* LCDD18 */
	at91_set_A_periph(AT91_PIN_PA19, 0);	/* LCDD19 */
	at91_set_A_periph(AT91_PIN_PA20, 0);	/* LCDD20 */
	at91_set_A_periph(AT91_PIN_PA21, 0);	/* LCDD21 */
	at91_set_A_periph(AT91_PIN_PA22, 0);	/* LCDD22 */
	at91_set_A_periph(AT91_PIN_PA23, 0);	/* LCDD23 */

	add_generic_device("atmel_hlcdfb", DEVICE_ID_SINGLE, NULL,
			   SAMA5D4_BASE_LCDC, SZ_4K, IORESOURCE_MEM, data);
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
	at91_set_A_periph(AT91_PIN_PB25, 0);		/* DTXD */
	at91_set_A_periph(AT91_PIN_PB24, 1);		/* DRXD */

	return SAMA5D4_BASE_DBGU;
}

resource_size_t __init at91_configure_usart0(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PD13, 0);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PD12, 1);		/* RXD0 */

	return SAMA5D4_BASE_USART0;
}

resource_size_t __init at91_configure_usart1(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PD17, 0);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PD16, 1);		/* RXD1 */

	return SAMA5D4_BASE_USART1;
}


resource_size_t __init at91_configure_usart2(unsigned pins)
{
	at91_set_B_periph(AT91_PIN_PB5, 0);		/* TXD2 */
	at91_set_B_periph(AT91_PIN_PB4, 1);		/* RXD2 */

	return SAMA5D4_BASE_USART2;
}

resource_size_t __init at91_configure_usart3(unsigned pins)
{
	at91_set_B_periph(AT91_PIN_PE17, 0);		/* TXD3 */
	at91_set_B_periph(AT91_PIN_PE16, 1);		/* RXD3 */

	return SAMA5D4_BASE_USART3;
}

resource_size_t __init at91_configure_usart4(unsigned pins)
{
	at91_set_B_periph(AT91_PIN_PE27, 0);		/* TXD4 */
	at91_set_B_periph(AT91_PIN_PE26, 1);		/* RXD4 */

	return SAMA5D4_BASE_USART4;
}

resource_size_t __init at91_configure_usart5(unsigned pins)
{
	at91_set_B_periph(AT91_PIN_PE30, 0);		/* UTXD0 */
	at91_set_B_periph(AT91_PIN_PE29, 1);		/* URXD0 */

	return SAMA5D4_BASE_UART0;
}

resource_size_t __init at91_configure_usart6(unsigned pins)
{
	at91_set_C_periph(AT91_PIN_PC26, 0);		/* UTXD1 */
	at91_set_C_periph(AT91_PIN_PC25, 1);		/* URXD1 */

	return SAMA5D4_BASE_UART1;
}
#endif
