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
#include <sizes.h>
#include <gpio.h>
#include <init.h>
#include <fs.h>
#include <io.h>

#include <mfd/mc13xxx.h>
#include <i2c/i2c.h>

#include <asm/armlinux.h>
#include <asm/mmu.h>

#include <generated/mach-types.h>

#include <mach/imx-flash-header.h>
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

#define LOCO_FEC_PHY_RST		IMX_GPIO_NR(7, 6)

static void loco_fec_reset(void)
{
	gpio_direction_output(LOCO_FEC_PHY_RST, 0);
	mdelay(1);
	gpio_set_value(LOCO_FEC_PHY_RST, 1);
}

#define MX53_LOCO_USB_PWREN		IMX_GPIO_NR(7, 8)

extern char flash_header_imx53_loco_start[];
extern char flash_header_imx53_loco_end[];

static int loco_late_init(void)
{
	struct mc13xxx *mc34708;
	int rev;

	if (!of_machine_is_compatible("fsl,imx53-qsb"))
		return 0;

	device_detect_by_name("mmc0");

	devfs_add_partition("mmc0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env0");

	mc34708 = mc13xxx_get();
	if (mc34708) {
		/* get the board revision from fuse */
		rev = readl(MX53_IIM_BASE_ADDR + 0x878);
		set_board_rev(rev);
		printf("MCIMX53-START-R board 1.0 rev %c\n", (rev == 1) ? 'A' : 'B' );
		armlinux_set_revision(loco_system_rev);
	} else {
		/* so we have a DA9053 based board */
		printf("MCIMX53-START board 1.0\n");
		armlinux_set_revision(loco_system_rev);
		return 0;
	}

	/* USB PWR enable */
	gpio_direction_output(MX53_LOCO_USB_PWREN, 0);
	gpio_set_value(MX53_LOCO_USB_PWREN, 1);

	loco_fec_reset();

	set_silicon_rev(imx_silicon_revision());

	armlinux_set_bootparams((void *)0x70000100);
	armlinux_set_architecture(MACH_TYPE_MX53_LOCO);

	imx53_bbu_internal_mmc_register_handler("mmc", "/dev/mmc0",
		BBU_HANDLER_FLAG_DEFAULT, (void *)flash_header_imx53_loco_start,
		flash_header_imx53_loco_end - flash_header_imx53_loco_start, 0);

	return 0;
}
late_initcall(loco_late_init);

static int loco_postcore_init(void)
{
	if (!of_machine_is_compatible("fsl,imx53-qsb"))
		return 0;

	imx53_init_lowlevel(1000);

	return 0;
}
postcore_initcall(loco_postcore_init);
