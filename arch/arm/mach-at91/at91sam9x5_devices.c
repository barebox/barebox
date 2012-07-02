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
#include <sizes.h>
#include <asm/armlinux.h>
#include <asm/hardware.h>
#include <mach/board.h>
#include <mach/at91_pmc.h>
#include <mach/at91sam9x5_matrix.h>
#include <mach/gpio.h>
#include <mach/io.h>
#include <mach/cpu.h>

#include "generic.h"

void at91_add_device_sdram(u32 size)
{
	arm_add_mem_device("ram0", AT91_CHIPSELECT_1, size);
	add_mem_device("sram0", AT91SAM9X5_SRAM_BASE,
			AT91SAM9X5_SRAM_SIZE, IORESOURCE_MEM_WRITEABLE);
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

	add_generic_device("at91_ohci", DEVICE_ID_DYNAMIC, NULL, AT91SAM9X5_OHCI_BASE,
			SZ_1M, IORESOURCE_MEM, data);
}
#else
void __init at91_add_device_usbh_ohci(struct at91_usbh_data *data) {}
#endif

#if defined(CONFIG_DRIVER_NET_MACB)
void at91_add_device_eth(int id, struct at91_ether_platform_data *data)
{
	resource_size_t start;

	if (!data)
		return;

	if (cpu_is_at91sam9g15())
		return;

	if (id && !cpu_is_at91sam9x25())
		return;

	switch (id) {
	case 0:
		start = AT91SAM9X5_BASE_EMAC0;
		/* Pins used for MII and RMII */
		at91_set_A_periph(AT91_PIN_PB4,  0);	/* ETXCK_EREFCK */
		at91_set_A_periph(AT91_PIN_PB3,  0);	/* ERXDV */
		at91_set_A_periph(AT91_PIN_PB0,  0);	/* ERX0 */
		at91_set_A_periph(AT91_PIN_PB1,  0);	/* ERX1 */
		at91_set_A_periph(AT91_PIN_PB2,  0);	/* ERXER */
		at91_set_A_periph(AT91_PIN_PB7,  0);	/* ETXEN */
		at91_set_A_periph(AT91_PIN_PB9,  0);	/* ETX0 */
		at91_set_A_periph(AT91_PIN_PB10, 0);	/* ETX1 */
		at91_set_A_periph(AT91_PIN_PB5,  0);	/* EMDIO */
		at91_set_A_periph(AT91_PIN_PB6,  0);	/* EMDC */

		if (!(data->flags & AT91SAM_ETHER_RMII)) {
			at91_set_A_periph(AT91_PIN_PB16, 0);	/* ECRS */
			at91_set_A_periph(AT91_PIN_PB17, 0);	/* ECOL */
			at91_set_A_periph(AT91_PIN_PB13, 0);	/* ERX2 */
			at91_set_A_periph(AT91_PIN_PB14, 0);	/* ERX3 */
			at91_set_A_periph(AT91_PIN_PB15, 0);	/* ERXCK */
			at91_set_A_periph(AT91_PIN_PB11, 0);	/* ETX2 */
			at91_set_A_periph(AT91_PIN_PB12, 0);	/* ETX3 */
			at91_set_A_periph(AT91_PIN_PB8,  0);	/* ETXER */
		}
		break;
	case 1:
		start = AT91SAM9X5_BASE_EMAC1;
		if (!(data->flags & AT91SAM_ETHER_RMII))
			pr_warn("AT91: Only RMII available on interface macb%d.\n", id);

		/* Pins used for RMII */
		at91_set_B_periph(AT91_PIN_PC29,  0);	/* ETXCK_EREFCK */
		at91_set_B_periph(AT91_PIN_PC28,  0);	/* ECRSDV */
		at91_set_B_periph(AT91_PIN_PC20,  0);	/* ERX0 */
		at91_set_B_periph(AT91_PIN_PC21,  0);	/* ERX1 */
		at91_set_B_periph(AT91_PIN_PC16,  0);	/* ERXER */
		at91_set_B_periph(AT91_PIN_PC27,  0);	/* ETXEN */
		at91_set_B_periph(AT91_PIN_PC18,  0);	/* ETX0 */
		at91_set_B_periph(AT91_PIN_PC19,  0);	/* ETX1 */
		at91_set_B_periph(AT91_PIN_PC31,  0);	/* EMDIO */
		at91_set_B_periph(AT91_PIN_PC30,  0);	/* EMDC */
		break;
	default:
		return;
	}

	add_generic_device("macb", id, NULL, start, SZ_16K,
			   IORESOURCE_MEM, data);
}
#else
void at91_add_device_eth(int id, struct at91_ether_platform_data *data) {}
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
		.start	= AT91_BASE_SYS + AT91_PMECC,
		.end	= AT91_BASE_SYS + AT91_PMECC + 512 - 1,
		.flags	= IORESOURCE_MEM,
	}
};

void __init at91_add_device_nand(struct atmel_nand_data *data)
{
	unsigned long csa;

	if (!data)
		return;

	csa = at91_sys_read(AT91_MATRIX_EBICSA);
	at91_sys_write(AT91_MATRIX_EBICSA, csa | AT91_MATRIX_EBI_CS3A_SMC_NANDFLASH);

	/* enable pin */
	if (data->enable_pin)
		at91_set_gpio_output(data->enable_pin, 1);

	/* ready/busy pin */
	if (data->rdy_pin)
		at91_set_gpio_input(data->rdy_pin, 1);

	/* card detect pin */
	if (data->det_pin)
		at91_set_gpio_input(data->det_pin, 1);

	add_generic_device_res("atmel_nand", 0, nand_resources,
			       ARRAY_SIZE(nand_resources), data);
}
#else
void __init at91_add_device_nand(struct atmel_nand_data *data) {}
#endif

/* --------------------------------------------------------------------
 *  UART
 * -------------------------------------------------------------------- */

#if defined(CONFIG_DRIVER_SERIAL_ATMEL)
resource_size_t __init at91_configure_dbgu(void)
{
	at91_set_A_periph(AT91_PIN_PA9, 0);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PA10, 1);		/* DTXD */

	return AT91_BASE_SYS + AT91_DBGU;
}

resource_size_t __init at91_configure_usart0(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PA0, 1);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PA1, 0);		/* RXD0 */

	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PA2, 0);	/* RTS0 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PA3, 0);	/* CTS0 */

	return AT91SAM9X5_BASE_USART0;
}

resource_size_t __init at91_configure_usart1(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PA5, 1);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PA6, 0);		/* RXD1 */

	if (pins & ATMEL_UART_RTS)
		at91_set_C_periph(AT91_PIN_PC27, 0);	/* RTS1 */
	if (pins & ATMEL_UART_CTS)
		at91_set_C_periph(AT91_PIN_PC28, 0);	/* CTS1 */

	return AT91SAM9X5_BASE_USART1;
}

resource_size_t __init at91_configure_usart2(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PA7, 1);		/* TXD2 */
	at91_set_A_periph(AT91_PIN_PA8, 0);		/* RXD2 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PB0, 0);	/* RTS2 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PB1, 0);	/* CTS2 */

	return AT91SAM9X5_BASE_USART2;
}

resource_size_t __init at91_configure_usart3(unsigned pins)
{
	at91_set_B_periph(AT91_PIN_PC22, 1);		/* TXD3 */
	at91_set_B_periph(AT91_PIN_PC23, 0);		/* RXD3 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PC24, 0);	/* RTS3 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PC25, 0);	/* CTS3 */

	return AT91SAM9X5_BASE_USART3;
}
#endif
