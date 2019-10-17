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
#include <platform_data/eth-fec.h>
#include <linux/sizes.h>
#include <io.h>
#include <net.h>
#include <asm/sections.h>
#include <asm/barebox-arm.h>
#include <linux/err.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>
#include <mach/mci.h>
#include <mach/fb.h>
#include <mach/ocotp.h>
#include <mach/iomux.h>
#include <generated/mach-types.h>

static struct mxs_mci_platform_data mci_pdata = {
	.caps = MMC_CAP_4_BIT_DATA,
	.voltages = MMC_VDD_32_33 | MMC_VDD_33_34,	/* fixed to 3.3 V */
	.f_min = 400 * 1000,
	.f_max = 25000000,
};

/* PhyAD[0..2]=0, RMIISEL=1 */
static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_RMII,
	.phy_addr = 0,
};

/*
 * The TX28 EVK comes with a VGA connector. We can support many video modes
 *
 * Note: The VGA connector is driven from the LCD lines via an ADV7125. This
 * DA converter needs an high active DE signal to show a video signal.
 */
static struct fb_videomode tx28evk_vmodes[] = {
	{
	/*
	 * Modeline "640x480" x 59.9 (to be used with the VGA connector)
	 * Clock: 25.18 MHz
	 * Line: 640 656 752 800 (31.5 kHz)
	 * Frame: 480 490 492 525
	 * Syncs: -hsync -vsync
	 */
		.name = "VGA",
		.refresh = 60,
		.xres = 640,
		.yres = 480,
		.pixclock = KHZ2PICOS(25180),
		.left_margin = 48,
		.hsync_len = 96,
		.right_margin = 16,
		.upper_margin = 33,
		.vsync_len = 2,
		.lower_margin = 10,
		.sync = FB_SYNC_DE_HIGH_ACT,
		.vmode = FB_VMODE_NONINTERLACED,
	}, {
	/*
	 * Emerging ETV570 640 x 480 display (directly connected)
	 * Clock: 25.175 MHz
	 * Syncs: low active, DE high active
	 * Display area: 115.2 mm x 86.4 mm
	 */
		.name = "ETV570",
		.refresh = 60,
		.xres = 640,
		.yres = 480,
		.pixclock = KHZ2PICOS(25175),
		.left_margin = 114,
		.hsync_len = 30,
		.right_margin = 16,
		.upper_margin = 32,
		.vsync_len = 3,
		.lower_margin = 10,
		.sync = FB_SYNC_DE_HIGH_ACT,
		.vmode = FB_VMODE_NONINTERLACED,
		/*
		 * This display is connected:
		 *  display side   -> CPU side
		 * ----------------------------
		 * RESET# pin  -> GPIO126 (3/30) LCD_RESET -> L = display reset
		 * PWRCTRL pin -> GPIO63 (1/31) LCD_ENABLE - > H=on, L=off
		 * LEDCTRL pin -> GPIO112 (2/16) PWM0 -> 2.5 V = LEDs off
		 *                                    ->   0 V = LEDs on
		 *
		 * Note: Backlight is on, only if PWRCTRL=H _and_ LEDCTRL=0
		 */
	}, {
	/*
	 * Modeline "800x600" x 60.3
	 * Clock: 40.00 MHz
	 * Line: 800 840 968 1056 (37.9 kHz)
	 * Frame: 600 601 605 628
	 * Syncs: +hsync +vsync
	 */
		.name = "SVGA",
		.refresh = 60,
		.xres = 800,
		.yres = 600,
		.pixclock = KHZ2PICOS(40000),
		.left_margin = 88,
		.hsync_len = 128,
		.right_margin = 40,
		.upper_margin = 23,
		.vsync_len = 4,
		.lower_margin = 1,
		.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT |
				FB_SYNC_DE_HIGH_ACT,
		.vmode = FB_VMODE_NONINTERLACED,
	}, {
	/*
	 * Modeline "1024x768" x 60.0
	 * Clock: 65.00 MHz
	 * Line: 1024 1048 1184 1344 (48.4 kHz)
	 * Frame: 768 771 777 806
	 * Syncs: -hsync -vsync
	 */
		.name = "XGA",
		.refresh = 60,
		.xres = 1024,
		.yres = 768,
		.pixclock = KHZ2PICOS(65000),
		.left_margin = 160,
		.hsync_len = 136,
		.right_margin = 24,
		.upper_margin = 29,
		.vsync_len = 6,
		.lower_margin = 3,
		.sync = FB_SYNC_DE_HIGH_ACT,
		.vmode = FB_VMODE_NONINTERLACED,
	}, {
	/*
	 * Modeline "1280x1024" x 60.0
	 * Clock: 108.00 MHz
	 * Line: 1280 1328 1440 1688 (64.0 kHz)
	 * Frame: 1024 1025 1028 1066
	 * Syncs: +hsync +vsync
	 */
		.name = "SXGA",
		.refresh = 60,
		.xres = 1280,
		.yres = 1024,
		.pixclock = KHZ2PICOS(108000),
		.left_margin = 248,
		.hsync_len = 112,
		.right_margin = 48,
		.upper_margin = 38,
		.vsync_len = 3,
		.lower_margin = 1,
		.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT |
				FB_SYNC_DE_HIGH_ACT,
		.vmode = FB_VMODE_NONINTERLACED,
	},
};

#define MAX_FB_SIZE SZ_2M

#define GPIO_LCD_RESET	126 /* 1 -> Reset  */
#define GPIO_BACKLIGHT	112 /* 0 -> backlight active */
#define GPIO_LCD_ENABLE	 63 /* 1 -> LCD enabled */

static void tx28_fb_enable(int enable)
{
	gpio_direction_output(GPIO_LCD_RESET, enable);
	gpio_direction_output(GPIO_LCD_ENABLE, enable);

	/* Give the display a chance to sync before we enable
	 * the backlight to avoid flickering
	 */
	if (enable)
		mdelay(300);

	gpio_direction_output(GPIO_BACKLIGHT, !enable);
}

static struct imx_fb_platformdata tx28_fb_pdata = {
	.mode_list = tx28evk_vmodes,
	.mode_cnt = ARRAY_SIZE(tx28evk_vmodes),
	.dotclk_delay = 0,	/* no adaption required */
	.ld_intf_width = 24,
	.fixed_screen = (void *)(0x40000000 + SZ_128M - MAX_FB_SIZE),
	.fixed_screen_size = MAX_FB_SIZE,
	.enable = tx28_fb_enable,
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
	LCD_CS | VE_3_3V | BITKEEPER(0),
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
	cdev = devfs_add_partition("disk0.1", 0, cdev->size,
					DEVFS_PARTITION_FIXED, "env0");
	if (IS_ERR(cdev))
		return PTR_ERR(cdev);
	return 0;
}

static void tx28_get_ethaddr(void)
{
	u32 buf[2];	/* to make use of cpu_to_be32 */
	u32 ethaddr[2];
	int ret;

	ret = mxs_ocotp_read(buf, 8, 0);
	if (ret != 8)
		return;

	ethaddr[0] = cpu_to_be32(buf[0]);
	ethaddr[1] = cpu_to_be32(buf[1]);

	eth_register_ethaddr(0, (char *)ethaddr);
}

void base_board_init(void)
{
	int i, ret;

	/* initialize gpios */
	for (i = 0; i < ARRAY_SIZE(tx28_starterkit_pad_setup); i++)
		imx_gpio_mode(tx28_starterkit_pad_setup[i]);

	add_generic_device("mxs_mci", 0, NULL, IMX_SSP0_BASE, 0x2000,
			   IORESOURCE_MEM, &mci_pdata);

	if (tx28_fb_pdata.fixed_screen < (void *)&_end) {
		printf("Warning: fixed_screen overlaps barebox\n");
		tx28_fb_pdata.fixed_screen = NULL;
	}

	add_generic_device("stmfb", 0, NULL, IMX_FB_BASE, 0x2000,
			   IORESOURCE_MEM, &tx28_fb_pdata);

	tx28_get_ethaddr();

	add_generic_device("imx28-fec", 0, NULL, IMX_FEC0_BASE, 0x4000,
			   IORESOURCE_MEM, &fec_info);

	ret = register_persistent_environment();
	if (ret != 0)
		printf("Cannot create the 'env0' persistent environment "
				"storage (%d)\n", ret);
}

static int tx28kit_console_init(void)
{
	if (barebox_arm_machine() != MACH_TYPE_TX28)
		return 0;

	barebox_set_model("Ka-Ro TX28");
	barebox_set_hostname("tx28");

	add_generic_device("stm_serial", 0, NULL, IMX_DBGUART_BASE, 0x2000,
			   IORESOURCE_MEM, NULL);

	return 0;
}

console_initcall(tx28kit_console_init);
