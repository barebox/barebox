/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 *               2009 Marc Kleine-Budde, Pengutronix
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
 *
 *
 * Derived from:
 *
 * * mx35_3stack.c - board file for uboot-v1
 *   Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *   (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
 *
 */

#include <common.h>
#include <environment.h>
#include <errno.h>
#include <fcntl.h>
#include <platform_data/eth-fec.h>
#include <fs.h>
#include <init.h>
#include <nand.h>
#include <net.h>
#include <envfs.h>
#include <linux/sizes.h>
#include <partition.h>
#include <gpio.h>

#include <asm/armlinux.h>
#include <asm/sections.h>
#include <asm/barebox-arm.h>
#include <io.h>
#include <generated/mach-types.h>

#include <mach/weim.h>
#include <mach/imx-nand.h>
#include <mach/imx35-regs.h>
#include <mach/iomux-mx35.h>
#include <mach/iomux-v3.h>
#include <mach/imx-ipu-fb.h>
#include <mach/generic.h>
#include <mach/devices-imx35.h>
#include <mach/revision.h>

#include <i2c/i2c.h>
#include <mfd/mc13xxx.h>
#include <mfd/mc9sdz60.h>


/* Board rev for the PDK 3stack */
#define MX35PDK_BOARD_REV_1		0
#define MX35PDK_BOARD_REV_2		1

static struct fec_platform_data fec_info = {
	.xcv_type	= PHY_INTERFACE_MODE_MII,
	.phy_addr	= 0x1F,
};

struct imx_nand_platform_data nand_info = {
	.hw_ecc		= 1,
	.flash_bbt	= 1,
};

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO("mc13892", 0x08),
	}, {
		I2C_BOARD_INFO("mc9sdz60", 0x69),
	},
};

/*
 * Generic display, shipped with the PDK
 */
static struct fb_videomode CTP_CLAA070LC0ACW = {
	/* 800x480 @ 60 Hz */
	.name		= "CTP-CLAA070LC0ACW",
	.refresh	= 60,
	.xres		= 800,
	.yres		= 480,
	.pixclock	= KHZ2PICOS(27000),
	.left_margin	= 50,
	.right_margin	= 50,	/* whole line should have 900 clocks */
	.upper_margin	= 10,
	.lower_margin	= 10,	/* whole frame should have 500 lines */
	.hsync_len	= 1,	/* note: DE only display */
	.vsync_len	= 1,	/* note: DE only display */
	.sync		= FB_SYNC_CLK_IDLE_EN | FB_SYNC_OE_ACT_HIGH,
	.vmode		= FB_VMODE_NONINTERLACED,
};

static struct imx_ipu_fb_platform_data ipu_fb_data = {
	.mode		= &CTP_CLAA070LC0ACW,
	.num_modes	= 1,
	.bpp		= 16,
};

/*
 * Revision to be passed to kernel. The kernel provided
 * by freescale relies on this.
 *
 * C --> CPU type
 * S --> Silicon revision
 * B --> Board rev
 *
 * 31    20     16     12    8      4     0
 *        | Cmaj | Cmin | B  | Smaj | Smin|
 *
 * e.g 0x00035120 --> i.MX35, Cpu silicon rev 2.0, Board rev 2
*/
static unsigned int imx35_3ds_system_rev = 0x00035000;

static void set_silicon_rev( int rev)
{
	imx35_3ds_system_rev = imx35_3ds_system_rev | (rev & 0xFF);
}

static void set_board_rev(int rev)
{
	imx35_3ds_system_rev =  (imx35_3ds_system_rev & ~(0xF << 8)) | (rev & 0xF) << 8;
}

static const struct devfs_partition f3s_nand0_partitions[] = {
	{
		.offset = 0,
		.size = 0x40000,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "self_raw",
		.bbname = "self0",
	}, {
		.offset = DEVFS_PARTITION_APPEND, /* 0x40000 */
		.size = 0x80000,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "env_raw",
		.bbname = "env0",
	}, {
		/* sentinel */
	}
};

static const struct devfs_partition f3s_nor0_partitions[] = {
	{
		.offset = 0,
		.size = 0x40000,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "self0",
	}, {
		.offset = DEVFS_PARTITION_APPEND, /* 0x40000 */
		.size = 0x80000,
		.flags = DEVFS_PARTITION_FIXED,
		.name = "env0",
	}, {
		/* sentinel */
	}
};

static int f3s_devices_init(void)
{
	uint32_t reg;

	/* CS0: Nor Flash */
	imx35_setup_weimcs(0, 0x0000cf03, 0x10000d03, 0x00720900);

	reg = readl(MX35_CCM_BASE_ADDR + MX35_CCM_RCSR);
	/* some fuses provide us vital information about connected hardware */
	if (reg & 0x20000000)
		nand_info.width = 2;	/* 16 bit */
	else
		nand_info.width = 1;	/* 8 bit */

	/*
	 * This platform supports NOR and NAND
	 */
	imx35_add_nand(&nand_info);
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, MX35_CS0_BASE_ADDR, 64 * 1024 * 1024, 0);

	switch ((reg >> 25) & 0x3) {
	case 0x01:		/* NAND is the source */
		devfs_create_partitions("nand0", f3s_nand0_partitions);
		break;

	case 0x00:		/* NOR is the source */
		devfs_create_partitions("nor0", f3s_nor0_partitions);
		protect_file("/dev/env0", 1);
		break;
	}

	set_silicon_rev(imx_silicon_revision());

	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	imx35_add_i2c0(NULL);

	imx35_add_fec(&fec_info);
	add_generic_device("smc911x", DEVICE_ID_DYNAMIC, NULL,	MX35_CS5_BASE_ADDR, MX35_CS5_SIZE,
			IORESOURCE_MEM, NULL);

	imx35_add_mmc0(NULL);

	imx35_add_fb(&ipu_fb_data);

	armlinux_set_architecture(MACH_TYPE_MX35_3DS);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_freescale_mx35_3ds);

	return 0;
}

device_initcall(f3s_devices_init);

static int f3s_enable_display(void)
{
	/* Enable power to the LCD. (bit 6 hi.) */
	mc9sdz60_set_bits(mc9sdz60_get(), MC9SDZ60_REG_GPIO_1, 0x40, 0x40);

	return 0;
}

late_initcall(f3s_enable_display);

static iomux_v3_cfg_t f3s_pads[] = {
	MX35_PAD_FEC_TX_CLK__FEC_TX_CLK,
	MX35_PAD_FEC_RX_CLK__FEC_RX_CLK,
	MX35_PAD_FEC_RX_DV__FEC_RX_DV,
	MX35_PAD_FEC_COL__FEC_COL,
	MX35_PAD_FEC_RDATA0__FEC_RDATA_0,
	MX35_PAD_FEC_TDATA0__FEC_TDATA_0,
	MX35_PAD_FEC_TX_EN__FEC_TX_EN,
	MX35_PAD_FEC_MDC__FEC_MDC,
	MX35_PAD_FEC_MDIO__FEC_MDIO,
	MX35_PAD_FEC_TX_ERR__FEC_TX_ERR,
	MX35_PAD_FEC_RX_ERR__FEC_RX_ERR,
	MX35_PAD_FEC_CRS__FEC_CRS,
	MX35_PAD_FEC_RDATA0__FEC_RDATA_0,
	MX35_PAD_FEC_TDATA0__FEC_TDATA_0,
	MX35_PAD_FEC_RDATA1__FEC_RDATA_1,
	MX35_PAD_FEC_TDATA1__FEC_TDATA_1,
	MX35_PAD_FEC_RDATA2__FEC_RDATA_2,
	MX35_PAD_FEC_TDATA2__FEC_TDATA_2,
	MX35_PAD_FEC_RDATA3__FEC_RDATA_3,
	MX35_PAD_FEC_TDATA3__FEC_TDATA_3,

	MX35_PAD_RXD1__UART1_RXD_MUX,
	MX35_PAD_TXD1__UART1_TXD_MUX,
	MX35_PAD_RTS1__UART1_RTS,
	MX35_PAD_CTS1__UART1_CTS,

	MX35_PAD_I2C1_CLK__I2C1_SCL,
	MX35_PAD_I2C1_DAT__I2C1_SDA,

	MX35_PAD_WDOG_RST__GPIO1_6,
	MX35_PAD_COMPARE__GPIO1_5,

	/* Display */
	MX35_PAD_LD0__IPU_DISPB_DAT_0,
	MX35_PAD_LD1__IPU_DISPB_DAT_1,
	MX35_PAD_LD2__IPU_DISPB_DAT_2,
	MX35_PAD_LD3__IPU_DISPB_DAT_3,
	MX35_PAD_LD4__IPU_DISPB_DAT_4,
	MX35_PAD_LD5__IPU_DISPB_DAT_5,
	MX35_PAD_LD6__IPU_DISPB_DAT_6,
	MX35_PAD_LD7__IPU_DISPB_DAT_7,
	MX35_PAD_LD8__IPU_DISPB_DAT_8,
	MX35_PAD_LD9__IPU_DISPB_DAT_9,
	MX35_PAD_LD10__IPU_DISPB_DAT_10,
	MX35_PAD_LD11__IPU_DISPB_DAT_11,
	MX35_PAD_LD12__IPU_DISPB_DAT_12,
	MX35_PAD_LD13__IPU_DISPB_DAT_13,
	MX35_PAD_LD14__IPU_DISPB_DAT_14,
	MX35_PAD_LD15__IPU_DISPB_DAT_15,
	MX35_PAD_LD16__IPU_DISPB_DAT_16,
	MX35_PAD_LD17__IPU_DISPB_DAT_17,
	MX35_PAD_D3_HSYNC__IPU_DISPB_D3_HSYNC,
	MX35_PAD_D3_FPSHIFT__IPU_DISPB_D3_CLK,
	MX35_PAD_D3_DRDY__IPU_DISPB_D3_DRDY,
	MX35_PAD_CONTRAST__IPU_DISPB_CONTR,
	MX35_PAD_D3_VSYNC__IPU_DISPB_D3_VSYNC,
	MX35_PAD_D3_REV__IPU_DISPB_D3_REV,
	MX35_PAD_D3_CLS__IPU_DISPB_D3_CLS,
};

static int f3s_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(f3s_pads, ARRAY_SIZE(f3s_pads));

	barebox_set_model("Freescale i.MX35 3DS");
	barebox_set_hostname("mx35-3stack");

	imx35_add_uart0();
	return 0;
}

console_initcall(f3s_console_init);

static int f3s_core_init(void)
{
	u32 reg;

	/* CS5: smc9117 */
	imx35_setup_weimcs(5, 0x0000D843, 0x22252521, 0x22220A00);

	/* enable clock for I2C1 and FEC */
	reg = readl(MX35_CCM_BASE_ADDR + MX35_CCM_CGR1);
	reg |= 0x3 << MX35_CCM_CGR1_FEC_SHIFT;
	reg |= 0x3 << MX35_CCM_CGR1_I2C1_SHIFT;
	reg = writel(reg, MX35_CCM_BASE_ADDR + MX35_CCM_CGR1);

	/* AIPS setup - Only setup MPROTx registers. The PACR default values are good.*/
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, MX35_AIPS1_BASE_ADDR);
	writel(0x77777777, MX35_AIPS1_BASE_ADDR + 0x4);
	writel(0x77777777, MX35_AIPS2_BASE_ADDR);
	writel(0x77777777, MX35_AIPS2_BASE_ADDR + 0x4);

	/*
	 * Clear the on and off peripheral modules Supervisor Protect bit
	 * for SDMA to access them. Did not change the AIPS control registers
	 * (offset 0x20) access type
	 */
	writel(0x0, MX35_AIPS1_BASE_ADDR + 0x40);
	writel(0x0, MX35_AIPS1_BASE_ADDR + 0x44);
	writel(0x0, MX35_AIPS1_BASE_ADDR + 0x48);
	writel(0x0, MX35_AIPS1_BASE_ADDR + 0x4C);
	reg = readl(MX35_AIPS1_BASE_ADDR + 0x50);
	reg &= 0x00FFFFFF;
	writel(reg, MX35_AIPS1_BASE_ADDR + 0x50);

	writel(0x0, MX35_AIPS2_BASE_ADDR + 0x40);
	writel(0x0, MX35_AIPS2_BASE_ADDR + 0x44);
	writel(0x0, MX35_AIPS2_BASE_ADDR + 0x48);
	writel(0x0, MX35_AIPS2_BASE_ADDR + 0x4C);
	reg = readl(MX35_AIPS2_BASE_ADDR + 0x50);
	reg &= 0x00FFFFFF;
	writel(reg, MX35_AIPS2_BASE_ADDR + 0x50);

	/* MAX (Multi-Layer AHB Crossbar Switch) setup */

	/* MPR - priority is M4 > M2 > M3 > M5 > M0 > M1 */
#define MAX_PARAM1 0x00302154
	writel(MAX_PARAM1, MX35_MAX_BASE_ADDR + 0x000); /* for S0 */
	writel(MAX_PARAM1, MX35_MAX_BASE_ADDR + 0x100); /* for S1 */
	writel(MAX_PARAM1, MX35_MAX_BASE_ADDR + 0x200); /* for S2 */
	writel(MAX_PARAM1, MX35_MAX_BASE_ADDR + 0x300); /* for S3 */
	writel(MAX_PARAM1, MX35_MAX_BASE_ADDR + 0x400); /* for S4 */

	/* SGPCR - always park on last master */
	writel(0x10, MX35_MAX_BASE_ADDR + 0x10);	/* for S0 */
	writel(0x10, MX35_MAX_BASE_ADDR + 0x110);	/* for S1 */
	writel(0x10, MX35_MAX_BASE_ADDR + 0x210);	/* for S2 */
	writel(0x10, MX35_MAX_BASE_ADDR + 0x310);	/* for S3 */
	writel(0x10, MX35_MAX_BASE_ADDR + 0x410);	/* for S4 */

	/* MGPCR - restore default values */
	writel(0x0, MX35_MAX_BASE_ADDR + 0x800);	/* for M0 */
	writel(0x0, MX35_MAX_BASE_ADDR + 0x900);	/* for M1 */
	writel(0x0, MX35_MAX_BASE_ADDR + 0xa00);	/* for M2 */
	writel(0x0, MX35_MAX_BASE_ADDR + 0xb00);	/* for M3 */
	writel(0x0, MX35_MAX_BASE_ADDR + 0xc00);	/* for M4 */
	writel(0x0, MX35_MAX_BASE_ADDR + 0xd00);	/* for M5 */

	return 0;
}

core_initcall(f3s_core_init);

static int f3s_get_rev(struct mc13xxx *mc13xxx)
{
	u32 rev;
	int err;

	err = mc13xxx_reg_read(mc13xxx, MC13XXX_REG_IDENTIFICATION, &rev);
	if (err)
		return err;

	if (rev == 0x00ffffff)
		return -ENODEV;

	return ((rev >> 6) & 0x7) ? MX35PDK_BOARD_REV_2 : MX35PDK_BOARD_REV_1;
}

static int f3s_pmic_init_v2(struct mc13xxx *mc13xxx)
{
	int err = 0;

	/* COMPARE pin (GPIO1_5) as output and set high */
	gpio_direction_output( 32*0 + 5 , 1);

	err |= mc13xxx_set_bits(mc13xxx, MC13892_REG_SETTING_0, 0x03, 0x03);
	err |= mc13xxx_set_bits(mc13xxx, MC13892_REG_MODE_0, 0x01, 0x01);
	if (err)
		printf("mc13892 Init sequence failed, the system might not be working!\n");

	return err;
}

static int f3s_pmic_init_all(struct mc9sdz60 *mc9sdz60)
{
	int err = 0;

	err |= mc9sdz60_set_bits(mc9sdz60, MC9SDZ60_REG_GPIO_1, 0x04, 0x04);

	err |= mc9sdz60_set_bits(mc9sdz60, MC9SDZ60_REG_RESET_1, 0x80, 0x00);
	mdelay(200);
	err |= mc9sdz60_set_bits(mc9sdz60, MC9SDZ60_REG_RESET_1, 0x80, 0x80);

	if (err)
		dev_err(&mc9sdz60->client->dev,
			"Init sequence failed, the system might not be working!\n");

	return err;
}

static int f3s_pmic_init(void)
{
	struct mc13xxx *mc13xxx;
	struct mc9sdz60 *mc9sdz60;
	int rev;

	mc13xxx = mc13xxx_get();
	if (!mc13xxx) {
		printf("FAILED to get PMIC handle!\n");
		return 0;
	}

	rev = f3s_get_rev(mc13xxx);
	switch (rev) {
	case MX35PDK_BOARD_REV_1:
		break;
	case MX35PDK_BOARD_REV_2:
		f3s_pmic_init_v2(mc13xxx);
		break;
	default:
		printf("FAILED to identify board revision!\n");
		return 0;
	}

	set_board_rev(rev);
	printf("i.MX35 PDK CPU board version %d.\n", rev );

	mc9sdz60 = mc9sdz60_get();
	if (!mc9sdz60) {
		printf("FAILED to get mc9sdz60 handle!\n");
		return 0;
	}

	f3s_pmic_init_all(mc9sdz60);

	armlinux_set_revision(imx35_3ds_system_rev);

	return 0;
}

late_initcall(f3s_pmic_init);
