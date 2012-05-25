/*
 * arch/arm/mach-at91/at91sam9261_devices.c
 *
 *  Copyright (C) 2006 Atmel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <common.h>
#include <sizes.h>
#include <asm/armlinux.h>
#include <asm/hardware.h>
#include <mach/at91_pmc.h>
#include <mach/at91sam9261_matrix.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/io.h>
#include <mach/cpu.h>

#include "generic.h"

void at91_add_device_sdram(u32 size)
{
	arm_add_mem_device("ram0", AT91_CHIPSELECT_1, size);
	if (cpu_is_at91sam9g10())
		add_mem_device("sram0", AT91SAM9G10_SRAM_BASE,
			AT91SAM9G10_SRAM_SIZE, IORESOURCE_MEM_WRITEABLE);
	else
		add_mem_device("sram0", AT91SAM9261_SRAM_BASE,
			AT91SAM9261_SRAM_SIZE, IORESOURCE_MEM_WRITEABLE);
}

/* --------------------------------------------------------------------
 *  USB Host
 * -------------------------------------------------------------------- */

#if defined(CONFIG_USB_OHCI)
void __init at91_add_device_usbh_ohci(struct at91_usbh_data *data)
{
	if (!data)
		return;

	add_generic_device("at91_ohci", DEVICE_ID_DYNAMIC, NULL, AT91SAM9261_UHP_BASE,
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
	if (data->vbus_pin > 0) {
		at91_set_gpio_input(data->vbus_pin, 0);
		at91_set_deglitch(data->vbus_pin, 1);
	}

	add_generic_device("at91_udc", DEVICE_ID_DYNAMIC, NULL, AT91SAM9261_BASE_UDP,
			SZ_16K, IORESOURCE_MEM, data);
}
#else
void __init at91_add_device_udc(struct at91_udc_data *data) {}
#endif

#if defined(CONFIG_NAND_ATMEL)
void at91_add_device_nand(struct atmel_nand_data *data)
{
	unsigned long csa;

	if (!data)
		return;

	csa = at91_sys_read(AT91_MATRIX_EBICSA);
	at91_sys_write(AT91_MATRIX_EBICSA, csa | AT91_MATRIX_CS3A_SMC_SMARTMEDIA);

	/* enable pin */
	if (data->enable_pin)
		at91_set_gpio_output(data->enable_pin, 1);

	/* ready/busy pin */
	if (data->rdy_pin)
		at91_set_gpio_input(data->rdy_pin, 1);

	/* card detect pin */
	if (data->det_pin)
		at91_set_gpio_input(data->det_pin, 1);

	at91_set_A_periph(AT91_PIN_PC0, 0);		/* NANDOE */
	at91_set_A_periph(AT91_PIN_PC1, 0);		/* NANDWE */

	add_generic_device("atmel_nand", 0, NULL, AT91_CHIPSELECT_3, 0x10,
			   IORESOURCE_MEM, data);
}
#else
void at91_add_device_nand(struct atmel_nand_data *data) {}
#endif

/* --------------------------------------------------------------------
 *  SPI
 * -------------------------------------------------------------------- */

#if defined(CONFIG_DRIVER_SPI_ATMEL)
static const unsigned spi0_standard_cs[4] = { AT91_PIN_PA3, AT91_PIN_PA4, AT91_PIN_PA5, AT91_PIN_PA6 };

static const unsigned spi1_standard_cs[4] = { AT91_PIN_PB28, AT91_PIN_PA24, AT91_PIN_PA25, AT91_PIN_PA26 };

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
		if (cs_pin > 0)
			at91_set_gpio_output(cs_pin, 1);
	}

	/* Configure SPI bus(es) */
	switch (spi_id) {
	case 0:
		start = AT91SAM9261_BASE_SPI0;
		at91_set_A_periph(AT91_PIN_PA0, 0);	/* SPI0_MISO */
		at91_set_A_periph(AT91_PIN_PA1, 0);	/* SPI0_MOSI */
		at91_set_A_periph(AT91_PIN_PA2, 0);	/* SPI0_SPCK */
		break;
	case 1:
		start = AT91SAM9213_BASE_SPI1;
		at91_set_A_periph(AT91_PIN_PB30, 0);	/* SPI1_MISO */
		at91_set_A_periph(AT91_PIN_PB31, 0);	/* SPI1_MOSI */
		at91_set_A_periph(AT91_PIN_PB29, 0);	/* SPI1_SPCK */
		break;
	}

	add_generic_device("atmel_spi", spi_id, NULL, start, SZ_16K,
			   IORESOURCE_MEM, pdata);
}
#else
void __init at91_add_device_spi(int spi_id, struct at91_spi_platform_data *pdata) {}
#endif

resource_size_t __init at91_configure_dbgu(void)
{
	at91_set_A_periph(AT91_PIN_PA9, 0);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PA10, 1);		/* DTXD */

	return AT91_BASE_SYS + AT91_DBGU;
}

resource_size_t __init at91_configure_usart0(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PC8, 1);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PC9, 0);		/* RXD0 */

	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PC10, 0);	/* RTS0 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PC11, 0);	/* CTS0 */

	return AT91SAM9261_BASE_US0;
}

resource_size_t __init at91_configure_usart1(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PC12, 1);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PC13, 0);		/* RXD1 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PA12, 0);	/* RTS1 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PA13, 0);	/* CTS1 */

	return AT91SAM9261_BASE_US1;
}

resource_size_t __init at91_configure_usart2(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PC15, 0);		/* RXD2 */
	at91_set_A_periph(AT91_PIN_PC14, 1);		/* TXD2 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PA15, 0);	/* RTS2*/
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PA16, 0);	/* CTS2 */

	return AT91SAM9261_BASE_US2;
}

#if defined(CONFIG_MCI_ATMEL)
/* Consider only one slot : slot 0 */
void at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data)
{
	struct device_d *dev;

	if (!data)
		return;

	/* need bus_width */
	if (!data->bus_width)
		return;

	/* input/irq */
	if (data->detect_pin) {
		at91_set_gpio_input(data->detect_pin, 1);
		at91_set_deglitch(data->detect_pin, 1);
	}

	if (data->wp_pin)
		at91_set_gpio_input(data->wp_pin, 1);

	/* CLK */
	at91_set_B_periph(AT91_PIN_PA2, 0);

	/* CMD */
	at91_set_B_periph(AT91_PIN_PA1, 1);

	/* DAT0, maybe DAT1..DAT3 */
	at91_set_B_periph(AT91_PIN_PA0, 1);
	if (data->bus_width == 4) {
		at91_set_B_periph(AT91_PIN_PA4, 1);
		at91_set_B_periph(AT91_PIN_PA5, 1);
		at91_set_B_periph(AT91_PIN_PA6, 1);
	}

	dev = add_generic_device("atmel_mci", 0, NULL, AT91SAM9261_BASE_MCI, SZ_16K,
			   IORESOURCE_MEM, data);
}
#else
void at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data) {}
#endif
