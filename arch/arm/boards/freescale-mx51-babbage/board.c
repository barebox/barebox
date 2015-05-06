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
 *
 *
 */

#define pr_fmt(fmt) "babbage: " fmt

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/imx51-regs.h>
#include <fec.h>
#include <gpio.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <of.h>
#include <fcntl.h>
#include <mach/bbu.h>
#include <nand.h>
#include <notifier.h>
#include <spi/spi.h>
#include <mfd/mc13xxx.h>
#include <io.h>
#include <asm/mmu.h>
#include <mach/imx5.h>
#include <mach/imx-nand.h>
#include <mach/spi.h>
#include <mach/generic.h>
#include <mach/iomux-mx51.h>
#include <mach/devices-imx51.h>
#include <mach/revision.h>
#include <mach/imx-flash-header.h>

#define MX51_CCM_CACRR 0x10

static void babbage_power_init(struct mc13xxx *mc13xxx)
{
	u32 val;

	/* Write needed to Power Gate 2 register */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_POWER_MISC, &val);
	val &= ~0x10000;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_POWER_MISC, val);

	/* Write needed to update Charger 0 */
	mc13xxx_reg_write(mc13xxx, MC13892_REG_CHARGE, 0x0023807F);

	/* power up the system first */
	mc13xxx_reg_write(mc13xxx, MC13892_REG_POWER_MISC, 0x00200000);

	if (imx_silicon_revision() < IMX_CHIP_REV_3_0) {
		/* Set core voltage to 1.1V */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_0, &val);
		val &= ~0x1f;
		val |= 0x14;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_0, val);

		/* Setup VCC (SW2) to 1.25 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_1, &val);
		val &= ~0x1f;
		val |= 0x1a;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_1, val);

		/* Setup 1V2_DIG1 (SW3) to 1.25 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_2, &val);
		val &= ~0x1f;
		val |= 0x1a;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_2, val);
	} else {
		/* Setup VCC (SW2) to 1.225 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_1, &val);
		val &= ~0x1f;
		val |= 0x19;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_1, val);

		/* Setup 1V2_DIG1 (SW3) to 1.2 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_2, &val);
		val &= ~0x1f;
		val |= 0x18;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_2, val);
	}

	if (mc13xxx_revision(mc13xxx) < MC13892_REVISION_2_0) {
		/* Set switchers in PWM mode for Atlas 2.0 and lower */
		/* Setup the switcher mode for SW1 & SW2*/
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_4, &val);
		val &= ~0x3c0f;
		val |= 0x1405;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_4, val);

		/* Setup the switcher mode for SW3 & SW4 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_5, &val);
		val &= ~0xf0f;
		val |= 0x505;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_5, val);
	} else {
		/* Set switchers in Auto in NORMAL mode & STANDBY mode for Atlas 2.0a */
		/* Setup the switcher mode for SW1 & SW2*/
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_4, &val);
		val &= ~0x3c0f;
		val |= 0x2008;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_4, val);

		/* Setup the switcher mode for SW3 & SW4 */
		mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_5, &val);
		val &= ~0xf0f;
		val |= 0x808;
		mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_5, val);
	}

	/* Set VDIG to 1.65V, VGEN3 to 1.8V, VCAM to 2.5V */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SETTING_0, &val);
	val &= ~0x34030;
	val |= 0x10020;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SETTING_0, val);

	/* Set VVIDEO to 2.775V, VAUDIO to 3V, VSD to 3.15V */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SETTING_1, &val);
	val &= ~0x1FC;
	val |= 0x1F4;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SETTING_1, val);

	/* Configure VGEN3 and VCAM regulators to use external PNP */
	val = 0x208;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_MODE_1, val);

	udelay(200);

	/* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
	val = 0x49249;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_MODE_1, val);

	udelay(200);

	pr_info("initialized PMIC\n");

	console_flush();
	imx51_init_lowlevel(800);
	clock_notifier_call_chain();
}

static int imx51_babbage_init(void)
{
	if (!of_machine_is_compatible("fsl,imx51-babbage"))
		return 0;

	barebox_set_hostname("babbage");

	mc13xxx_register_init_callback(babbage_power_init);

	armlinux_set_architecture(MACH_TYPE_MX51_BABBAGE);

	imx51_bbu_internal_mmc_register_handler("mmc", "/dev/mmc0",
		BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}
coredevice_initcall(imx51_babbage_init);

#ifdef CONFIG_ARCH_IMX_XLOAD

static int imx51_babbage_xload_init_pinmux(void)
{
	static const iomux_v3_cfg_t pinmux[] = {
		/* (e)CSPI */
		MX51_PAD_CSPI1_MOSI__ECSPI1_MOSI,
		MX51_PAD_CSPI1_MISO__ECSPI1_MISO,
		MX51_PAD_CSPI1_SCLK__ECSPI1_SCLK,

		/* (e)CSPI chip select lines */
		MX51_PAD_CSPI1_SS1__GPIO4_25,


		/* eSDHC 1 */
		MX51_PAD_SD1_CMD__SD1_CMD,
		MX51_PAD_SD1_CLK__SD1_CLK,
		MX51_PAD_SD1_DATA0__SD1_DATA0,
		MX51_PAD_SD1_DATA1__SD1_DATA1,
		MX51_PAD_SD1_DATA2__SD1_DATA2,
		MX51_PAD_SD1_DATA3__SD1_DATA3,
	};

	mxc_iomux_v3_setup_multiple_pads(ARRAY_AND_SIZE(pinmux));

	return 0;
}
coredevice_initcall(imx51_babbage_xload_init_pinmux);

static int imx51_babbage_xload_init_devices(void)
{
	static int spi0_chipselects[] = {
		IMX_GPIO_NR(4, 25),
	};

	static struct spi_imx_master spi0_pdata = {
		.chipselect = spi0_chipselects,
		.num_chipselect = ARRAY_SIZE(spi0_chipselects),
	};

	static const struct spi_board_info spi0_devices[] = {
		{
			.name		= "mtd_dataflash",
			.chip_select	= 0,
			.max_speed_hz	= 25 * 1000 * 1000,
			.bus_num	= 0,
		},
	};

	imx51_add_mmc0(NULL);

	spi_register_board_info(ARRAY_AND_SIZE(spi0_devices));
	imx51_add_spi0(&spi0_pdata);

	return 0;
}
device_initcall(imx51_babbage_xload_init_devices);

#endif
