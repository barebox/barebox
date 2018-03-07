/*
 * Copyright (C) 2013 Sascha Hauer <s.hauer@pengutronix.de>
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
 */
#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <debug_ll.h>
#include <asm/sections.h>
#include <asm/mmu.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx6-mmdc.h>
#include <mach/generic.h>

static void sdram_init(void)
{
	writel(0x0, 0x021b0000);
	writel(0xffffffff, 0x020c4068);
	writel(0xffffffff, 0x020c406c);
	writel(0xffffffff, 0x020c4070);
	writel(0xffffffff, 0x020c4074);
	writel(0xffffffff, 0x020c4078);
	writel(0xffffffff, 0x020c407c);
	writel(0xffffffff, 0x020c4080);
	writel(0xffffffff, 0x020c4084);
	writel(0x000C0000, 0x020e0798);
	writel(0x00000000, 0x020e0758);
	writel(0x00000030, 0x020e0588);
	writel(0x00000030, 0x020e0594);
	writel(0x00000030, 0x020e056c);
	writel(0x00000030, 0x020e0578);
	writel(0x00000030, 0x020e074c);
	writel(0x00000030, 0x020e057c);
	writel(0x00000000, 0x020e058c);
	writel(0x00000030, 0x020e059c);
	writel(0x00000030, 0x020e05a0);
	writel(0x00000030, 0x020e078c);
	writel(0x00020000, 0x020e0750);
	writel(0x00000038, 0x020e05a8);
	writel(0x00000038, 0x020e05b0);
	writel(0x00000038, 0x020e0524);
	writel(0x00000038, 0x020e051c);
	writel(0x00000038, 0x020e0518);
	writel(0x00000038, 0x020e050c);
	writel(0x00000038, 0x020e05b8);
	writel(0x00000038, 0x020e05c0);
	writel(0x00020000, 0x020e0774);
	writel(0x00000030, 0x020e0784);
	writel(0x00000030, 0x020e0788);
	writel(0x00000030, 0x020e0794);
	writel(0x00000030, 0x020e079c);
	writel(0x00000030, 0x020e07a0);
	writel(0x00000030, 0x020e07a4);
	writel(0x00000030, 0x020e07a8);
	writel(0x00000030, 0x020e0748);
	writel(0x00000030, 0x020e05ac);
	writel(0x00000030, 0x020e05b4);
	writel(0x00000030, 0x020e0528);
	writel(0x00000030, 0x020e0520);
	writel(0x00000030, 0x020e0514);
	writel(0x00000030, 0x020e0510);
	writel(0x00000030, 0x020e05bc);
	writel(0x00000030, 0x020e05c4);
	writel(0xa1390003, 0x021b0800);
	writel(0x0059005C, 0x021b080c);
	writel(0x00590056, 0x021b0810);
	writel(0x002E0049, 0x021b480c);
	writel(0x001B0033, 0x021b4810);
	writel(0x434F035B, 0x021b083c);
	writel(0x033F033F, 0x021b0840);
	writel(0x4337033D, 0x021b483c);
	writel(0x03210275, 0x021b4840);
	writel(0x4C454344, 0x021b0848);
	writel(0x463F3E4A, 0x021b4848);
	writel(0x46314742, 0x021b0850);
	writel(0x4D2A4B39, 0x021b4850);
	writel(0x33333333, 0x021b081c);
	writel(0x33333333, 0x021b0820);
	writel(0x33333333, 0x021b0824);
	writel(0x33333333, 0x021b0828);
	writel(0x33333333, 0x021b481c);
	writel(0x33333333, 0x021b4820);
	writel(0x33333333, 0x021b4824);
	writel(0x33333333, 0x021b4828);
	writel(0x00000800, 0x021b08b8);
	writel(0x00000800, 0x021b48b8);
	writel(0x00020036, 0x021b0004);
	writel(0x09555050, 0x021b0008);
	writel(0x8A8F7934, 0x021b000c);
	writel(0xDB568E65, 0x021b0010);
	writel(0x01FF00DB, 0x021b0014);
	writel(0x00011740, 0x021b0018);
	writel(0x00008000, 0x021b001c);
	writel(0x000026d2, 0x021b002c);
	writel(0x008F0E21, 0x021b0030);
	writel(0x0000007f, 0x021b0040);
	writel(0x114201f0, 0x021b0400);
	writel(0x11420000, 0x021b4400);
	writel(0x841A0000, 0x021b0000);
	writel(0x04108032, 0x021b001c);
	writel(0x00028033, 0x021b001c);
	writel(0x00048031, 0x021b001c);
	writel(0x09308030, 0x021b001c);
	writel(0x04008040, 0x021b001c);
	writel(0x00005800, 0x021b0020);
	writel(0x00011117, 0x021b0818);
	writel(0x00011117, 0x021b4818);
	writel(0x00025576, 0x021b0004);
	writel(0x00011006, 0x021b0404);
	writel(0x00000000, 0x021b001c);

	/* Enable UART for lowlevel debugging purposes. Can be removed later */
	writel(0x4, 0x020e00bc);
	writel(0x4, 0x020e00c0);
	writel(0x1, 0x020e0928);
	writel(0x00000000, 0x021e8080);
	writel(0x00004027, 0x021e8084);
	writel(0x00000704, 0x021e8088);
	writel(0x00000a81, 0x021e8090);
	writel(0x0000002b, 0x021e809c);
	writel(0x00013880, 0x021e80b0);
	writel(0x0000047f, 0x021e80a4);
	writel(0x0000c34f, 0x021e80a8);
	writel(0x00000001, 0x021e8080);
	putc_ll('>');
}

extern char __dtb_imx6q_dmo_edmqmx6_start[];

ENTRY_FUNCTION(start_imx6_realq7, r0, r1, r2)
{
	unsigned long sdram = 0x10000000;
	void *fdt;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00940000 - 8);

	fdt = __dtb_imx6q_dmo_edmqmx6_start + get_runtime_offset();

	if (get_pc() < 0x10000000) {
		sdram_init();

		mmdc_do_write_level_calibration();
		mmdc_do_dqs_calibration();
	}

	barebox_arm_entry(sdram, SZ_2G, fdt);
}
