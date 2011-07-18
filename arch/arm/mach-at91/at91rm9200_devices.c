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
#include <asm/armlinux.h>
#include <mach/hardware.h>
#include <mach/at91rm9200.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/io.h>

#include "generic.h"

static struct resource sdram_dev_resources[] = {
	[0] = {
		.start	= AT91_CHIPSELECT_1,
	},
};

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = IORESOURCE_MEM_WRITEABLE,
};

static struct device_d sdram_dev = {
	.id = -1,
	.name = "mem",
	.num_resources	= ARRAY_SIZE(sdram_dev_resources),
	.resource	= sdram_dev_resources,
	.platform_data = &ram_pdata,
};

void at91_add_device_sdram(u32 size)
{
	sdram_dev_resources[0].size = size;
	register_device(&sdram_dev);
	armlinux_add_dram(&sdram_dev);
}

/* --------------------------------------------------------------------
 *  Ethernet
 * -------------------------------------------------------------------- */

#if defined(CONFIG_DRIVER_NET_AT91_ETHER)
static struct resource eth_resources[] = {
	[0] = {
		.start	= AT91_VA_BASE_EMAC,
		.size	= 0x1000,
	},
};

static struct device_d at91rm9200_eth_device = {
	.id		= 0,
	.name		= "at91_ether",
	.resource	= eth_resources,
	.num_resources	= ARRAY_SIZE(eth_resources),
};

void __init at91_add_device_eth(struct at91_ether_platform_data *data)
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

	if (!(data->flags & AT91SAM_ETHER_RMII)) {
		at91_set_B_periph(AT91_PIN_PB19, 0);	/* ERXCK */
		at91_set_B_periph(AT91_PIN_PB18, 0);	/* ECOL */
		at91_set_B_periph(AT91_PIN_PB17, 0);	/* ERXDV */
		at91_set_B_periph(AT91_PIN_PB16, 0);	/* ERX3 */
		at91_set_B_periph(AT91_PIN_PB15, 0);	/* ERX2 */
		at91_set_B_periph(AT91_PIN_PB14, 0);	/* ETXER */
		at91_set_B_periph(AT91_PIN_PB13, 0);	/* ETX3 */
		at91_set_B_periph(AT91_PIN_PB12, 0);	/* ETX2 */
	}

	at91rm9200_eth_device.platform_data = data;
	register_device(&at91rm9200_eth_device);
}
#else
void __init at91_add_device_eth(struct at91_ether_platform_data *data) {}
#endif

/* --------------------------------------------------------------------
 *  NAND / SmartMedia
 * -------------------------------------------------------------------- */

#if defined(CONFIG_NAND_ATMEL)
static struct resource nand_resources[] = {
	[0] = {
		.start	= AT91_CHIPSELECT_3,
		.size	= 0x10,
	},
};

static struct device_d at91rm9200_nand_device = {
	.id		= -1,
	.name		= "atmel_nand",
	.resource	= nand_resources,
	.num_resources	= ARRAY_SIZE(nand_resources),
};

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
	if (data->enable_pin)
		at91_set_gpio_output(data->enable_pin, 1);

	/* ready/busy pin */
	if (data->rdy_pin)
		at91_set_gpio_input(data->rdy_pin, 1);

	/* card detect pin */
	if (data->det_pin)
		at91_set_gpio_input(data->det_pin, 1);

	at91_set_A_periph(AT91_PIN_PC1, 0);		/* SMOE */
	at91_set_A_periph(AT91_PIN_PC3, 0);		/* SMWE */

	at91rm9200_nand_device.platform_data = data;
	platform_device_register(&at91rm9200_nand_device);
}
#else
void __init at91_add_device_nand(struct atmel_nand_data *data) {}
#endif

/* --------------------------------------------------------------------
 *  UART
 * -------------------------------------------------------------------- */

static struct resource dbgu_resources[] = {
	[0] = {
		.start	= AT91_BASE_SYS + AT91_DBGU,
		.size	= 4096,
	},
};

static struct device_d dbgu_serial_device = {
	.id		= 0,
	.name		= "atmel_serial",
	.resource	= dbgu_resources,
	.num_resources	= ARRAY_SIZE(dbgu_resources),
};

static inline void configure_dbgu_pins(void)
{
	at91_set_A_periph(AT91_PIN_PA30, 0);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PA31, 1);		/* DTXD */
}

static struct resource uart0_resources[] = {
	[0] = {
		.start	= AT91RM9200_BASE_US0,
		.size	= 4096,
	},
};

static struct device_d uart0_serial_device = {
	.id		= 1,
	.name		= "atmel_serial",
	.resource	= uart0_resources,
	.num_resources	= ARRAY_SIZE(uart0_resources),
};

static inline void configure_usart0_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PA17, 1);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PA18, 0);		/* RXD0 */

	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PA20, 0);	/* CTS0 */

	if (pins & ATMEL_UART_RTS) {
		/*
		 * AT91RM9200 Errata #39 - RTS0 is not internally connected to PA21.
		 *  We need to drive the pin manually.  Default is off (RTS is active low).
		 */
		at91_set_gpio_output(AT91_PIN_PA21, 1);
	}
}

static struct resource uart1_resources[] = {
	[0] = {
		.start	= AT91RM9200_BASE_US1,
		.size	= 4096,
	},
};

static struct device_d uart1_serial_device = {
	.id		= 2,
	.name		= "atmel_serial",
	.resource	= uart1_resources,
	.num_resources	= ARRAY_SIZE(uart1_resources),
};

static inline void configure_usart1_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PB20, 1);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PB21, 0);		/* RXD1 */

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
}

static struct resource uart2_resources[] = {
	[0] = {
		.start	= AT91RM9200_BASE_US2,
		.size	= 4096,
	},
};

static struct device_d uart2_serial_device = {
	.id		= 3,
	.name		= "atmel_serial",
	.resource	= uart2_resources,
	.num_resources	= ARRAY_SIZE(uart2_resources),
};

static inline void configure_usart2_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PA22, 0);		/* RXD2 */
	at91_set_A_periph(AT91_PIN_PA23, 1);		/* TXD2 */

	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PA30, 0);	/* CTS2 */
	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PA31, 0);	/* RTS2 */
}

static struct resource uart3_resources[] = {
	[0] = {
		.start	= AT91RM9200_BASE_US3,
		.size	= 4096,
	},
};

static struct device_d uart3_serial_device = {
	.id		= 4,
	.name		= "atmel_serial",
	.resource	= uart3_resources,
	.num_resources	= ARRAY_SIZE(uart3_resources),
};

static inline void configure_usart3_pins(unsigned pins)
{
	at91_set_B_periph(AT91_PIN_PA5, 1);		/* TXD3 */
	at91_set_B_periph(AT91_PIN_PA6, 0);		/* RXD3 */

	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PB1, 0);	/* CTS3 */
	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PB0, 0);	/* RTS3 */
}

void __init at91_register_uart(unsigned id, unsigned pins)
{
	switch (id) {
		case 0:		/* DBGU */
			configure_dbgu_pins();
			at91_clock_associate("mck", &dbgu_serial_device, "usart");
			register_device(&dbgu_serial_device);
			break;
		case AT91RM9200_ID_US0:
			configure_usart0_pins(pins);
			at91_clock_associate("usart0_clk", &uart0_serial_device, "usart");
			break;
		case AT91RM9200_ID_US1:
			configure_usart1_pins(pins);
			at91_clock_associate("usart1_clk", &uart1_serial_device, "usart");
			register_device(&uart1_serial_device);
			break;
		case AT91RM9200_ID_US2:
			configure_usart2_pins(pins);
			at91_clock_associate("usart2_clk", &uart2_serial_device, "usart");
			register_device(&uart2_serial_device);
			break;
		case AT91RM9200_ID_US3:
			configure_usart3_pins(pins);
			at91_clock_associate("usart3_clk", &uart3_serial_device, "usart");
			register_device(&uart3_serial_device);
			break;
		default:
			return;
	}

}
