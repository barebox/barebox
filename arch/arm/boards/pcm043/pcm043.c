/*
 * (C) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (C) 2009 Pengutronix, Juergen Beisert <kernel@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 * Board support for Phytec's, i.MX35 based CPU card, called: PCM043
 */

#include <common.h>
#include <command.h>
#include <init.h>
#include <driver.h>
#include <environment.h>
#include <fs.h>
#include <gpio.h>
#include <sizes.h>
#include <mach/imx35-regs.h>
#include <asm/armlinux.h>
#include <io.h>
#include <partition.h>
#include <nand.h>
#include <generated/mach-types.h>
#include <mach/imx-nand.h>
#include <fec.h>
#include <fb.h>
#include <led.h>
#include <bootsource.h>
#include <asm/mmu.h>
#include <mach/weim.h>
#include <mach/imx-ipu-fb.h>
#include <mach/imx-pll.h>
#include <mach/iomux-mx35.h>
#include <mach/devices-imx35.h>
#include <mach/generic.h>
#include <mach/bbu.h>

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_MII,
};

struct imx_nand_platform_data nand_info = {
	.width	= 1,
	.hw_ecc	= 1,
	.flash_bbt = 1,
};

static struct fb_videomode pcm043_fb_mode[] = {
	{
		/* 240x320 @ 60 Hz */
		.name		= "TX090",
		.refresh	= 60,
		.xres		= 240,
		.yres		= 320,
		.pixclock	= 38255,
		.left_margin	= 144,
		.right_margin	= 0,
		.upper_margin	= 7,
		.lower_margin	= 40,
		.hsync_len	= 96,
		.vsync_len	= 1,
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_OE_ACT_HIGH,
		.vmode		= FB_VMODE_NONINTERLACED,
		.flag		= 0,
	}, {
		/* 240x320 @ 60 Hz */
		.name		= "Sharp-LQ035Q7",
		.refresh	= 60,
		.xres		= 240,
		.yres		= 320,
		.pixclock	= 185925,
		.left_margin	= 9,
		.right_margin	= 16,
		.upper_margin	= 7,
		.lower_margin	= 9,
		.hsync_len	= 1,
		.vsync_len	= 1,
		.sync		= FB_SYNC_HOR_HIGH_ACT | FB_SYNC_SHARP_MODE | \
				 FB_SYNC_CLK_INVERT | FB_SYNC_CLK_IDLE_EN,
		.vmode		= FB_VMODE_NONINTERLACED,
		.flag		= 0,
	}
};

static struct imx_ipu_fb_platform_data ipu_fb_data = {
	.mode			= pcm043_fb_mode,
	.num_modes		= ARRAY_SIZE(pcm043_fb_mode),
	.framebuffer_ovl	= (void *) (MX35_CSD0_BASE_ADDR + SZ_128M - SZ_1M),
	.bpp			= 16,
};

static int pcm043_mmu_init(void)
{
	l2x0_init((void __iomem *)0x30000000, 0x00030024, 0x00000000);

	return 0;
}
postmmu_initcall(pcm043_mmu_init);

struct gpio_led led0 = {
	.gpio = 1 * 32 + 6,
};

static int pcm043_devices_init(void)
{
	uint32_t reg;
	char *envstr;
	unsigned long bbu_nand_flags = 0;

	/* CS0: Nor Flash */
	imx35_setup_weimcs(5, 0x22C0CF00, 0x75000D01, 0x00000900);

	led_gpio_register(&led0);

	reg = readl(MX35_CCM_BASE_ADDR + MX35_CCM_RCSR);
	/* some fuses provide us vital information about connected hardware */
	if (reg & 0x20000000)
		nand_info.width = 2;    /* 16 bit */
	else
		nand_info.width = 1;    /* 8 bit */

	imx35_add_fec(&fec_info);
	/*
	 * This platform supports NOR and NAND
	 */
	imx35_add_nand(&nand_info);
	/*
	 * Up to 32MiB NOR type flash, connected to
	 * CS line 0, data width is 16 bit
	 */
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, MX35_CS0_BASE_ADDR, 32 * 1024 * 1024, 0);

	switch (bootsource_get()) {
	case BOOTSOURCE_NAND:
		devfs_add_partition("nand0", 0x00000, SZ_512K, DEVFS_PARTITION_FIXED, "self_raw");
		dev_add_bb_dev("self_raw", "self0");
		devfs_add_partition("nand0", SZ_512K, SZ_256K, DEVFS_PARTITION_FIXED, "env_raw");
		dev_add_bb_dev("env_raw", "env0");
		envstr = "NAND";
		bbu_nand_flags = BBU_HANDLER_FLAG_DEFAULT;
		break;
	case BOOTSOURCE_NOR:
	default:
		devfs_add_partition("nor0", 0x00000, SZ_512K, DEVFS_PARTITION_FIXED, "self0"); /* ourself */
		devfs_add_partition("nor0", SZ_512K, SZ_128K, DEVFS_PARTITION_FIXED, "env0");  /* environment */
		protect_file("/dev/env0", 1);
		envstr = "NOR";
		break;
	}

	pr_info("using environment from %s flash\n", envstr);

	imx35_add_fb(&ipu_fb_data);

	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_PCM043);

	imx_bbu_external_nand_register_handler("nand", "/dev/nand0.barebox",
			bbu_nand_flags);

	return 0;
}

device_initcall(pcm043_devices_init);

static iomux_v3_cfg_t pcm043_pads[] = {
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
	MX35_PAD_ATA_CS0__GPIO2_6, /* LED */
};

static int imx35_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(pcm043_pads, ARRAY_SIZE(pcm043_pads));

	barebox_set_model("Phytec phyCORE-i.MX35");
	barebox_set_hostname("phycore-imx35");

	imx35_add_uart0();

	return 0;
}

console_initcall(imx35_console_init);

static int pcm043_core_setup(void)
{
	u32 tmp;

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
	tmp = readl(MX35_AIPS1_BASE_ADDR + 0x50);
	tmp &= 0x00FFFFFF;
	writel(tmp, MX35_AIPS1_BASE_ADDR + 0x50);

	writel(0x0, MX35_AIPS2_BASE_ADDR + 0x40);
	writel(0x0, MX35_AIPS2_BASE_ADDR + 0x44);
	writel(0x0, MX35_AIPS2_BASE_ADDR + 0x48);
	writel(0x0, MX35_AIPS2_BASE_ADDR + 0x4C);
	tmp = readl(MX35_AIPS2_BASE_ADDR + 0x50);
	tmp &= 0x00FFFFFF;
	writel(tmp, MX35_AIPS2_BASE_ADDR + 0x50);

	/* MAX (Multi-Layer AHB Crossbar Switch) setup */

	/* MPR - priority is M4 > M2 > M3 > M5 > M0 > M1 */
#define MAX_PARAM1 0x00302154
	writel(MAX_PARAM1, MX35_MAX_BASE_ADDR + 0x0);   /* for S0 */
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

core_initcall(pcm043_core_setup);

#define MPCTL_PARAM_399     (IMX_PLL_PD(0) | IMX_PLL_MFD(15) | IMX_PLL_MFI(8) | IMX_PLL_MFN(5))
#define MPCTL_PARAM_532     ((1 << 31) | IMX_PLL_PD(0) | IMX_PLL_MFD(11) | IMX_PLL_MFI(11) | IMX_PLL_MFN(1))

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

	printf("Switched CPU frequency to %ldMHz\n", freq);

	return 0;
}

static const __maybe_unused char cmd_cpufreq_help[] =
"Usage: cpufreq 399|532\n"
"\n"
"Set CPU frequency to <freq> MHz\n";

BAREBOX_CMD_START(cpufreq)
	.cmd            = do_cpufreq,
	.usage          = "adjust CPU frequency",
	BAREBOX_CMD_HELP(cmd_cpufreq_help)
BAREBOX_CMD_END

