/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 * Copyright (C) 2011 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <environment.h>
#include <partition.h>
#include <common.h>
#include <linux/sizes.h>
#include <gpio.h>
#include <init.h>
#include <fs.h>
#include <io.h>
#include <of.h>

#include <mfd/mc13xxx.h>
#include <i2c/i2c.h>

#include <asm/armlinux.h>
#include <asm/mmu.h>

#include <generated/mach-types.h>

#include <mach/imx53-regs.h>
#include <mach/revision.h>
#include <mach/generic.h>
#include <mach/imx5.h>
#include <mach/bbu.h>
#include <mach/iim.h>

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
 * e.g 0x00053120 --> i.MX35, Cpu silicon rev 2.0, Board rev 2
*/
static unsigned int loco_system_rev = 0x00053000;

static void set_silicon_rev( int rev)
{
	loco_system_rev = loco_system_rev | (rev & 0xFF);
}

static void set_board_rev(int rev)
{
	loco_system_rev =  (loco_system_rev & ~(0xF << 8)) | (rev & 0xF) << 8;
}

#define MX53_LOCO_USB_PWREN		IMX_GPIO_NR(7, 8)

static int loco_late_init(void)
{
	struct mc13xxx *mc34708;
	int rev;

	if (!of_machine_is_compatible("fsl,imx53-qsb") &&
	    !of_machine_is_compatible("fsl,imx53-qsrb"))
		return 0;

	mc34708 = mc13xxx_get();
	if (mc34708) {
		unsigned int val;
		int ret;
		/* get the board revision from fuse */
		rev = readl(MX53_IIM_BASE_ADDR + 0x878);
		set_board_rev(rev);
		printf("MCIMX53-START-R board 1.0 rev %c\n", (rev == 1) ? 'A' : 'B' );
		barebox_set_hostname("loco-r");
		armlinux_set_revision(loco_system_rev);
		/* Set VDDGP to 1.25V for 1GHz on SW1 */
		mc13xxx_reg_read(mc34708, MC13892_REG_SW_0, &val);
		val = (val & ~SWx_VOLT_MASK_MC34708) | SWx_1_250V_MC34708;
		ret = mc13xxx_reg_write(mc34708, MC13892_REG_SW_0, val);
		if (ret) {
			printf("Writing to REG_SW_0 failed: %d\n", ret);
			return ret;
		}

		/* Set VCC as 1.30V on SW2 */
		mc13xxx_reg_read(mc34708, MC13892_REG_SW_1, &val);
		val = (val & ~SWx_VOLT_MASK_MC34708) | SWx_1_300V_MC34708;
		ret = mc13xxx_reg_write(mc34708, MC13892_REG_SW_1, val);
		if (ret) {
			printf("Writing to REG_SW_1 failed: %d\n", ret);
			return ret;
		}

		/* Set global reset timer to 4s */
		mc13xxx_reg_read(mc34708, MC13892_REG_POWER_CTL2, &val);
		val = (val & ~TIMER_MASK_MC34708) | TIMER_4S_MC34708;
		ret = mc13xxx_reg_write(mc34708, MC13892_REG_POWER_CTL2, val);
		if (ret) {
			printf("Writing to REG_POWER_CTL2 failed: %d\n", ret);
			return ret;
		}

		/* Set VUSBSEL and VUSBEN for USB PHY supply*/
		mc13xxx_reg_read(mc34708, MC13892_REG_MODE_0, &val);
		val |= (VUSBSEL_MC34708 | VUSBEN_MC34708);
		ret = mc13xxx_reg_write(mc34708, MC13892_REG_MODE_0, val);
		if (ret) {
			printf("Writing to REG_MODE_0 failed: %d\n", ret);
			return ret;
		}

		/* Set SWBST to 5V in auto mode */
		val = SWBST_AUTO;
		ret = mc13xxx_reg_write(mc34708, SWBST_CTRL, val);
		if (ret) {
			printf("Writing to SWBST_CTRL failed: %d\n", ret);
			return ret;
		}
	} else {
		/* so we have a DA9053 based board */
		printf("MCIMX53-START board 1.0\n");
		barebox_set_hostname("loco");
		armlinux_set_revision(loco_system_rev);
	}

	/* USB PWR enable */
	gpio_direction_output(MX53_LOCO_USB_PWREN, 0);
	gpio_set_value(MX53_LOCO_USB_PWREN, 1);

	set_silicon_rev(imx_silicon_revision());

	armlinux_set_architecture(MACH_TYPE_MX53_LOCO);

	imx53_bbu_internal_mmc_register_handler("mmc", "/dev/mmc0",
		BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}
late_initcall(loco_late_init);

static int loco_postcore_init(void)
{
	if (!of_machine_is_compatible("fsl,imx53-qsb") &&
	    !of_machine_is_compatible("fsl,imx53-qsrb"))
		return 0;

	imx53_init_lowlevel(1000);

	return 0;
}
postcore_initcall(loco_postcore_init);
