/*
 * Copyright (C) 2010 Juergen Beisert, Pengutronix <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <gpio.h>
#include <environment.h>
#include <errno.h>
#include <mci.h>
#include <fec.h>
#include <asm/io.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>
#include <mach/mci.h>

static struct stm_mci_platform_data mci_pdata = {
	.caps = MMC_MODE_4BIT | MMC_MODE_HS | MMC_MODE_HS_52MHz,
	.voltages = MMC_VDD_32_33 | MMC_VDD_33_34,	/* fixed to 3.3 V */
	.f_min = 400 * 1000,
};

static struct device_d mci_socket = {
	.name = "stm_mci",
	.map_base = IMX_SSP0_BASE,
	.platform_data = &mci_pdata,
};

/* PhyAD[0..2]=0, RMIISEL=1 */
static struct fec_platform_data fec_info = {
	.xcv_type = RMII,
	.phy_addr = 0,
};

static struct device_d fec_dev = {
	.name = "fec_imx",
	.map_base = IMX_FEC0_BASE,
	.platform_data = &fec_info,
};

static const uint32_t tx28_starterkit_pad_setup[] = {
	/*
	 * Part II of phy's initialization
	 * Setup phy's mode to '111'
	 */

	/*
	 * force the mod pins to a specific level
	 * '111' means: "All capable. Auto-negotiation enabled".
	 * For other values refer LAN8710's datasheet,
	 * chapter "Mode Bus - MODE[2:0]"
	 */
	ENET0_RXD0_GPIO | VE_3_3V | GPIO_OUT | GPIO_VALUE(1), /* MOD0 */
	ENET0_RXD1_GPIO | VE_3_3V | GPIO_OUT | GPIO_VALUE(1), /* MOD1 */
	ENET0_RX_EN_GPIO | VE_3_3V | GPIO_OUT | GPIO_VALUE(1), /* MOD2 */

	/* release the reset ('mod' pins get latched) */
	ENET0_RX_CLK_GPIO | VE_3_3V | BITKEEPER(0) | GPIO_OUT | GPIO_VALUE(1),

	/* right now the 'mod' pins are in their native mode */
	ENET0_RXD0 | VE_3_3V | PULLUP(0),
	ENET0_RXD1 | VE_3_3V | PULLUP(0),
	ENET0_RX_EN | VE_3_3V | PULLUP(0),

	/* Debug UART, available at card connector UART1 */
	AUART0_CTS_DUART_RX | VE_3_3V | STRENGTH(S8MA),
	AUART0_RTS_DUART_TX | VE_3_3V | STRENGTH(S8MA),
	AUART0_RX_DUART_CTS | VE_3_3V | STRENGTH(S8MA),
	AUART0_TX_DUART_RTS | VE_3_3V | STRENGTH(S8MA),
	/* Application UART, available at connector UART2 */
	AUART1_RX | VE_3_3V | BITKEEPER(0),
	AUART1_TX | VE_3_3V | BITKEEPER(0),
	AUART1_CTS | VE_3_3V | PULLUP(1),
	AUART1_RTS | VE_3_3V | PULLUP(1),
	/* Application UART, available at connector FIXME */
	AUART2_RX | VE_3_3V | PULLUP(1),
	AUART2_TX | VE_3_3V | PULLUP(1),
	AUART2_CTS | VE_3_3V | BITKEEPER(0),
	AUART2_RTS | VE_3_3V | BITKEEPER(0),

	/* MCI interface */
	SSP0_D0 | VE_3_3V | PULLUP(1),
	SSP0_D1 | VE_3_3V | PULLUP(1),
	SSP0_D2 | VE_3_3V | PULLUP(1),
	SSP0_D3 | VE_3_3V | PULLUP(1),
	SSP0_CMD | VE_3_3V | PULLUP(1),
	SSP0_CD | VE_3_3V | PULLUP(1),
	SSP0_SCK | VE_3_3V | BITKEEPER(0),

	/* MCI slot power control 1 = off */
	PWM3_GPIO | VE_3_3V | GPIO_OUT | GPIO_VALUE(0),
	/* MCI write protect 1 = not protected */
	SSP1_SCK_GPIO | VE_3_3V | GPIO_IN,	/* FIXME pull up ? */

	/* LED */
	ENET0_RXD3_GPIO | VE_3_3V | GPIO_OUT | GPIO_VALUE(1),

	/*
	 * The backlight is on, if:
	 * - the PWM0 pin outputs a low level
	 *   * AND *
	 * - the LCD_ENABLE is at high level.
	 * In all other combinations the backlight is off.
	 *
	 * Switch it off here to avoid flickering.
	 */
	PWM0_GPIO | VE_3_3V | PULLUP(0) | GPIO_OUT | GPIO_VALUE(1),

	/* LCD interface to the VGA connector */
	/* sync signals */
	LCD_WR_RWN_LCD_HSYNC | VE_3_3V | BITKEEPER(0),
	LCD_RD_E_LCD_VSYNC | VE_3_3V | BITKEEPER(0),
	LCD_CS_LCD_ENABLE | VE_3_3V | BITKEEPER(0),
	LCD_RS_LCD_DOTCLK | VE_3_3V | BITKEEPER(0),
	/* data signals */
	LCD_D0 | VE_3_3V | BITKEEPER(0),
	LCD_D1 | VE_3_3V | BITKEEPER(0),
	LCD_D2 | VE_3_3V | BITKEEPER(0),
	LCD_D3 | VE_3_3V | BITKEEPER(0),
	LCD_D4 | VE_3_3V | BITKEEPER(0),
	LCD_D5 | VE_3_3V | BITKEEPER(0),
	LCD_D6 | VE_3_3V | BITKEEPER(0),
	LCD_D7 | VE_3_3V | BITKEEPER(0),
	LCD_D8 | VE_3_3V | BITKEEPER(0),
	LCD_D9 | VE_3_3V | BITKEEPER(0),
	LCD_D10 | VE_3_3V | BITKEEPER(0),
	LCD_D11 | VE_3_3V | BITKEEPER(0),
	LCD_D12 | VE_3_3V | BITKEEPER(0),
	LCD_D13 | VE_3_3V | BITKEEPER(0),
	LCD_D14 | VE_3_3V | BITKEEPER(0),
	LCD_D15 | VE_3_3V | BITKEEPER(0),
	LCD_D16 | VE_3_3V | BITKEEPER(0),
	LCD_D17 | VE_3_3V | BITKEEPER(0),
	LCD_D18 | VE_3_3V | BITKEEPER(0),
	LCD_D19 | VE_3_3V | BITKEEPER(0),
	LCD_D20 | VE_3_3V | BITKEEPER(0),
	LCD_D21 | VE_3_3V | BITKEEPER(0),
	LCD_D22 | VE_3_3V | BITKEEPER(0),
	LCD_D23 | VE_3_3V | BITKEEPER(0),

	/* keep display's reset at low */
	LCD_RESET_GPIO | VE_3_3V | GPIO_OUT | GPIO_VALUE(0),
	/* keep display's power off */
	LCD_ENABLE_GPIO | VE_3_3V | GPIO_OUT | GPIO_VALUE(0),
};

/**
 * Try to register an environment storage on the attached MCI card
 * @return 0 on success
 *
 * We rely on the existance of a usable SD card, already attached to
 * our system, to get a persistent memory for our environment.
 * If this SD card is also the boot medium, we can use the second partition
 * for our environment purpose (if present!).
 */
static int register_persistent_environment(void)
{
	struct cdev *cdev;

	/*
	 * The TX28 STK5 has only one usable MCI card socket.
	 * So, we expect its name as "disk0".
	 */
	cdev = cdev_by_name("disk0");
	if (cdev == NULL) {
		pr_err("No MCI card preset\n");
		return -ENODEV;
	}

	/* MCI card is present, also a usable partition on it? */
	cdev = cdev_by_name("disk0.1");
	if (cdev == NULL) {
		pr_err("No second partition available\n");
		pr_info("Please create at least a second partition with"
			" 256 kiB...512 kiB in size (your choice)\n");
		return -ENODEV;
	}

	/* use the full partition as our persistent environment storage */
	return devfs_add_partition("disk0.1", 0, cdev->size,
					DEVFS_PARTITION_FIXED, "env0");
}

void base_board_init(void)
{
	int i, ret;

	/* initialize gpios */
	for (i = 0; i < ARRAY_SIZE(tx28_starterkit_pad_setup); i++)
		imx_gpio_mode(tx28_starterkit_pad_setup[i]);

	/* enable IOCLK0 to run at the PLL frequency */
	imx_set_ioclk(0, 480000000);
	/* run the SSP unit clock at 100 MHz */
	imx_set_sspclk(0, 100000000, 1);

	register_device(&mci_socket);

	imx_enable_enetclk();
	register_device(&fec_dev);

	ret = register_persistent_environment();
	if (ret != 0)
		printf("Cannot create the 'env0' persistent environment "
				"storage (%d)\n", ret);
}

static struct device_d tx28kit_serial_device = {
	.name     = "stm_serial",
	.map_base = IMX_DBGUART_BASE,
	.size     = 8192,
};

static int tx28kit_console_init(void)
{
	return register_device(&tx28kit_serial_device);
}

console_initcall(tx28kit_console_init);
