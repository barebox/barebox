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

static void babbage_power_init(void)
{
	struct mc13xxx *mc13xxx;
	u32 val;

	mc13xxx = mc13xxx_get();
	if (!mc13xxx) {
		printf("could not get PMIC\n");
		return;
	}

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
#define GPIO_LAN8700_RESET	(1 * 32 + 14)

	/* Reset the ethernet controller over GPIO */
	gpio_direction_output(GPIO_LAN8700_RESET, 0);

	/* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
	val = 0x49249;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_MODE_1, val);

	udelay(200);

	gpio_set_value(GPIO_LAN8700_RESET, 1);

	mdelay(50);
}

extern char flash_header_imx51_babbage_start[];
extern char flash_header_imx51_babbage_end[];

static int imx51_babbage_late_init(void)
{
	if (!of_machine_is_compatible("fsl,imx51-babbage"))
		return 0;

	babbage_power_init();

	console_flush();
	imx51_init_lowlevel(800);
	clock_notifier_call_chain();

	armlinux_set_bootparams((void *)0x90000100);
	armlinux_set_architecture(MACH_TYPE_MX51_BABBAGE);

	imx51_bbu_internal_mmc_register_handler("mmc", "/dev/mmc0",
		BBU_HANDLER_FLAG_DEFAULT, (void *)flash_header_imx51_babbage_start,
		flash_header_imx51_babbage_end - flash_header_imx51_babbage_start, 0);

	return 0;
}
late_initcall(imx51_babbage_late_init);
