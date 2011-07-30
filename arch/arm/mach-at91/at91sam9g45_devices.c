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
}

#if defined(CONFIG_DRIVER_NET_MACB)
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

	add_generic_device("macb", 0, NULL, AT91SAM9G45_BASE_EMAC, 0x1000,
			   IORESOURCE_MEM, data);
}
#else
void at91_add_device_eth(struct at91_ether_platform_data *data) {}
#endif

#if defined(CONFIG_NAND_ATMEL)
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

	add_generic_device("atmel_nand", -1, NULL, AT91_CHIPSELECT_3, 0x10,
			   IORESOURCE_MEM, data);
}
#else
void at91_add_device_nand(struct atmel_nand_data *data) {}
#endif

static inline void configure_dbgu_pins(void)
{
	at91_set_A_periph(AT91_PIN_PB12, 0);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PB13, 1);		/* DTXD */
}

static inline void configure_usart0_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB19, 1);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PB18, 0);		/* RXD0 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PB17, 0);	/* RTS0 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PB15, 0);	/* CTS0 */
}

static inline void configure_usart1_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB4, 1);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PB5, 0);		/* RXD1 */

	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PD16, 0);	/* RTS1 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PD17, 0);	/* CTS1 */
}

static inline void configure_usart2_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB6, 1);		/* TXD2 */
	at91_set_A_periph(AT91_PIN_PB7, 0);		/* RXD2 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PC9, 0);	/* RTS2 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PC11, 0);	/* CTS2 */
}

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
		case AT91SAM9G45_ID_US0:
			configure_usart0_pins(pins);
			clk_name = "usart0_clk";
			start = AT91SAM9G45_BASE_US0;
			id = 1;
			break;
		case AT91SAM9G45_ID_US1:
			configure_usart1_pins(pins);
			clk_name = "usart1_clk";
			start = AT91SAM9G45_BASE_US1;
			id = 2;
			break;
		case AT91SAM9G45_ID_US2:
			configure_usart2_pins(pins);
			clk_name = "usart2_clk";
			start = AT91SAM9G45_BASE_US2;
			id = 3;
			break;
		case AT91SAM9G45_ID_US3:
			configure_usart3_pins(pins);
			clk_name = "usart3_clk";
			start = AT91SAM9G45_BASE_US3;
			id = 4;
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
	resource_size_t start;
	struct device_d *dev;
	char* clk_name;

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
		clk_name = "mci0_clk";
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
		start = AT91SAM9G45_BASE_MCI1;
		clk_name = "mci1_clk";
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

	dev = add_generic_device("atmel_mci", mmc_id, NULL, start, 4096,
			   IORESOURCE_MEM, data);
	at91_clock_associate(clk_name, dev, "mci_clk");
}
#else
void at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data) {}
#endif

