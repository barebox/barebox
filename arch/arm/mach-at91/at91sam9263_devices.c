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
#include <asm/armlinux.h>
#include <asm/hardware.h>
#include <mach/at91_pmc.h>
#include <mach/at91sam9263_matrix.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/io.h>

#include "generic.h"

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = AT91_CHIPSELECT_1,
	.platform_data = &ram_pdata,
};

void at91_add_device_sdram(u32 size)
{
	sdram_dev.size = size;
	register_device(&sdram_dev);
	armlinux_add_dram(&sdram_dev);
}

#if defined(CONFIG_DRIVER_NET_MACB)
static struct device_d macb_dev = {
	.id	  = -1,
	.name     = "macb",
	.map_base = AT91SAM9263_BASE_EMAC,
	.size     = 0x1000,
};

void at91_add_device_eth(struct at91_ether_platform_data *data)
{
	if (!data)
		return;

	at91_set_A_periph(AT91_PIN_PE21, 0);	/* ETXCK_EREFCK */
	at91_set_B_periph(AT91_PIN_PC25, 0);	/* ERXDV */
	at91_set_A_periph(AT91_PIN_PE25, 0);	/* ERX0 */
	at91_set_A_periph(AT91_PIN_PE26, 0);	/* ERX1 */
	at91_set_A_periph(AT91_PIN_PE27, 0);	/* ERXER */
	at91_set_A_periph(AT91_PIN_PE28, 0);	/* ETXEN */
	at91_set_A_periph(AT91_PIN_PE23, 0);	/* ETX0 */
	at91_set_A_periph(AT91_PIN_PE24, 0);	/* ETX1 */
	at91_set_A_periph(AT91_PIN_PE30, 0);	/* EMDIO */
	at91_set_A_periph(AT91_PIN_PE29, 0);	/* EMDC */

	if (!(data->flags & AT91SAM_ETHER_RMII)) {
		at91_set_A_periph(AT91_PIN_PE22, 0);	/* ECRS */
		at91_set_B_periph(AT91_PIN_PC26, 0);	/* ECOL */
		at91_set_B_periph(AT91_PIN_PC22, 0);	/* ERX2 */
		at91_set_B_periph(AT91_PIN_PC23, 0);	/* ERX3 */
		at91_set_B_periph(AT91_PIN_PC27, 0);	/* ERXCK */
		at91_set_B_periph(AT91_PIN_PC20, 0);	/* ETX2 */
		at91_set_B_periph(AT91_PIN_PC21, 0);	/* ETX3 */
		at91_set_B_periph(AT91_PIN_PC24, 0);	/* ETXER */
	}

	macb_dev.platform_data = data;
	register_device(&macb_dev);
}
#else
void at91_add_device_eth(struct at91_ether_platform_data *data) {}
#endif

#if defined(CONFIG_NAND_ATMEL)
static struct device_d nand_dev = {
	.id	  = -1,
	.name     = "atmel_nand",
	.map_base = AT91_CHIPSELECT_3,
	.size     = 0x10,
};

void at91_add_device_nand(struct atmel_nand_data *data)
{
	unsigned long csa;

	if (!data)
		return;

	csa = at91_sys_read(AT91_MATRIX_EBI0CSA);
	at91_sys_write(AT91_MATRIX_EBI0CSA, csa | AT91_MATRIX_EBI0_CS3A_SMC_SMARTMEDIA);

	/* enable pin */
	if (data->enable_pin)
		at91_set_gpio_output(data->enable_pin, 1);

	/* ready/busy pin */
	if (data->rdy_pin)
		at91_set_gpio_input(data->rdy_pin, 1);

	/* card detect pin */
	if (data->det_pin)
		at91_set_gpio_input(data->det_pin, 1);

	nand_dev.platform_data = data;
	register_device(&nand_dev);
}
#else
void at91_add_device_nand(struct atmel_nand_data *data) {}
#endif

static struct device_d dbgu_serial_device = {
	.id	  = 0,
	.name     = "atmel_serial",
	.map_base = (AT91_BASE_SYS + AT91_DBGU),
	.size     = 4096,
};

static inline void configure_dbgu_pins(void)
{
	at91_set_A_periph(AT91_PIN_PC30, 0);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PC31, 1);		/* DTXD */
}

static struct device_d uart0_serial_device = {
	.id	  = 1,
	.name     = "atmel_serial",
	.map_base = AT91SAM9263_BASE_US0,
	.size     = 4096,
};

static inline void configure_usart0_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PA26, 1);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PA27, 0);		/* RXD0 */

	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PA28, 0);	/* RTS0 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PA29, 0);	/* CTS0 */
}

static struct device_d uart1_serial_device = {
	.id	  = 2,
	.name     = "atmel_serial",
	.map_base = AT91SAM9263_BASE_US1,
	.size     = 4096,
};

static inline void configure_usart1_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PD0, 1);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PD1, 0);		/* RXD1 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PD7, 0);	/* RTS1 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PD8, 0);	/* CTS1 */
}

static struct device_d uart2_serial_device = {
	.id	  = 3,
	.name     = "atmel_serial",
	.map_base = AT91SAM9263_BASE_US2,
	.size     = 4096,
};

static inline void configure_usart2_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PD2, 1);		/* TXD2 */
	at91_set_A_periph(AT91_PIN_PD3, 0);		/* RXD2 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PD5, 0);	/* RTS2 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PD6, 0);	/* CTS2 */
}

void at91_register_uart(unsigned id, unsigned pins)
{
	switch (id) {
		case 0:		/* DBGU */
			configure_dbgu_pins();
			at91_clock_associate("mck", &dbgu_serial_device, "usart");
			register_device(&dbgu_serial_device);
			break;
		case AT91SAM9263_ID_US0:
			configure_usart0_pins(pins);
			at91_clock_associate("usart0_clk", &uart0_serial_device, "usart");
			register_device(&uart0_serial_device);
			break;
		case AT91SAM9263_ID_US1:
			configure_usart1_pins(pins);
			at91_clock_associate("usart1_clk", &uart1_serial_device, "usart");
			register_device(&uart1_serial_device);
			break;
		case AT91SAM9263_ID_US2:
			configure_usart2_pins(pins);
			at91_clock_associate("usart2_clk", &uart2_serial_device, "usart");
			register_device(&uart2_serial_device);
			break;
		default:
			return;
	}

}
