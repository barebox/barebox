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
#include <mach/at91sam9n12_matrix.h>
#include <mach/at91sam9_ddrsdr.h>
#include <mach/iomux.h>
#include <mach/cpu.h>
#include <i2c/i2c-gpio.h>

#include "generic.h"

void at91_add_device_sdram(u32 size)
{
	if (!size)
		size = at91sam9n12_get_ddram_size();

	arm_add_mem_device("ram0", AT91_CHIPSELECT_1, size);
	add_mem_device("sram0", AT91SAM9N12_SRAM_BASE,
			AT91SAM9N12_SRAM_SIZE, IORESOURCE_MEM_WRITEABLE);
}

/* --------------------------------------------------------------------
 *  USB Host (OHCI)
 * -------------------------------------------------------------------- */

#if defined(CONFIG_USB_OHCI)
void __init at91_add_device_usbh_ohci(struct at91_usbh_data *data)
{
	int i;

	if (!data)
		return;

	/* Enable VBus control for UHP ports */
	for (i = 0; i < data->ports; i++) {
		if (gpio_is_valid(data->vbus_pin[i]))
			at91_set_gpio_output(data->vbus_pin[i],
					     data->vbus_pin_active_low[i]);
	}

	add_generic_device("at91_ohci", DEVICE_ID_DYNAMIC, NULL, AT91SAM9N12_OHCI_BASE,
			SZ_1M, IORESOURCE_MEM, data);
}
#else
void __init at91_add_device_usbh_ohci(struct at91_usbh_data *data) {}
#endif

/* --------------------------------------------------------------------
 *  USB Device (Gadget)
 * -------------------------------------------------------------------- */

#ifdef CONFIG_USB_GADGET_DRIVER_AT91
void __init at91_add_device_udc(struct at91_udc_data *data)
{
	if (gpio_is_valid(data->vbus_pin)) {
		at91_set_gpio_input(data->vbus_pin, 0);
		at91_set_deglitch(data->vbus_pin, 1);
	}

	add_generic_device("at91_udc", DEVICE_ID_SINGLE, NULL, AT91SAM9N12_BASE_UDPFS,
			SZ_16K,  IORESOURCE_MEM, data);
}
#else
void __init at91_add_device_udc(struct at91_udc_data *data) {}
#endif

#if defined(CONFIG_MCI_ATMEL)
/* Consider only one slot : slot 0 */
void __init at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data)
{
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

	/* CLK */
	at91_set_A_periph(AT91_PIN_PA17, 0);

	/* CMD */
	at91_set_A_periph(AT91_PIN_PA16, 1);

	/* DAT0, maybe DAT1..DAT3 */
	at91_set_A_periph(AT91_PIN_PA15, 1);
	if (data->bus_width == 4) {
		at91_set_A_periph(AT91_PIN_PA18, 1);
		at91_set_A_periph(AT91_PIN_PA19, 1);
		at91_set_A_periph(AT91_PIN_PA20, 1);
	}

	add_generic_device("atmel_mci", DEVICE_ID_SINGLE, NULL, AT91SAM9N12_BASE_MCI, SZ_16K,
			   IORESOURCE_MEM, data);
}
#else
void __init at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data) {}
#endif

/* --------------------------------------------------------------------
 *  NAND / SmartMedia
 * -------------------------------------------------------------------- */

#if defined(CONFIG_NAND_ATMEL)
static struct resource nand_resources[] = {
	[0] = {
		.start	= AT91_CHIPSELECT_3,
		.end	= AT91_CHIPSELECT_3 + SZ_256M - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= AT91SAM9N12_BASE_PMECC,
		.end	= AT91SAM9N12_BASE_PMECC + 0x600 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= AT91SAM9N12_BASE_PMERRLOC,
		.end	= AT91SAM9N12_BASE_PMERRLOC + 0x200 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[3] = {
		.start	= AT91SAM9N12_ROM_BASE,
		.end	= AT91SAM9N12_ROM_BASE + AT91SAM9N12_ROM_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

void __init at91_add_device_nand(struct atmel_nand_data *data)
{
	unsigned long csa;

	if (!data)
		return;

	data->pmecc_lookup_table_offset = 0x8000;

	csa = readl(AT91SAM9N12_BASE_MATRIX + AT91SAM9N12_MATRIX_EBICSA);

	/* Assign CS3 to NAND/SmartMedia Interface */
	csa |= AT91SAM9N12_MATRIX_EBI_CS3A_SMC_NANDFLASH;
	/* Configure databus */
	if (!data->bus_on_d0)
		csa |= AT91SAM9N12_MATRIX_NFD0_ON_D16;
	else
		csa &= ~AT91SAM9N12_MATRIX_NFD0_ON_D16;
	/* Configure IO drive */
	csa |= AT91SAM9N12_MATRIX_EBI_HIGH_DRIVE;

	writel(csa, AT91SAM9N12_BASE_MATRIX + AT91SAM9N12_MATRIX_EBICSA);

	/* enable pin */
	if (gpio_is_valid(data->enable_pin))
		at91_set_gpio_output(data->enable_pin, 1);

	/* ready/busy pin */
	if (gpio_is_valid(data->rdy_pin))
		at91_set_gpio_input(data->rdy_pin, 1);

	/* card detect pin */
	if (gpio_is_valid(data->det_pin))
		at91_set_gpio_input(data->det_pin, 1);

	/* configure NANDOE */
	at91_set_A_periph(AT91_PIN_PD0, 1);
	/* configure NANDWE */
	at91_set_A_periph(AT91_PIN_PD1, 1);
	/* configure ALE */
	at91_set_A_periph(AT91_PIN_PD2, 1);
	/* configure CLE */
	at91_set_A_periph(AT91_PIN_PD3, 1);

	/* configure multiplexed pins for D16~D31 */
	if (!data->bus_on_d0) {
		at91_set_A_periph(AT91_PIN_PD6, 1);
		at91_set_A_periph(AT91_PIN_PD7, 1);
		at91_set_A_periph(AT91_PIN_PD8, 1);
		at91_set_A_periph(AT91_PIN_PD9, 1);
		at91_set_A_periph(AT91_PIN_PD10, 1);
		at91_set_A_periph(AT91_PIN_PD11, 1);
		at91_set_A_periph(AT91_PIN_PD12, 1);
		at91_set_A_periph(AT91_PIN_PD13, 1);

		if (data->bus_width_16) {
			at91_set_A_periph(AT91_PIN_PD14, 1);
			at91_set_A_periph(AT91_PIN_PD15, 1);
			at91_set_A_periph(AT91_PIN_PD16, 1);
			at91_set_A_periph(AT91_PIN_PD17, 1);
			at91_set_A_periph(AT91_PIN_PD18, 1);
			at91_set_A_periph(AT91_PIN_PD19, 1);
			at91_set_A_periph(AT91_PIN_PD20, 1);
			at91_set_A_periph(AT91_PIN_PD21, 1);
		}

	}

	add_generic_device_res("atmel_nand", DEVICE_ID_SINGLE, nand_resources,
			       ARRAY_SIZE(nand_resources), data);
}
#else
void __init at91_add_device_nand(struct atmel_nand_data *data) {}
#endif

#if defined(CONFIG_I2C_GPIO)
static struct i2c_gpio_platform_data pdata_i2c0 = {
	.sda_pin		= AT91_PIN_PA30,
	.sda_is_open_drain	= 1,
	.scl_pin		= AT91_PIN_PA31,
	.scl_is_open_drain	= 1,
	.udelay			= 5,		/* ~100 kHz */
};

void at91_add_device_i2c(short i2c_id, struct i2c_board_info *devices, int nr_devices)
{
	struct i2c_gpio_platform_data *pdata = &pdata_i2c0;

	i2c_register_board_info(0, devices, nr_devices);

	at91_set_GPIO_periph(pdata->sda_pin, 1);		/* TWD (SDA) */
	at91_set_multi_drive(pdata->sda_pin, 1);

	at91_set_GPIO_periph(pdata->scl_pin, 1);		/* TWCK (SCL) */
	at91_set_multi_drive(pdata->scl_pin, 1);

	add_generic_device_res("i2c-gpio", 0, NULL, 0, pdata);
}
#else
void at91_add_device_i2c(short i2c_id, struct i2c_board_info *devices, int nr_devices) {}
#endif

/* --------------------------------------------------------------------
 *  SPI
 * -------------------------------------------------------------------- */

#if defined(CONFIG_DRIVER_SPI_ATMEL)
static unsigned spi0_standard_cs[4] = { AT91_PIN_PA14, AT91_PIN_PA7, AT91_PIN_PA1, AT91_PIN_PB3 };

static unsigned spi1_standard_cs[4] = { AT91_PIN_PA8, AT91_PIN_PA0, AT91_PIN_PA31, AT91_PIN_PA30 };

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
		start = AT91SAM9N12_BASE_SPI0;
		at91_set_A_periph(AT91_PIN_PA11, 0);	/* SPI0_MISO */
		at91_set_A_periph(AT91_PIN_PA12, 0);	/* SPI0_MOSI */
		at91_set_A_periph(AT91_PIN_PA13, 0);	/* SPI0_SPCK */
		break;
	case 1:
		start = AT91SAM9N12_BASE_SPI1;
		at91_set_B_periph(AT91_PIN_PA21, 0);	/* SPI1_MISO */
		at91_set_B_periph(AT91_PIN_PA22, 0);	/* SPI1_MOSI */
		at91_set_B_periph(AT91_PIN_PA23, 0);	/* SPI1_SPCK */
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

	at91_set_A_periph(AT91_PIN_PC26, 0);	/* LCDPWM */

	at91_set_A_periph(AT91_PIN_PC27, 0);	/* LCDVSYNC */
	at91_set_A_periph(AT91_PIN_PC28, 0);	/* LCDHSYNC */

	at91_set_A_periph(AT91_PIN_PC24, 0);	/* LCDDISP */
	at91_set_A_periph(AT91_PIN_PC29, 0);	/* LCDDEN */
	at91_set_A_periph(AT91_PIN_PC30, 0);	/* LCDPCK */

	at91_set_A_periph(AT91_PIN_PC0, 0);	/* LCDD0 */
	at91_set_A_periph(AT91_PIN_PC1, 0);	/* LCDD1 */
	at91_set_A_periph(AT91_PIN_PC2, 0);	/* LCDD2 */
	at91_set_A_periph(AT91_PIN_PC3, 0);	/* LCDD3 */
	at91_set_A_periph(AT91_PIN_PC4, 0);	/* LCDD4 */
	at91_set_A_periph(AT91_PIN_PC5, 0);	/* LCDD5 */
	at91_set_A_periph(AT91_PIN_PC6, 0);	/* LCDD6 */
	at91_set_A_periph(AT91_PIN_PC7, 0);	/* LCDD7 */
	at91_set_A_periph(AT91_PIN_PC8, 0);	/* LCDD8 */
	at91_set_A_periph(AT91_PIN_PC9, 0);	/* LCDD9 */
	at91_set_A_periph(AT91_PIN_PC10, 0);	/* LCDD10 */
	at91_set_A_periph(AT91_PIN_PC11, 0);	/* LCDD11 */
	at91_set_A_periph(AT91_PIN_PC12, 0);	/* LCDD12 */
	at91_set_A_periph(AT91_PIN_PC13, 0);	/* LCDD13 */
	at91_set_A_periph(AT91_PIN_PC14, 0);	/* LCDD14 */
	at91_set_A_periph(AT91_PIN_PC15, 0);	/* LCDD15 */
	at91_set_A_periph(AT91_PIN_PC16, 0);	/* LCDD16 */
	at91_set_A_periph(AT91_PIN_PC17, 0);	/* LCDD17 */
	at91_set_A_periph(AT91_PIN_PC18, 0);	/* LCDD18 */
	at91_set_A_periph(AT91_PIN_PC19, 0);	/* LCDD19 */
	at91_set_A_periph(AT91_PIN_PC20, 0);	/* LCDD20 */
	at91_set_A_periph(AT91_PIN_PC21, 0);	/* LCDD21 */
	at91_set_A_periph(AT91_PIN_PC22, 0);	/* LCDD22 */
	at91_set_A_periph(AT91_PIN_PC23, 0);	/* LCDD23 */

	add_generic_device("atmel_hlcdfb", DEVICE_ID_SINGLE, NULL, AT91SAM9N12_BASE_LCDC, SZ_4K,
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
	at91_set_A_periph(AT91_PIN_PA9, 1);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PA10, 0);		/* DTXD */

	return AT91SAM9N12_BASE_DBGU;
}

resource_size_t __init at91_configure_usart0(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PA0, 0);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PA1, 1);		/* RXD0 */

	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PA2, 0);	/* RTS0 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PA3, 0);	/* CTS0 */

	return AT91SAM9N12_BASE_USART0;
}

resource_size_t __init at91_configure_usart1(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PA5, 0);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PA6, 1);		/* RXD1 */

	if (pins & ATMEL_UART_RTS)
		at91_set_C_periph(AT91_PIN_PC27, 0);	/* RTS1 */
	if (pins & ATMEL_UART_CTS)
		at91_set_C_periph(AT91_PIN_PC28, 0);	/* CTS1 */

	return AT91SAM9N12_BASE_USART1;
}

resource_size_t __init at91_configure_usart2(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PA7, 0);		/* TXD2 */
	at91_set_A_periph(AT91_PIN_PA8, 1);		/* RXD2 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PB0, 0);	/* RTS2 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PB1, 0);	/* CTS2 */

	return AT91SAM9N12_BASE_USART2;
}

resource_size_t __init at91_configure_usart3(unsigned pins)
{
	at91_set_B_periph(AT91_PIN_PC22, 0);		/* TXD3 */
	at91_set_B_periph(AT91_PIN_PC23, 1);		/* RXD3 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PC24, 0);	/* RTS3 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PC25, 0);	/* CTS3 */

	return AT91SAM9N12_BASE_USART3;
}
#endif
