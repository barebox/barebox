/*
 * arch/arm/mach-at91/at91sam9263_devices.c
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
#include <mach/at91sam9g45_matrix.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/io.h>

#include "generic.h"

void at91_add_device_sdram(u32 size)
{
	arm_add_mem_device("ram0", AT91_CHIPSELECT_6, size);
	add_mem_device("sram0", AT91SAM9G45_SRAM_BASE,
			AT91SAM9G45_SRAM_SIZE, IORESOURCE_MEM_WRITEABLE);
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
		if (data->vbus_pin[i])
			at91_set_gpio_output(data->vbus_pin[i], 0);
	}

	add_generic_device("at91_ohci", DEVICE_ID_DYNAMIC, NULL, AT91SAM9G45_OHCI_BASE,
			1024 * 1024, IORESOURCE_MEM, data);
}
#else
void __init at91_add_device_usbh_ohci(struct at91_usbh_data *data) {}
#endif

#if defined(CONFIG_DRIVER_NET_MACB)
void at91_add_device_eth(int id, struct at91_ether_platform_data *data)
{
	if (!data)
		return;

	/* Pins used for MII and RMII */
	at91_set_A_periph(AT91_PIN_PA17, 0);	/* ETXCK_EREFCK */
	at91_set_A_periph(AT91_PIN_PA15, 0);	/* ERXDV */
	at91_set_A_periph(AT91_PIN_PA12, 0);	/* ERX0 */
	at91_set_A_periph(AT91_PIN_PA13, 0);	/* ERX1 */
	at91_set_A_periph(AT91_PIN_PA16, 0);	/* ERXER */
	at91_set_A_periph(AT91_PIN_PA14, 0);	/* ETXEN */
	at91_set_A_periph(AT91_PIN_PA10, 0);	/* ETX0 */
	at91_set_A_periph(AT91_PIN_PA11, 0);	/* ETX1 */
	at91_set_A_periph(AT91_PIN_PA19, 0);	/* EMDIO */
	at91_set_A_periph(AT91_PIN_PA18, 0);	/* EMDC */

	if (!(data->flags & AT91SAM_ETHER_RMII)) {
		at91_set_B_periph(AT91_PIN_PA29, 0);	/* ECRS */
		at91_set_B_periph(AT91_PIN_PA30, 0);	/* ECOL */
		at91_set_B_periph(AT91_PIN_PA8,  0);	/* ERX2 */
		at91_set_B_periph(AT91_PIN_PA9,  0);	/* ERX3 */
		at91_set_B_periph(AT91_PIN_PA28, 0);	/* ERXCK */
		at91_set_B_periph(AT91_PIN_PA6,  0);	/* ETX2 */
		at91_set_B_periph(AT91_PIN_PA7,  0);	/* ETX3 */
		at91_set_B_periph(AT91_PIN_PA27, 0);	/* ETXER */
	}

	add_generic_device("macb", 0, NULL, AT91SAM9G45_BASE_EMAC, 0x1000,
			   IORESOURCE_MEM, data);
}
#else
void at91_add_device_eth(int id, struct at91_ether_platform_data *data) {}
#endif

#if defined(CONFIG_NAND_ATMEL)
static struct resource nand_resources[] = {
	[0] = {
		.start	= AT91_CHIPSELECT_3,
		.end	= AT91_CHIPSELECT_3 + SZ_256M - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= AT91_BASE_SYS + AT91_ECC,
		.end	= AT91_BASE_SYS + AT91_ECC + 512 - 1,
		.flags	= IORESOURCE_MEM,
	}
};

void at91_add_device_nand(struct atmel_nand_data *data)
{
	unsigned long csa;

	if (!data)
		return;

	csa = at91_sys_read(AT91_MATRIX_EBICSA);
	at91_sys_write(AT91_MATRIX_EBICSA, csa | AT91_MATRIX_EBI_CS3A_SMC_SMARTMEDIA);

	/* enable pin */
	if (data->enable_pin)
		at91_set_gpio_output(data->enable_pin, 1);

	/* ready/busy pin */
	if (data->rdy_pin)
		at91_set_gpio_input(data->rdy_pin, 1);

	/* card detect pin */
	if (data->det_pin)
		at91_set_gpio_input(data->det_pin, 1);

	add_generic_device_res("atmel_nand", DEVICE_ID_DYNAMIC, nand_resources,
			       ARRAY_SIZE(nand_resources), data);
}
#else
void at91_add_device_nand(struct atmel_nand_data *data) {}
#endif

resource_size_t __init at91_configure_dbgu(void)
{
	at91_set_A_periph(AT91_PIN_PB12, 0);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PB13, 1);		/* DTXD */

	return AT91_BASE_SYS + AT91_DBGU;
}

resource_size_t __init at91_configure_usart0(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB19, 1);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PB18, 0);		/* RXD0 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PB17, 0);	/* RTS0 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PB15, 0);	/* CTS0 */

	return AT91SAM9G45_BASE_US0;
}

resource_size_t __init at91_configure_usart1(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB4, 1);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PB5, 0);		/* RXD1 */

	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PD16, 0);	/* RTS1 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PD17, 0);	/* CTS1 */

	return AT91SAM9G45_BASE_US1;
}

resource_size_t __init at91_configure_usart2(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB6, 1);		/* TXD2 */
	at91_set_A_periph(AT91_PIN_PB7, 0);		/* RXD2 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PC9, 0);	/* RTS2 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PC11, 0);	/* CTS2 */

	return AT91SAM9G45_BASE_US2;
}

resource_size_t __init at91_configure_usart3(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB8, 1);		/* TXD3 */
	at91_set_A_periph(AT91_PIN_PB9, 0);		/* RXD3 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PA23, 0);	/* RTS3 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PA24, 0);	/* CTS3 */

	return AT91SAM9G45_BASE_US3;
}

#if defined(CONFIG_MCI_ATMEL)
/* Consider only one slot : slot 0 */
void at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data)
{
	resource_size_t start;

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

	if (mmc_id == 0) {		/* MCI0 */
		start = AT91SAM9G45_BASE_MCI0;
		/* CLK */
		at91_set_A_periph(AT91_PIN_PA0, 0);

		/* CMD */
		at91_set_A_periph(AT91_PIN_PA1, 1);

		/* DAT0, maybe DAT1..DAT3 and maybe DAT4..DAT7 */
		at91_set_A_periph(AT91_PIN_PA2, 1);
		if (data->bus_width >= 4) {
			at91_set_A_periph(AT91_PIN_PA3, 1);
			at91_set_A_periph(AT91_PIN_PA4, 1);
			at91_set_A_periph(AT91_PIN_PA5, 1);
			if (data->bus_width == 8) {
				at91_set_A_periph(AT91_PIN_PA6, 1);
				at91_set_A_periph(AT91_PIN_PA7, 1);
				at91_set_A_periph(AT91_PIN_PA8, 1);
				at91_set_A_periph(AT91_PIN_PA9, 1);
			}
		}
	} else {			/* MCI1 */
		data->slot_b = 1;
		start = AT91SAM9G45_BASE_MCI1;
		/* CLK */
		at91_set_A_periph(AT91_PIN_PA31, 0);

		/* CMD */
		at91_set_A_periph(AT91_PIN_PA22, 1);

		/* DAT0, maybe DAT1..DAT3 and maybe DAT4..DAT7 */
		at91_set_A_periph(AT91_PIN_PA23, 1);
		if (data->bus_width >= 4) {
			at91_set_A_periph(AT91_PIN_PA24, 1);
			at91_set_A_periph(AT91_PIN_PA25, 1);
			at91_set_A_periph(AT91_PIN_PA26, 1);
			if (data->bus_width == 8) {
				at91_set_A_periph(AT91_PIN_PA27, 1);
				at91_set_A_periph(AT91_PIN_PA28, 1);
				at91_set_A_periph(AT91_PIN_PA29, 1);
				at91_set_A_periph(AT91_PIN_PA30, 1);
			}
		}
	}

	add_generic_device("atmel_mci", mmc_id, NULL, start, 4096,
			   IORESOURCE_MEM, data);
}
#else
void at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data) {}
#endif

#if defined(CONFIG_DRIVER_SPI_ATMEL)
/* SPI */
static unsigned spi0_standard_cs[4] = { AT91_PIN_PB3, AT91_PIN_PB18, AT91_PIN_PB19, AT91_PIN_PD27 };

static unsigned spi1_standard_cs[4] = { AT91_PIN_PB17, AT91_PIN_PD28, AT91_PIN_PD18, AT91_PIN_PD19 };

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
		start = AT91SAM9G45_BASE_SPI0;
		at91_set_A_periph(AT91_PIN_PB0, 0);	/* SPI0_MISO */
		at91_set_A_periph(AT91_PIN_PB1, 0);	/* SPI0_MOSI */
		at91_set_A_periph(AT91_PIN_PB2, 0);	/* SPI0_SPCK */
		break;
	case 1:
		start = AT91SAM9G45_BASE_SPI1;
		at91_set_A_periph(AT91_PIN_PB14, 0);	/* SPI1_MISO */
		at91_set_A_periph(AT91_PIN_PB15, 0);	/* SPI1_MOSI */
		at91_set_A_periph(AT91_PIN_PB16, 0);	/* SPI1_SPCK */
		break;
	}

	add_generic_device("atmel_spi", spi_id, NULL, start, SZ_16K,
			   IORESOURCE_MEM, pdata);
}
#else
void at91_add_device_spi(int spi_id, struct at91_spi_platform_data *pdata) {}
#endif
