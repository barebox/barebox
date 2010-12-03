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
#include <mach/at91sam9g45_matrix.h>
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
	.map_base	= AT91_CHIPSELECT_6,
	.platform_data	= &ram_pdata,
};

void at91_add_device_sdram(u32 size)
{
	sdram_dev.size = size;
	register_device(&sdram_dev);
	armlinux_add_dram(&sdram_dev);
}

#if defined(CONFIG_DRIVER_NET_MACB)
static struct device_d macb_dev = {
	.id		= 0,
	.name		= "macb",
	.map_base	= AT91SAM9G45_BASE_EMAC,
	.size		= 0x1000,
};

void at91_add_device_eth(struct at91_ether_platform_data *data)
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

	macb_dev.platform_data = data;
	register_device(&macb_dev);
}
#else
void at91_add_device_eth(struct at91_ether_platform_data *data) {}
#endif

#if defined(CONFIG_NAND_ATMEL)
static struct device_d nand_dev = {
	.id		= -1,
	.name		= "atmel_nand",
	.map_base	= AT91_CHIPSELECT_3,
	.size		= 0x10,
};

void at91_add_device_nand(struct atmel_nand_data *data)
{
	unsigned long csa;

	if (!data)
		return;

	data->ecc_base = (void __iomem *)(AT91_BASE_SYS + AT91_ECC);
	data->ecc_mode = NAND_ECC_HW;

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

	nand_dev.platform_data = data;
	register_device(&nand_dev);
}
#else
void at91_add_device_nand(struct atmel_nand_data *data) {}
#endif

static struct device_d dbgu_serial_device = {
	.id	  = -1,
	.name     = "atmel_serial",
	.map_base = (AT91_BASE_SYS + AT91_DBGU),
	.size     = 4096,
};

static inline void configure_dbgu_pins(void)
{
	at91_set_A_periph(AT91_PIN_PB12, 0);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PB13, 1);		/* DTXD */
}

static struct device_d uart0_serial_device = {
	.id	  = -1,
	.name     = "atmel_serial",
	.map_base = AT91SAM9G45_BASE_US0,
	.size     = 4096,
};

static inline void configure_usart0_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB19, 1);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PB18, 0);		/* RXD0 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PB17, 0);	/* RTS0 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PB15, 0);	/* CTS0 */
}

static struct device_d uart1_serial_device = {
	.id	  = -1,
	.name     = "atmel_serial",
	.map_base = AT91SAM9G45_BASE_US1,
	.size     = 4096,
};

static inline void configure_usart1_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB4, 1);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PB5, 0);		/* RXD1 */

	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PD16, 0);	/* RTS1 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PD17, 0);	/* CTS1 */
}

static struct device_d uart2_serial_device = {
	.id	  = -1,
	.name     = "atmel_serial",
	.map_base = AT91SAM9G45_BASE_US2,
	.size     = 4096,
};

static inline void configure_usart2_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB6, 1);		/* TXD2 */
	at91_set_A_periph(AT91_PIN_PB7, 0);		/* RXD2 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PC9, 0);	/* RTS2 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PC11, 0);	/* CTS2 */
}

static struct device_d uart3_serial_device = {
	.id	  = -1,
	.name     = "atmel_serial",
	.map_base = AT91SAM9G45_ID_US3,
	.size     = 4096,
};

static inline void configure_usart3_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB8, 1);		/* TXD3 */
	at91_set_A_periph(AT91_PIN_PB9, 0);		/* RXD3 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PA23, 0);	/* RTS3 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PA24, 0);	/* CTS3 */
}

void at91_register_uart(unsigned id, unsigned pins)
{
	switch (id) {
		case 0:		/* DBGU */
			configure_dbgu_pins();
			at91_clock_associate("mck", &dbgu_serial_device, "usart");
			register_device(&dbgu_serial_device);
			break;
		case AT91SAM9G45_ID_US0:
			configure_usart0_pins(pins);
			at91_clock_associate("usart0_clk", &uart0_serial_device, "usart");
			register_device(&uart0_serial_device);
			break;
		case AT91SAM9G45_ID_US1:
			configure_usart1_pins(pins);
			at91_clock_associate("usart1_clk", &uart1_serial_device, "usart");
			register_device(&uart1_serial_device);
			break;
		case AT91SAM9G45_ID_US2:
			configure_usart2_pins(pins);
			at91_clock_associate("usart2_clk", &uart2_serial_device, "usart");
			register_device(&uart2_serial_device);
			break;
		case AT91SAM9G45_ID_US3:
			configure_usart3_pins(pins);
			at91_clock_associate("usart3_clk", &uart2_serial_device, "usart");
			register_device(&uart3_serial_device);
			break;
		default:
			return;
	}

}
