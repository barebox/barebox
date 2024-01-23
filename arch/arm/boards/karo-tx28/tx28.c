// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2010 Juergen Beisert <kernel@pengutronix.de>, Pengutronix

#include <common.h>
#include <init.h>
#include <gpio.h>
#include <environment.h>
#include <errno.h>
#include <asm/armlinux.h>
#include <asm/barebox-arm.h>
#include <io.h>
#include <asm/mach-types.h>
#include <mach/mxs/imx-regs.h>
#include <mach/mxs/devices.h>
#include <mach/mxs/iomux.h>
#include <asm/mmu.h>

#include "tx28.h"

/* setup the CPU card internal signals */
static const uint32_t tx28_pad_setup[] = {
	/* NAND interface */
	GPMI_D0 | VE_3_3V | PULLUP(1),
	GPMI_D1 | VE_3_3V | PULLUP(1),
	GPMI_D2 | VE_3_3V | PULLUP(1),
	GPMI_D3 | VE_3_3V | PULLUP(1),
	GPMI_D4 | VE_3_3V | PULLUP(1),
	GPMI_D5 | VE_3_3V | PULLUP(1),
	GPMI_D6 | VE_3_3V | PULLUP(1),
	GPMI_D7 | VE_3_3V | PULLUP(1),
	GPMI_READY0 | VE_3_3V | PULLUP(0),	/* external PU */
	GPMI_CE0N | VE_3_3V | PULLUP(1),
	GPMI_RDN | VE_3_3V | PULLUP(1),
	GPMI_WRN | VE_3_3V | BITKEEPER(1),
	GPMI_ALE | VE_3_3V | PULLUP(1),
	GPMI_CLE | VE_3_3V | PULLUP(1),
	GPMI_RESETN | VE_3_3V | PULLUP(0),	/* external PU */

	/* Network interface */

	/*
	 * Note: To setup the external phy in a manner the baseboard
	 * supports, its configuration is divided into a small part here in
	 * the CPU card setup and the remaining configuration in the baseboard
	 * file.
	 * Here: Switch on the power supply to the external phy, but keep its
	 * reset line low.
	 */

	/* send a "good morning" to the ext. phy 0 = reset */
	ENET0_RX_CLK_GPIO | VE_3_3V | PULLUP(0) | GPIO_OUT | GPIO_VALUE(0),

	/* phy power control 1 = on */
	PWM4_GPIO | VE_3_3V | GPIO_OUT | PULLUP(0) | GPIO_VALUE(1),

	ENET_CLK | VE_3_3V | BITKEEPER(0),
	ENET0_MDC | VE_3_3V | PULLUP(0),
	ENET0_MDIO | VE_3_3V | PULLUP(0),
	ENET0_TXD0 | VE_3_3V | PULLUP(0),
	ENET0_TXD1 | VE_3_3V | PULLUP(0),
	ENET0_TX_EN | VE_3_3V | PULLUP(0),
	ENET0_TX_CLK | VE_3_3V | BITKEEPER(0),

};

static int tx28_devices_init(void)
{
	int i;

	if (barebox_arm_machine() != MACH_TYPE_TX28)
		return 0;

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(tx28_pad_setup); i++)
		imx_gpio_mode(tx28_pad_setup[i]);

	armlinux_set_architecture(MACH_TYPE_TX28);

	base_board_init();

	imx28_add_nand();

	return 0;
}

device_initcall(tx28_devices_init);
