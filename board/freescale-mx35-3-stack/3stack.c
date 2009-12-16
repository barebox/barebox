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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Derived from:
 *
 * * mx35_3stack.c - board file for uboot-v1
 *   Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *   (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
 *
 */

#include <common.h>
#include <cfi_flash.h>
#include <environment.h>
#include <errno.h>
#include <fcntl.h>
#include <fec.h>
#include <fs.h>
#include <init.h>
#include <nand.h>
#include <net.h>
#include <partition.h>

#include <asm/armlinux.h>
#include <asm/io.h>
#include <asm/mach-types.h>

#include <mach/gpio.h>
#include <mach/imx-nand.h>
#include <mach/imx-regs.h>
#include <mach/iomux-mx35.h>
#include <mach/iomux-v3.h>
#include <mach/pmic.h>
#include <mach/imx-ipu-fb.h>

#include <i2c/i2c.h>
#include <i2c/mc13892.h>
#include <i2c/mc9sdz60.h>

static struct device_d cfi_dev = {
	.name		= "cfi_flash",
	.map_base	= IMX_CS0_BASE,
	.size		= 64 * 1024 * 1024,
};

static struct fec_platform_data fec_info = {
	.xcv_type = MII100,
	.phy_addr = 0x1F,
};

static struct device_d fec_dev = {
	.name		= "fec_imx",
	.map_base	= IMX_FEC_BASE,
	.platform_data	= &fec_info,
};

static struct memory_platform_data sdram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.name		= "mem",
	.map_base	= IMX_SDRAM_CS0,
	.size		= 128 * 1024 * 1024,
	.platform_data	= &sdram_pdata,
};

struct imx_nand_platform_data nand_info = {
	.hw_ecc		= 1,
	.flash_bbt	= 1,
};

static struct device_d nand_dev = {
	.name		= "imx_nand",
	.map_base	= IMX_NFC_BASE,
	.platform_data	= &nand_info,
};

static struct device_d smc911x_dev = {
	.name		= "smc911x",
	.map_base	= IMX_CS5_BASE,
	.size		= IMX_CS5_RANGE,
};

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO("mc13892", 0x08),
	}, {
		I2C_BOARD_INFO("mc9sdz60", 0x69),
	},
};

static struct device_d i2c_dev = {
	.name		= "i2c-imx",
	.map_base	= IMX_I2C1_BASE,
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
	.flag		= 0,
};

static struct imx_ipu_fb_platform_data ipu_fb_data = {
	.mode		= &CTP_CLAA070LC0ACW,
	.bpp		= 16,
};

static struct device_d imxfb_dev = {
	.name		= "imx-ipu-fb",
	.map_base	= 0x53fc0000,
	.size		= 0x1000,
	.platform_data	= &ipu_fb_data,
};

static int f3s_devices_init(void)
{
	uint32_t reg;

	/* CS0: Nor Flash */
	writel(0x0000cf03, CSCR_U(0));
	writel(0x10000d03, CSCR_L(0));
	writel(0x00720900, CSCR_A(0));

	reg = readl(IMX_CCM_BASE + CCM_RCSR);
	/* some fuses provide us vital information about connected hardware */
	if (reg & 0x20000000)
		nand_info.width = 2;	/* bit */
	else
		nand_info.width = 1;	/* 8 bit */

	/*
	 * This platform supports NOR and NAND
	 */
	register_device(&nand_dev);
	register_device(&cfi_dev);

	switch ( (reg >> 25) & 0x3) {
	case 0x01:		/* NAND is the source */
		devfs_add_partition("nand0", 0x00000, 0x40000, PARTITION_FIXED, "self_raw");
		dev_add_bb_dev("self_raw", "self0");
		devfs_add_partition("nand0", 0x40000, 0x20000, PARTITION_FIXED, "env_raw");
		dev_add_bb_dev("env_raw", "env0");
		break;

	case 0x00:		/* NOR is the source */
		devfs_add_partition("nor0", 0x00000, 0x40000, PARTITION_FIXED, "self0");
		devfs_add_partition("nor0", 0x40000, 0x20000, PARTITION_FIXED, "env0");
		protect_file("/dev/env0", 1);
		break;
	}

	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	register_device(&i2c_dev);

	register_device(&fec_dev);
	register_device(&smc911x_dev);

	register_device(&sdram_dev);
	register_device(&imxfb_dev);

	armlinux_add_dram(&sdram_dev);
	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_MX35_3DS);

	return 0;
}

device_initcall(f3s_devices_init);

static int f3s_enable_display(void)
{
	gpio_direction_output(1, 1);
	return 0;
}

late_initcall(f3s_enable_display);

static struct device_d f3s_serial_device = {
	.name		= "imx_serial",
	.map_base	= IMX_UART1_BASE,
	.size		= 4096,
};

static struct pad_desc f3s_pads[] = {
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
	MX35_PAD_CONTRAST__GPIO1_1,
	MX35_PAD_D3_VSYNC__IPU_DISPB_D3_VSYNC,
	MX35_PAD_D3_REV__IPU_DISPB_D3_REV,
	MX35_PAD_D3_CLS__IPU_DISPB_D3_CLS,
};

static int f3s_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(f3s_pads, ARRAY_SIZE(f3s_pads));

	register_device(&f3s_serial_device);
	return 0;
}

console_initcall(f3s_console_init);

static int f3s_core_init(void)
{
	u32 reg;

	writel(0x0000D843, CSCR_U(5)); /* CS5: smc9117 */
	writel(0x22252521, CSCR_L(5));
	writel(0x22220A00, CSCR_A(5));

	/* enable clock for I2C1 and FEC */
	reg = readl(IMX_CCM_BASE + CCM_CGR1);
	reg |= 0x3 << CCM_CGR1_FEC_SHIFT;
	reg |= 0x3 << CCM_CGR1_I2C1_SHIFT;
	reg = writel(reg, IMX_CCM_BASE + CCM_CGR1);

	/* AIPS setup - Only setup MPROTx registers. The PACR default values are good.*/
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, IMX_AIPS1_BASE);
	writel(0x77777777, IMX_AIPS1_BASE + 0x4);
	writel(0x77777777, IMX_AIPS2_BASE);
	writel(0x77777777, IMX_AIPS2_BASE + 0x4);

	/*
	 * Clear the on and off peripheral modules Supervisor Protect bit
	 * for SDMA to access them. Did not change the AIPS control registers
	 * (offset 0x20) access type
	 */
	writel(0x0, IMX_AIPS1_BASE + 0x40);
	writel(0x0, IMX_AIPS1_BASE + 0x44);
	writel(0x0, IMX_AIPS1_BASE + 0x48);
	writel(0x0, IMX_AIPS1_BASE + 0x4C);
	reg = readl(IMX_AIPS1_BASE + 0x50);
	reg &= 0x00FFFFFF;
	writel(reg, IMX_AIPS1_BASE + 0x50);

	writel(0x0, IMX_AIPS2_BASE + 0x40);
	writel(0x0, IMX_AIPS2_BASE + 0x44);
	writel(0x0, IMX_AIPS2_BASE + 0x48);
	writel(0x0, IMX_AIPS2_BASE + 0x4C);
	reg = readl(IMX_AIPS2_BASE + 0x50);
	reg &= 0x00FFFFFF;
	writel(reg, IMX_AIPS2_BASE + 0x50);

	/* MAX (Multi-Layer AHB Crossbar Switch) setup */

	/* MPR - priority is M4 > M2 > M3 > M5 > M0 > M1 */
#define MAX_PARAM1 0x00302154
	writel(MAX_PARAM1, IMX_MAX_BASE + 0x0);   /* for S0 */
	writel(MAX_PARAM1, IMX_MAX_BASE + 0x100); /* for S1 */
	writel(MAX_PARAM1, IMX_MAX_BASE + 0x200); /* for S2 */
	writel(MAX_PARAM1, IMX_MAX_BASE + 0x300); /* for S3 */
	writel(MAX_PARAM1, IMX_MAX_BASE + 0x400); /* for S4 */

	/* SGPCR - always park on last master */
	writel(0x10, IMX_MAX_BASE + 0x10);	/* for S0 */
	writel(0x10, IMX_MAX_BASE + 0x110);	/* for S1 */
	writel(0x10, IMX_MAX_BASE + 0x210);	/* for S2 */
	writel(0x10, IMX_MAX_BASE + 0x310);	/* for S3 */
	writel(0x10, IMX_MAX_BASE + 0x410);	/* for S4 */

	/* MGPCR - restore default values */
	writel(0x0, IMX_MAX_BASE + 0x800);	/* for M0 */
	writel(0x0, IMX_MAX_BASE + 0x900);	/* for M1 */
	writel(0x0, IMX_MAX_BASE + 0xa00);	/* for M2 */
	writel(0x0, IMX_MAX_BASE + 0xb00);	/* for M3 */
	writel(0x0, IMX_MAX_BASE + 0xc00);	/* for M4 */
	writel(0x0, IMX_MAX_BASE + 0xd00);	/* for M5 */

	return 0;
}

core_initcall(f3s_core_init);

static int f3s_get_rev(struct i2c_client *client)
{
	u8 reg[3];
	int rev;

	i2c_read_reg(client, 0x7, reg, sizeof(reg));

	rev = reg[0] << 16 | reg [1] << 8 | reg[2];
	dev_info(&client->dev, "revision: 0x%x\n", rev);

	/* just return '0' or '1' */
	return !!((rev >> 6) & 0x7);
}

static void f3s_pmic_init_v2(struct i2c_client *client)
{
	u8 reg[3];

	i2c_read_reg(client, 0x1e, reg, sizeof(reg));
	reg[2] |= 0x03;
	i2c_write_reg(client, 0x1e, reg, sizeof(reg));

	i2c_read_reg(client, 0x20, reg, sizeof(reg));
	reg[2] |= 0x01;
	i2c_write_reg(client, 0x20, reg, sizeof(reg));
}

static void f3s_pmic_init_all(struct i2c_client *client)
{
	u8 reg[1];

	i2c_read_reg(client, 0x20, reg, sizeof(reg));
	reg[0] |= 0x04;
	i2c_write_reg(client, 0x20, reg, sizeof(reg));

	mdelay(200);

	i2c_read_reg(client, 0x1a, reg, sizeof(reg));
	reg[0] &= 0x7f;
	i2c_write_reg(client, 0x1a, reg, sizeof(reg));

	mdelay(200);

	reg[0] |= 0x80;
	i2c_write_reg(client, 0x1a, reg, sizeof(reg));
}

static int f3s_pmic_init(void)
{
	struct i2c_client *client;
	int rev;

	client = mc13892_get_client();
	if (!client)
		return -ENODEV;

	rev = f3s_get_rev(client);
	if (rev) {
		printf("i.MX35 CPU board version 2.0\n");
		f3s_pmic_init_v2(client);
	} else {
		printf("i.MX35 CPU board version 1.0\n");
	}

	client = mc9sdz60_get_client();
	if (!client)
		return -ENODEV;
	f3s_pmic_init_all(client);

	return 0;
}

late_initcall(f3s_pmic_init);

#ifdef CONFIG_NAND_IMX_BOOT
void __bare_init nand_boot(void)
{
	/*
	 * The driver is able to detect NAND's pagesize by CPU internal
	 * fuses or external pull ups. But not the blocksize...
	 */
	imx_nand_load_image((void *)TEXT_BASE, 256 * 1024);
}
#endif

