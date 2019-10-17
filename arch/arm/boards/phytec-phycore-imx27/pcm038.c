/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
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

#define pr_fmt(fmt) "pcm038: " fmt

#include <bootsource.h>
#include <common.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <notifier.h>
#include <linux/sizes.h>
#include <envfs.h>
#include <mach/devices-imx27.h>
#include <mach/imx-pll.h>
#include <mach/imx27-regs.h>
#include <mach/imxfb.h>
#include <mach/iomux-mx27.h>
#include <mfd/mc13xxx.h>
#include <mach/bbu.h>

#include "pll.h"

#define PCM038_GPIO_OTG_STP	(GPIO_PORTE + 1)

static struct fb_videomode imxfb_mode = {
	.name		= "Sharp-LQ035Q7",
	.refresh	= 60,
	.xres		= 240,
	.yres		= 320,
	.pixclock	= 188679, /* in ps (5.3MHz) */
	.hsync_len	= 7,
	.left_margin	= 5,
	.right_margin	= 16,
	.vsync_len	= 1,
	.upper_margin	= 7,
	.lower_margin	= 9,
};

static struct imx_fb_platform_data pcm038_fb_data = {
	.mode	= &imxfb_mode,
	.num_modes = 1,
	.pwmr	= 0x00a903ff,
	.lscr1	= 0x00120300,
	.dmacr	= 0x00020010,
	/*
	 * - HSYNC active high
	 * - VSYNC active high
	 * - clk notenabled while idle
	 * - clock not inverted
	 * - data not inverted
	 * - data enable low active
	 * - enable sharp mode
	 */
	.pcr	= 0xf00080c0,
	.bpp	= 16,
};

static const unsigned int pcm038_pins[] = {
	/* Display */
	PA5_PF_LSCLK,
	PA6_PF_LD0,
	PA7_PF_LD1,
	PA8_PF_LD2,
	PA9_PF_LD3,
	PA10_PF_LD4,
	PA11_PF_LD5,
	PA12_PF_LD6,
	PA13_PF_LD7,
	PA14_PF_LD8,
	PA15_PF_LD9,
	PA16_PF_LD10,
	PA17_PF_LD11,
	PA18_PF_LD12,
	PA19_PF_LD13,
	PA20_PF_LD14,
	PA21_PF_LD15,
	PA22_PF_LD16,
	PA23_PF_LD17,
	PA24_PF_REV,
	PA25_PF_CLS,
	PA26_PF_PS,
	PA27_PF_SPL_SPR,
	PA28_PF_HSYNC,
	PA29_PF_VSYNC,
	PA30_PF_CONTRAST,
	PA31_PF_OE_ACD,
	/* USB */
	PE1_PF_USBOTG_STP,
};

static int pcm038_init(void)
{
	struct mc13xxx *mc13xxx = mc13xxx_get();
	char *envdev;
	uint32_t i, bbu_nand_flags = 0, bbu_nor_flags = 0;

	if (!of_machine_is_compatible("phytec,imx27-pcm038"))
		return 0;

	/* Apply delay for STP line to stop ULPI */
	imx27_gpio_mode(PCM038_GPIO_OTG_STP | GPIO_GPIO);
	gpio_direction_output(PCM038_GPIO_OTG_STP, 1);
	mdelay(1);

	for (i = 0; i < ARRAY_SIZE(pcm038_pins); i++)
		imx27_gpio_mode(pcm038_pins[i]);

	imx27_add_fb(&pcm038_fb_data);

	switch (bootsource_get()) {
	case BOOTSOURCE_NAND:
		of_device_enable_path("/chosen/environment-nand");
		bbu_nand_flags |= BBU_HANDLER_FLAG_DEFAULT;
		envdev = "NAND";
		break;
	default:
		of_device_enable_path("/chosen/environment-nor");
		bbu_nor_flags |= BBU_HANDLER_FLAG_DEFAULT;
		envdev = "NOR";
		break;
	}

	pr_notice("Using environment in %s Flash\n", envdev);

	if (!mc13xxx) {
		pr_err("Failed to initialize PMIC. Will continue with low CPU speed\n");
		return 0;
	}

	mc13xxx_reg_write(mc13xxx, MC13783_REG_SWITCHERS(0),
				MC13783_SWX_VOLTAGE(MC13783_SWX_VOLTAGE_1_450) |
				MC13783_SWX_VOLTAGE_DVS(MC13783_SWX_VOLTAGE_1_450) |
				MC13783_SWX_VOLTAGE_STANDBY(MC13783_SWX_VOLTAGE_1_450));

	mc13xxx_reg_write(mc13xxx, MC13783_REG_SWITCHERS(4),
				MC13783_SW1A_MODE(MC13783_SWX_MODE_NO_PULSE_SKIP) |
				MC13783_SW1A_MODE_STANDBY(MC13783_SWX_MODE_NO_PULSE_SKIP) |
				MC13783_SW1A_SOFTSTART |
				MC13783_SW1B_MODE(MC13783_SWX_MODE_NO_PULSE_SKIP) |
				MC13783_SW1B_MODE_STANDBY(MC13783_SWX_MODE_NO_PULSE_SKIP) |
				MC13783_SW1B_SOFTSTART |
				MC13783_SW_PLL_FACTOR(32));

	if (IS_ENABLED(CONFIG_MCI_IMX)) {
		/* VMMC1 = 3.00 V */
		mc13xxx_set_bits(mc13xxx, MC13783_REG_REG_SETTING(1),
				 7 << 6, 6 << 6);
		/* Enable VMMC */
		mc13xxx_set_bits(mc13xxx, MC13783_REG_REG_MODE(1),
				 1 << 18, 1 << 18);
	}

	/* Wait for required power level to run the CPU at 400 MHz */
	mdelay(100);

	console_flush();
	writel(CSCR_VAL_FINAL, MX27_CCM_BASE_ADDR + MX27_CSCR);
	writel(0x130410c3, MX27_CCM_BASE_ADDR + MX27_PCDR0);
	writel(0x09030911, MX27_CCM_BASE_ADDR + MX27_PCDR1);

	/* Clocks have changed. Notify clients */
	clock_notifier_call_chain();

	/* Clock gating enable */
	writel(0x00050f08, MX27_SYSCTRL_BASE_ADDR + MX27_GPCR);

	imx_bbu_external_nand_register_handler("nand", "/dev/nand0.boot",
			bbu_nand_flags);
	imx_bbu_external_nor_register_handler("nor", "/dev/nor0.boot",
			bbu_nor_flags);

	defaultenv_append_directory(defaultenv_pcm038);

	return 0;
}
device_initcall(pcm038_init);
