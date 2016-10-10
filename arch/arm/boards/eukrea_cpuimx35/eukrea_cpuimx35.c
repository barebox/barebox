/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 *               2009 Marc Kleine-Budde, Pengutronix
 * (c) 2010 Eukrea Electromatique, Eric Bénard <eric@eukrea.com>
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
#include <command.h>
#include <environment.h>
#include <errno.h>
#include <fcntl.h>
#include <platform_data/eth-fec.h>
#include <fs.h>
#include <init.h>
#include <nand.h>
#include <net.h>
#include <partition.h>
#include <gpio.h>
#include <envfs.h>

#include <asm/armlinux.h>
#include <io.h>
#include <generated/mach-types.h>
#include <asm/mmu.h>

#include <mach/imx-nand.h>
#include <mach/imx35-regs.h>
#include <mach/iomux-mx35.h>
#include <mach/iomux-v3.h>
#include <mach/imx-ipu-fb.h>
#include <mach/imx-pll.h>
#include <i2c/i2c.h>
#include <usb/fsl_usb2.h>
#include <mach/usb.h>
#include <mach/devices-imx35.h>

static struct fec_platform_data fec_info = {
	.xcv_type	= PHY_INTERFACE_MODE_MII,
	.phy_addr	= 0,
};

struct imx_nand_platform_data nand_info = {
	.width		= 1,
	.hw_ecc		= 1,
	.flash_bbt	= 1,
};

static struct fb_videomode imxfb_mode = {
	.name		= "CMO_QVGA",
	.refresh	= 60,
	.xres		= 320,
	.yres		= 240,
	.pixclock	= KHZ2PICOS(7000),
	.left_margin	= 68,
	.right_margin	= 20,
	.upper_margin	= 15,
	.lower_margin	= 4,
	.hsync_len	= 30,
	.vsync_len	= 3,
	.sync		= 0,
	.vmode		= FB_VMODE_NONINTERLACED,
};

static void eukrea_cpuimx35_enable_display(int enable)
{
	gpio_direction_output(4, enable);
}

static struct imx_ipu_fb_platform_data ipu_fb_data = {
	.mode		= &imxfb_mode,
	.num_modes	= 1,
	.bpp		= 16,
	.enable		= eukrea_cpuimx35_enable_display,
};

#ifdef CONFIG_USB
#ifndef CONFIG_USB_GADGET
struct imxusb_platformdata otg_pdata = {
	.flags = MXC_EHCI_INTERFACE_DIFF_UNI,
	.mode = IMX_USB_MODE_HOST,
	.phymode = USBPHY_INTERFACE_MODE_UTMI,
};
#endif

struct imxusb_platformdata hs_pdata = {
	.flags = MXC_EHCI_INTERFACE_SINGLE_UNI | MXC_EHCI_INTERNAL_PHY | MXC_EHCI_IPPUE_DOWN,
	.mode = IMX_USB_MODE_HOST,
};
#endif

#ifdef CONFIG_USB_GADGET
static struct fsl_usb2_platform_data usb_pdata = {
	.operating_mode	= FSL_USB2_DR_DEVICE,
	.phy_mode	= FSL_USB2_PHY_UTMI,
};
#endif

static int eukrea_cpuimx35_mmu_init(void)
{
	l2x0_init((void __iomem *)0x30000000, 0x00030024, 0x00000000);

	return 0;
}
postmmu_initcall(eukrea_cpuimx35_mmu_init);

static iomux_v3_cfg_t eukrea_cpuimx35_pads[] = {
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

	MX35_PAD_LD23__GPIO3_29,
	MX35_PAD_CONTRAST__GPIO1_1,
	MX35_PAD_D3_CLS__GPIO1_4,

	MX35_PAD_I2C1_CLK__I2C1_SCL,
	MX35_PAD_I2C1_DAT__I2C1_SDA,

	MX35_PAD_SD1_CMD__ESDHC1_CMD,
	MX35_PAD_SD1_CLK__ESDHC1_CLK,
	MX35_PAD_SD1_DATA0__ESDHC1_DAT0,
	MX35_PAD_SD1_DATA1__ESDHC1_DAT1,
	MX35_PAD_SD1_DATA2__ESDHC1_DAT2,
	MX35_PAD_SD1_DATA3__ESDHC1_DAT3,

	MX35_PAD_LD19__GPIO3_25,
};

static int eukrea_cpuimx35_devices_init(void)
{
#ifdef CONFIG_USB_GADGET
	unsigned int tmp;
#endif
	mxc_iomux_v3_setup_multiple_pads(eukrea_cpuimx35_pads,
		ARRAY_SIZE(eukrea_cpuimx35_pads));

	imx35_add_nand(&nand_info);

	devfs_add_partition("nand0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	imx35_add_fec(&fec_info);
	imx35_add_fb(&ipu_fb_data);

	imx35_add_i2c0(NULL);
	imx35_add_mmc0(NULL);

	/* led default off */
	gpio_direction_output(32 * 2 + 29, 1);

	/* Switch : input */
	gpio_direction_input(32 * 2 + 25);

	/* screen default on to prevent flicker */
	gpio_direction_output(4, 0);
	/* backlight default off */
	gpio_direction_output(1, 0);

#ifdef CONFIG_USB
#ifndef CONFIG_USB_GADGET
	imx_add_usb((void *)MX35_USB_OTG_BASE_ADDR, 0, &otg_pdata);
#endif
	imx_add_usb((void *)MX35_USB_HS_BASE_ADDR, 1, &hs_pdata);
#endif

#ifdef CONFIG_USB_GADGET
	/* Workaround ENGcm09152 */
	tmp = readl(MX35_USB_OTG_BASE_ADDR + 0x608);
	writel(tmp | (1 << 23), MX35_USB_OTG_BASE_ADDR + 0x608);
	add_generic_device("fsl-udc", DEVICE_ID_DYNAMIC, NULL, MX35_USB_OTG_BASE_ADDR, 0x200,
			   IORESOURCE_MEM, &usb_pdata);
#endif
	armlinux_set_architecture(MACH_TYPE_EUKREA_CPUIMX35SD);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_eukrea_cpuimx35);

	return 0;
}

device_initcall(eukrea_cpuimx35_devices_init);

static int eukrea_cpuimx35_console_init(void)
{
	barebox_set_model("Eukrea CPUIMX35");
	barebox_set_hostname("eukrea-cpuimx35");

	imx35_add_uart0();
	return 0;
}

console_initcall(eukrea_cpuimx35_console_init);

static int eukrea_cpuimx35_core_init(void)
{
	u32 reg;

	/* enable clock for I2C1, ESDHC1, USB and FEC */
	reg = readl(MX35_CCM_BASE_ADDR + MX35_CCM_CGR0);
	reg |= 0x3 << MX35_CCM_CGR0_ESDHC1_SHIFT;
	reg = writel(reg, MX35_CCM_BASE_ADDR + MX35_CCM_CGR0);
	reg = readl(MX35_CCM_BASE_ADDR + MX35_CCM_CGR1);
	reg |= 0x3 << MX35_CCM_CGR1_FEC_SHIFT;
	reg |= 0x3 << MX35_CCM_CGR1_I2C1_SHIFT,
	reg = writel(reg, MX35_CCM_BASE_ADDR + MX35_CCM_CGR1);
	reg = readl(MX35_CCM_BASE_ADDR + MX35_CCM_CGR2);
	reg |= 0x3 << MX35_CCM_CGR2_USB_SHIFT;
	reg = writel(reg, MX35_CCM_BASE_ADDR + MX35_CCM_CGR2);

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

	/*
	 * M3IF Control Register (M3IFCTL)
	 * MRRP[0] = L2CC0 not on priority list (0 << 0)	= 0x00000000
	 * MRRP[1] = MAX1 not on priority list (0 << 0)		= 0x00000000
	 * MRRP[2] = L2CC1 not on priority list (0 << 0)	= 0x00000000
	 * MRRP[3] = USB  not on priority list (0 << 0)		= 0x00000000
	 * MRRP[4] = SDMA not on priority list (0 << 0)		= 0x00000000
	 * MRRP[5] = GPU not on priority list (0 << 0)		= 0x00000000
	 * MRRP[6] = IPU1 on priority list (1 << 6)		= 0x00000040
	 * MRRP[7] = IPU2 not on priority list (0 << 0)		= 0x00000000
	 *                                                       ------------
	 *                                                        0x00000040
	 */
	writel(0x40, MX35_M3IF_BASE_ADDR);

	return 0;
}

core_initcall(eukrea_cpuimx35_core_init);

static int do_cpufreq(int argc, char *argv[])
{
	unsigned long freq;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	freq = simple_strtoul(argv[1], NULL, 0);

	switch (freq) {
	case 399:
		writel(MPCTL_PARAM_399, MX35_CCM_BASE_ADDR + MX35_CCM_MPCTL);
		break;
	case 532:
		writel(MPCTL_PARAM_532, MX35_CCM_BASE_ADDR + MX35_CCM_MPCTL);
		break;
	default:
		return COMMAND_ERROR_USAGE;
	}

	printf("Switched CPU frequency to %luMHz\n", freq);

	return 0;
}

BAREBOX_CMD_START(cpufreq)
	.cmd            = do_cpufreq,
	BAREBOX_CMD_DESC("adjust CPU frequency")
	BAREBOX_CMD_OPTS("399|532")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
BAREBOX_CMD_END
