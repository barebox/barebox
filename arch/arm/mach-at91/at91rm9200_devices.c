/*
 * arch/arm/mach-at91/at91rm9200_devices.c
 *
 *  Copyright (C) 2005 Thibaut VARENE <varenet@parisc-linux.org>
 *  Copyright (C) 2005 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <common.h>
#include <gpio.h>
#include <asm/armlinux.h>
#include <mach/hardware.h>
#include <mach/at91rm9200.h>
#include <mach/board.h>
#include <mach/iomux.h>
#include <mach/io.h>
#include <mach/at91rm9200_mc.h>
#include <i2c/i2c-gpio.h>
#include <linux/sizes.h>

#include "generic.h"

void at91_add_device_sdram(u32 size)
{
	if (!size)
		size = at91rm9200_get_sdram_size();

	arm_add_mem_device("ram0", AT91_CHIPSELECT_1, size);
	add_mem_device("sram0", AT91RM9200_SRAM_BASE, AT91RM9200_SRAM_SIZE,
			IORESOURCE_MEM_WRITEABLE);
}

/* --------------------------------------------------------------------
 *  USB Host
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

	add_generic_device("at91_ohci", DEVICE_ID_DYNAMIC, NULL, AT91RM9200_UHP_BASE,
			1024 * 1024, IORESOURCE_MEM, data);
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

	if (gpio_is_valid(data->pullup_pin))
		at91_set_gpio_output(data->pullup_pin, 0);

	add_generic_device("at91_udc", DEVICE_ID_DYNAMIC, NULL, AT91RM9200_BASE_UDP,
			SZ_16K, IORESOURCE_MEM, data);
}
#else
void __init at91_add_device_udc(struct at91_udc_data *data) {}
#endif

/* --------------------------------------------------------------------
 *  Ethernet
 * -------------------------------------------------------------------- */

#if defined(CONFIG_DRIVER_NET_AT91_ETHER)
void __init at91_add_device_eth(int id, struct macb_platform_data *data)
{
	if (!data)
		return;

	/* Pins used for MII and RMII */
	at91_set_A_periph(AT91_PIN_PA16, 0);	/* EMDIO */
	at91_set_A_periph(AT91_PIN_PA15, 0);	/* EMDC */
	at91_set_A_periph(AT91_PIN_PA14, 0);	/* ERXER */
	at91_set_A_periph(AT91_PIN_PA13, 0);	/* ERX1 */
	at91_set_A_periph(AT91_PIN_PA12, 0);	/* ERX0 */
	at91_set_A_periph(AT91_PIN_PA11, 0);	/* ECRS_ECRSDV */
	at91_set_A_periph(AT91_PIN_PA10, 0);	/* ETX1 */
	at91_set_A_periph(AT91_PIN_PA9, 0);	/* ETX0 */
	at91_set_A_periph(AT91_PIN_PA8, 0);	/* ETXEN */
	at91_set_A_periph(AT91_PIN_PA7, 0);	/* ETXCK_EREFCK */

	if (data->phy_interface != PHY_INTERFACE_MODE_RMII) {
		at91_set_B_periph(AT91_PIN_PB19, 0);	/* ERXCK */
		at91_set_B_periph(AT91_PIN_PB18, 0);	/* ECOL */
		at91_set_B_periph(AT91_PIN_PB17, 0);	/* ERXDV */
		at91_set_B_periph(AT91_PIN_PB16, 0);	/* ERX3 */
		at91_set_B_periph(AT91_PIN_PB15, 0);	/* ERX2 */
		at91_set_B_periph(AT91_PIN_PB14, 0);	/* ETXER */
		at91_set_B_periph(AT91_PIN_PB13, 0);	/* ETX3 */
		at91_set_B_periph(AT91_PIN_PB12, 0);	/* ETX2 */
	}

	add_generic_device("at91_ether", 0, NULL, AT91_VA_BASE_EMAC, 0x1000,
			   IORESOURCE_MEM, data);
}
#else
void __init at91_add_device_eth(int id, struct macb_platform_data *data) {}
#endif

/* --------------------------------------------------------------------
 *  NAND / SmartMedia
 * -------------------------------------------------------------------- */

#if defined(CONFIG_NAND_ATMEL)
void __init at91_add_device_nand(struct atmel_nand_data *data)
{
	unsigned int csa;

	if (!data)
		return;

	/* enable the address range of CS3 */
	csa = at91_sys_read(AT91_EBI_CSA);
	at91_sys_write(AT91_EBI_CSA, csa | AT91_EBI_CS3A_SMC_SMARTMEDIA);

	/* set the bus interface characteristics */
	at91_sys_write(AT91_SMC_CSR(3), AT91_SMC_ACSS_STD | AT91_SMC_DBW_8 | AT91_SMC_WSEN
		| AT91_SMC_NWS_(5)
		| AT91_SMC_TDF_(1)
		| AT91_SMC_RWSETUP_(0)	/* tDS Data Set up Time 30 - ns */
		| AT91_SMC_RWHOLD_(1)	/* tDH Data Hold Time 20 - ns */
	);

	/* enable pin */
	if (gpio_is_valid(data->enable_pin))
		at91_set_gpio_output(data->enable_pin, 1);

	/* ready/busy pin */
	if (gpio_is_valid(data->rdy_pin))
		at91_set_gpio_input(data->rdy_pin, 1);

	/* card detect pin */
	if (gpio_is_valid(data->det_pin))
		at91_set_gpio_input(data->det_pin, 1);

	at91_set_A_periph(AT91_PIN_PC1, 0);		/* SMOE */
	at91_set_A_periph(AT91_PIN_PC3, 0);		/* SMWE */

	add_generic_device("atmel_nand", 0, NULL, AT91_CHIPSELECT_3, 0x10,
			   IORESOURCE_MEM, data);
}
#else
void __init at91_add_device_nand(struct atmel_nand_data *data) {}
#endif

#if defined(CONFIG_I2C_GPIO)
static struct i2c_gpio_platform_data pdata_i2c = {
	.sda_pin		= AT91_PIN_PA25,
	.sda_is_open_drain	= 1,
	.scl_pin		= AT91_PIN_PA26,
	.scl_is_open_drain	= 1,
	.udelay			= 5,		/* ~100 kHz */
};

void at91_add_device_i2c(short i2c_id, struct i2c_board_info *devices, int nr_devices)
{
	struct i2c_gpio_platform_data *pdata = &pdata_i2c;

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
static unsigned spi_standard_cs[4] = { AT91_PIN_PA3, AT91_PIN_PA4, AT91_PIN_PA5, AT91_PIN_PA6 };

static struct at91_spi_platform_data spi_pdata[] = {
	[0] = {
		.chipselect = spi_standard_cs,
		.num_chipselect = ARRAY_SIZE(spi_standard_cs),
	},
};

void at91_add_device_spi(int spi_id, struct at91_spi_platform_data *pdata)
{
	int i;
	int cs_pin;

	BUG_ON(spi_id > 0);

	if (!pdata)
		pdata = &spi_pdata[spi_id];

	for (i = 0; i < pdata->num_chipselect; i++) {
		cs_pin = pdata->chipselect[i];

		/* enable chip-select pin */
		if (!gpio_is_valid(cs_pin))
			continue;

		if (cs_pin == AT91_PIN_PA3)
			at91_set_A_periph(cs_pin, 0);
		else
			at91_set_gpio_output(cs_pin, 1);
	}

	at91_set_A_periph(AT91_PIN_PA0, 0);	/* MISO */
	at91_set_A_periph(AT91_PIN_PA1, 0);	/* MOSI */
	at91_set_A_periph(AT91_PIN_PA2, 0);	/* SPCK */

	add_generic_device("atmel_spi", spi_id, NULL, AT91RM9200_BASE_SPI,
			   SZ_16K, IORESOURCE_MEM, pdata);
}
#else
void __init at91_add_device_spi(int spi_id, struct at91_spi_platform_data *pdata) {}
#endif


/* --------------------------------------------------------------------
 *  UART
 * -------------------------------------------------------------------- */

resource_size_t __init at91_configure_dbgu(void)
{
	at91_set_A_periph(AT91_PIN_PA30, 1);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PA31, 0);		/* DTXD */

	return AT91_BASE_SYS + AT91_DBGU;
}

resource_size_t __init at91_configure_usart0(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PA17, 0);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PA18, 1);		/* RXD0 */

	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PA20, 0);	/* CTS0 */

	if (pins & ATMEL_UART_RTS) {
		/*
		 * AT91RM9200 Errata #39 - RTS0 is not internally connected to PA21.
		 *  We need to drive the pin manually.  Default is off (RTS is active low).
		 */
		at91_set_gpio_output(AT91_PIN_PA21, 1);
	}

	return AT91RM9200_BASE_US0;
}

resource_size_t __init at91_configure_usart1(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB20, 0);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PB21, 1);		/* RXD1 */

	if (pins & ATMEL_UART_RI)
		at91_set_A_periph(AT91_PIN_PB18, 0);	/* RI1 */
	if (pins & ATMEL_UART_DTR)
		at91_set_A_periph(AT91_PIN_PB19, 0);	/* DTR1 */
	if (pins & ATMEL_UART_DCD)
		at91_set_A_periph(AT91_PIN_PB23, 0);	/* DCD1 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PB24, 0);	/* CTS1 */
	if (pins & ATMEL_UART_DSR)
		at91_set_A_periph(AT91_PIN_PB25, 0);	/* DSR1 */
	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PB26, 0);	/* RTS1 */

	return AT91RM9200_BASE_US1;
}

resource_size_t __init at91_configure_usart2(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PA22, 1);		/* RXD2 */
	at91_set_A_periph(AT91_PIN_PA23, 0);		/* TXD2 */

	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PA30, 0);	/* CTS2 */
	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PA31, 0);	/* RTS2 */

	return AT91RM9200_BASE_US2;
}

resource_size_t __init at91_configure_usart3(unsigned pins)
{
	at91_set_B_periph(AT91_PIN_PA5, 0);		/* TXD3 */
	at91_set_B_periph(AT91_PIN_PA6, 1);		/* RXD3 */

	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PB1, 0);	/* CTS3 */
	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PB0, 0);	/* RTS3 */

	return AT91RM9200_BASE_US3;
}
