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
#include <asm/armlinux.h>
#include <asm/hardware.h>
#include <mach/at91_pmc.h>
#include <mach/at91sam9261_matrix.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/io.h>

#include "generic.h"

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.id		= -1,
	.name		= "mem",
	.map_base	= AT91_CHIPSELECT_1,
	.platform_data	= &ram_pdata,
};

void at91_add_device_sdram(u32 size)
{
	sdram_dev.size = size;
	register_device(&sdram_dev);
	armlinux_add_dram(&sdram_dev);
}

#if defined(CONFIG_NAND_ATMEL)
static struct device_d nand_dev = {
	.id		= 0,
	.name		= "atmel_nand",
	.map_base	= AT91_CHIPSELECT_3,
	.size		= 0x10,
};

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

	nand_dev.platform_data = data;
	register_device(&nand_dev);
}
#else
void at91_add_device_nand(struct atmel_nand_data *data) {}
#endif

static struct device_d dbgu_serial_device = {
	.id		= 0,
	.name		= "atmel_serial",
	.map_base	= (AT91_BASE_SYS + AT91_DBGU),
	.size		= 4096,
};

static inline void configure_dbgu_pins(void)
{
	at91_set_A_periph(AT91_PIN_PA9, 0);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PA10, 1);		/* DTXD */
}

static struct device_d uart0_serial_device = {
	.id		= 1,
	.name		= "atmel_serial",
	.map_base	= AT91SAM9261_BASE_US0,
	.size		= 4096,
};

static inline void configure_usart0_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PC8, 1);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PC9, 0);		/* RXD0 */

	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PC10, 0);	/* RTS0 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PC11, 0);	/* CTS0 */
}

static struct device_d uart1_serial_device = {
	.id		= 2,
	.name		= "atmel_serial",
	.map_base	= AT91SAM9261_BASE_US1,
	.size		= 4096,
};

static inline void configure_usart1_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PC12, 1);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PC13, 0);		/* RXD1 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PA12, 0);	/* RTS1 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PA13, 0);	/* CTS1 */
}

static struct device_d uart2_serial_device = {
	.id		= 3,
	.name		= "atmel_serial",
	.map_base	= AT91SAM9261_BASE_US2,
	.size		= 4096,
};

static inline void configure_usart2_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PC15, 0);		/* RXD2 */
	at91_set_A_periph(AT91_PIN_PC14, 1);		/* TXD2 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PA15, 0);	/* RTS2*/
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PA16, 0);	/* CTS2 */
}

void at91_register_uart(unsigned id, unsigned pins)
{
	switch (id) {
		case 0:		/* DBGU */
			configure_dbgu_pins();
			at91_clock_associate("mck", &dbgu_serial_device, "usart");
			register_device(&dbgu_serial_device);
			break;
		case AT91SAM9261_ID_US0:
			configure_usart0_pins(pins);
			at91_clock_associate("usart0_clk", &uart0_serial_device, "usart");
			register_device(&uart0_serial_device);
			break;
		case AT91SAM9261_ID_US1:
			configure_usart1_pins(pins);
			at91_clock_associate("usart1_clk", &uart1_serial_device, "usart");
			register_device(&uart1_serial_device);
			break;
		case AT91SAM9261_ID_US2:
			configure_usart2_pins(pins);
			at91_clock_associate("usart2_clk", &uart2_serial_device, "usart");
			register_device(&uart2_serial_device);
			break;
		default:
			return;
	}
}
