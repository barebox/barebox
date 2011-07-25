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

#include "generic.h"

void at91_add_device_sdram(u32 size)
{
	struct device_d *sdram_dev;

	sdram_dev = add_mem_device("ram0", AT91_CHIPSELECT_1, size,
				   IORESOURCE_MEM_WRITEABLE);
	armlinux_add_dram(sdram_dev);
}

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

static inline void configure_dbgu_pins(void)
{
	at91_set_A_periph(AT91_PIN_PA9, 0);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PA10, 1);		/* DTXD */
}

static inline void configure_usart0_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PC8, 1);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PC9, 0);		/* RXD0 */

	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PC10, 0);	/* RTS0 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PC11, 0);	/* CTS0 */
}

static inline void configure_usart1_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PC12, 1);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PC13, 0);		/* RXD1 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PA12, 0);	/* RTS1 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PA13, 0);	/* CTS1 */
}

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
	resource_size_t start;
	struct device_d *dev;
	char* clk_name;

	switch (id) {
		case 0:		/* DBGU */
			configure_dbgu_pins();
			start = AT91_BASE_SYS + AT91_DBGU;
			clk_name = "mck";
			id = 0;
			break;
		case AT91SAM9261_ID_US0:
			configure_usart0_pins(pins);
			clk_name = "usart0_clk";
			start = AT91SAM9261_BASE_US0;
			id = 1;
			break;
		case AT91SAM9261_ID_US1:
			configure_usart1_pins(pins);
			clk_name = "usart1_clk";
			start = AT91SAM9261_BASE_US1;
			id = 2;
			break;
		case AT91SAM9261_ID_US2:
			configure_usart2_pins(pins);
			clk_name = "usart3_clk";
			start = AT91SAM9261_BASE_US2;
			id = 3;
			break;
		default:
			return;
	}

	dev = add_generic_device("atmel_serial", id, NULL, start, 4096,
			   IORESOURCE_MEM, NULL);
	at91_clock_associate(clk_name, dev, "usart");
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
	at91_clock_associate("mci_clk", dev, "mci_clk");
}
#else
void at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data) {}
#endif
