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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Board support for the Garz+Fricke Cupid board
 */

#include <common.h>
#include <command.h>
#include <init.h>
#include <driver.h>
#include <environment.h>
#include <fs.h>
#include <mach/imx-regs.h>
#include <asm/armlinux.h>
#include <mach/gpio.h>
#include <asm/io.h>
#include <partition.h>
#include <nand.h>
#include <generated/mach-types.h>
#include <mach/imx-nand.h>
#include <fec.h>
#include <fb.h>
#include <asm/mmu.h>
#include <mach/imx-ipu-fb.h>
#include <mach/imx-pll.h>
#include <mach/iomux-mx35.h>

static struct fec_platform_data fec_info = {
	.xcv_type = MII100,
};

static struct device_d fec_dev = {
	.id	  = -1,
	.name     = "fec_imx",
	.map_base = IMX_FEC_BASE,
	.platform_data	= &fec_info,
};

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram0_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = IMX_SDRAM_CS0,
	.size     = 128 * 1024 * 1024,
	.platform_data = &ram_pdata,
};

struct imx_nand_platform_data nand_info = {
	.width	= 1,
	.hw_ecc	= 1,
	.flash_bbt = 1,
};

static struct device_d nand_dev = {
	.id	  = -1,
	.name     = "imx_nand",
	.map_base = IMX_NFC_BASE,
	.platform_data	= &nand_info,
};

static struct fb_videomode guf_cupid_fb_mode = {
	/* 800x480 @ 70 Hz */
	.name		= "CPT CLAA070LC0JCT",
	.refresh	= 70,
	.xres		= 800,
	.yres		= 480,
	.pixclock	= 30761,
	.left_margin	= 24,
	.right_margin	= 47,
	.upper_margin	= 5,
	.lower_margin	= 3,
	.hsync_len	= 24,
	.vsync_len	= 3,
	.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_CLK_INVERT |
			  FB_SYNC_OE_ACT_HIGH,
	.vmode		= FB_VMODE_NONINTERLACED,
	.flag		= 0,
};

#define GPIO_LCD_ENABLE		(2 * 32 + 24)
#define GPIO_LCD_BACKLIGHT	(0 * 32 + 19)

static void cupid_fb_enable(int enable)
{
	if (enable) {
		gpio_direction_output(GPIO_LCD_ENABLE, 1);
		mdelay(100);
		gpio_direction_output(GPIO_LCD_BACKLIGHT, 1);
	} else {
		gpio_direction_output(GPIO_LCD_BACKLIGHT, 0);
		mdelay(100);
		gpio_direction_output(GPIO_LCD_ENABLE, 0);
	}
}

static struct imx_ipu_fb_platform_data ipu_fb_data = {
	.mode		= &guf_cupid_fb_mode,
	.bpp		= 16,
	.enable		= cupid_fb_enable,
};

static struct device_d imx_ipu_fb_dev = {
	.id		= -1,
	.name		= "imx-ipu-fb",
	.map_base	= 0x53fc0000,
	.size		= 0x1000,
	.platform_data	= &ipu_fb_data,
};

static struct device_d esdhc_dev = {
	.name		= "imx-esdhc",
	.map_base	= IMX_SDHC1_BASE,
};

#ifdef CONFIG_MMU
static int cupid_mmu_init(void)
{
	mmu_init();

	arm_create_section(0x80000000, 0x80000000, 128, PMD_SECT_DEF_CACHED);
	arm_create_section(0x90000000, 0x80000000, 128, PMD_SECT_DEF_UNCACHED);

	setup_dma_coherent(0x10000000);

	mmu_enable();

#ifdef CONFIG_CACHE_L2X0
	l2x0_init((void __iomem *)0x30000000, 0x00030024, 0x00000000);
#endif
	return 0;
}
postcore_initcall(cupid_mmu_init);
#endif

static int cupid_devices_init(void)
{
	uint32_t reg;

	gpio_direction_output(GPIO_LCD_ENABLE, 0);
	gpio_direction_output(GPIO_LCD_BACKLIGHT, 0);

	reg = readl(IMX_CCM_BASE + CCM_RCSR);
	/* some fuses provide us vital information about connected hardware */
	if (reg & 0x20000000)
		nand_info.width = 2;    /* 16 bit */
	else
		nand_info.width = 1;    /* 8 bit */

	register_device(&fec_dev);
	register_device(&nand_dev);

	devfs_add_partition("nand0", 0x00000, 0x40000, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", 0x40000, 0x80000, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	register_device(&sdram0_dev);
	register_device(&imx_ipu_fb_dev);
	register_device(&esdhc_dev);

	armlinux_add_dram(&sdram0_dev);
	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_GUF_CUPID);

	return 0;
}

device_initcall(cupid_devices_init);

static struct device_d cupid_serial_device = {
	.id	  = -1,
	.name     = "imx_serial",
	.map_base = IMX_UART1_BASE,
	.size     = 16 * 1024,
};

static struct pad_desc cupid_pads[] = {
	/* UART1 */
	MX35_PAD_CTS1__UART1_CTS,
	MX35_PAD_RTS1__UART1_RTS,
	MX35_PAD_TXD1__UART1_TXD_MUX,
	MX35_PAD_RXD1__UART1_RXD_MUX,
	/* UART2 */
	MX35_PAD_CTS2__UART2_CTS,
	MX35_PAD_RTS2__UART2_RTS,
	MX35_PAD_TXD2__UART2_TXD_MUX,
	MX35_PAD_RXD2__UART2_RXD_MUX,
	/* FEC */
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
	MX35_PAD_FEC_RDATA1__FEC_RDATA_1,
	MX35_PAD_FEC_TDATA1__FEC_TDATA_1,
	MX35_PAD_FEC_RDATA2__FEC_RDATA_2,
	MX35_PAD_FEC_TDATA2__FEC_TDATA_2,
	MX35_PAD_FEC_RDATA3__FEC_RDATA_3,
	MX35_PAD_FEC_TDATA3__FEC_TDATA_3,
	/* I2C1 */
	MX35_PAD_I2C1_CLK__I2C1_SCL,
	MX35_PAD_I2C1_DAT__I2C1_SDA,
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
	MX35_PAD_D3_VSYNC__IPU_DISPB_D3_VSYNC,
	MX35_PAD_LD18__GPIO3_24,		/* LCD enable */
	MX35_PAD_CSPI1_SS1__GPIO1_19,		/* LCD backligtht PWM */
	/* USB Host*/
	MX35_PAD_MLB_CLK__GPIO3_3,		/* USB Host PWR */
	MX35_PAD_MLB_DAT__GPIO3_4,		/* USB Host Overcurrent */
	/* USB OTG */
	MX35_PAD_USBOTG_PWR__USB_TOP_USBOTG_PWR,
	MX35_PAD_USBOTG_OC__USB_TOP_USBOTG_OC,
	/* SSI */
	MX35_PAD_STXFS4__AUDMUX_AUD4_TXFS,
	MX35_PAD_STXD4__AUDMUX_AUD4_TXD,
	MX35_PAD_SRXD4__AUDMUX_AUD4_RXD,
	MX35_PAD_SCK4__AUDMUX_AUD4_TXC,
	/* UCB1400 IRQ */
	MX35_PAD_ATA_INTRQ__GPIO2_29,
	/* Speaker On */
	MX35_PAD_LD20__GPIO3_26,
	/* LEDs */
	MX35_PAD_TX1__GPIO1_14,
	/* ESDHC1 */
	MX35_PAD_SD1_CMD__ESDHC1_CMD,
	MX35_PAD_SD1_CLK__ESDHC1_CLK,
	MX35_PAD_SD1_DATA0__ESDHC1_DAT0,
	MX35_PAD_SD1_DATA1__ESDHC1_DAT1,
	MX35_PAD_SD1_DATA2__ESDHC1_DAT2,
	MX35_PAD_SD1_DATA3__ESDHC1_DAT3,
	/* ESDHC1 CD */
	MX35_PAD_ATA_DATA5__GPIO2_18,
	/* ESDHC1 WP */
	MX35_PAD_ATA_DATA6__GPIO2_19,
};

static int cupid_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(cupid_pads, ARRAY_SIZE(cupid_pads));

	register_device(&cupid_serial_device);
	return 0;
}

console_initcall(cupid_console_init);

static int cupid_core_setup(void)
{
	u32 tmp;

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
	tmp = readl(IMX_AIPS1_BASE + 0x50);
	tmp &= 0x00FFFFFF;
	writel(tmp, IMX_AIPS1_BASE + 0x50);

	writel(0x0, IMX_AIPS2_BASE + 0x40);
	writel(0x0, IMX_AIPS2_BASE + 0x44);
	writel(0x0, IMX_AIPS2_BASE + 0x48);
	writel(0x0, IMX_AIPS2_BASE + 0x4C);
	tmp = readl(IMX_AIPS2_BASE + 0x50);
	tmp &= 0x00FFFFFF;
	writel(tmp, IMX_AIPS2_BASE + 0x50);

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

	writel(0x0000DCF6, CSCR_U(0)); /* CS0: NOR Flash */
	writel(0x444A4541, CSCR_L(0));
	writel(0x44443302, CSCR_A(0));

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
	writel(0x40, IMX_M3IF_BASE);

	return 0;
}

core_initcall(cupid_core_setup);

#define MPCTL_PARAM_399     (IMX_PLL_PD(0) | IMX_PLL_MFD(15) | IMX_PLL_MFI(8) | IMX_PLL_MFN(5))
#define MPCTL_PARAM_532     ((1 << 31) | IMX_PLL_PD(0) | IMX_PLL_MFD(11) | IMX_PLL_MFI(11) | IMX_PLL_MFN(1))

static int do_cpufreq(struct command *cmdtp, int argc, char *argv[])
{
	unsigned long freq;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	freq = simple_strtoul(argv[1], NULL, 0);

	switch (freq) {
	case 399:
		writel(MPCTL_PARAM_399, IMX_CCM_BASE + CCM_MPCTL);
		break;
	case 532:
		writel(MPCTL_PARAM_532, IMX_CCM_BASE + CCM_MPCTL);
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

