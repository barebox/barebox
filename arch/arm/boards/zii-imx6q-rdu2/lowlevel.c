/*
 * Copyright (C) 2016 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
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

#include <debug_ll.h>
#include <common.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <mach/imx6.h>
#include <mach/xload.h>
#include <mach/iomux-mx6.h>
#include <asm/barebox-arm.h>

struct reginit {
	u32 address;
	u32 value;
};

static const struct reginit imx6qp_dcd[] = {
	{ 0x020e0798, 0x000C0000 },
	{ 0x020e0758, 0x00000000 },

	{ 0x020e0588, 0x00020030 },
	{ 0x020e0594, 0x00020030 },

	{ 0x020e056c, 0x00020030 },
	{ 0x020e0578, 0x00020030 },
	{ 0x020e074c, 0x00020030 },

	{ 0x020e057c, 0x00020030 },
	{ 0x020e058c, 0x00000000 },
	{ 0x020e059c, 0x00020030 },
	{ 0x020e05a0, 0x00020030 },
	{ 0x020e078c, 0x00020030 },

	{ 0x020e0750, 0x00020000 },
	{ 0x020e05a8, 0x00020030 },
	{ 0x020e05b0, 0x00020030 },
	{ 0x020e0524, 0x00020030 },
	{ 0x020e051c, 0x00020030 },
	{ 0x020e0518, 0x00020030 },
	{ 0x020e050c, 0x00020030 },
	{ 0x020e05b8, 0x00020030 },
	{ 0x020e05c0, 0x00020030 },

	{ 0x020e0534, 0x00018200 },
	{ 0x020e0538, 0x00008000 },
	{ 0x020e053c, 0x00018200 },
	{ 0x020e0540, 0x00018200 },
	{ 0x020e0544, 0x00018200 },
	{ 0x020e0548, 0x00018200 },
	{ 0x020e054c, 0x00018200 },
	{ 0x020e0550, 0x00018200 },

	{ 0x020e0774, 0x00020000 },
	{ 0x020e0784, 0x00020030 },
	{ 0x020e0788, 0x00020030 },
	{ 0x020e0794, 0x00020030 },
	{ 0x020e079c, 0x00020030 },
	{ 0x020e07a0, 0x00020030 },
	{ 0x020e07a4, 0x00020030 },
	{ 0x020e07a8, 0x00020030 },
	{ 0x020e0748, 0x00020030 },

	{ 0x020e05ac, 0x00020030 },
	{ 0x020e05b4, 0x00020030 },
	{ 0x020e0528, 0x00020030 },
	{ 0x020e0520, 0x00020030 },
	{ 0x020e0514, 0x00020030 },
	{ 0x020e0510, 0x00020030 },
	{ 0x020e05bc, 0x00020030 },
	{ 0x020e05c4, 0x00020030 },

	{ 0x021b001c, 0x00008000 },

	{ 0x021b0800, 0xA1390003 },

	{ 0x021b080c, 0x002A001F },
	{ 0x021b0810, 0x002F002A },
	{ 0x021b480c, 0x001F0031 },
	{ 0x021b4810, 0x001B0022 },

	{ 0x021b083c, 0x433C0354 },
	{ 0x021b0840, 0x03380330 },
	{ 0x021b483c, 0x43440358 },
	{ 0x021b4840, 0x03340300 },

	{ 0x021b0848, 0x483A4040 },
	{ 0x021b4848, 0x3E383648 },

	{ 0x021b0850, 0x3C424048 },
	{ 0x021b4850, 0x4C425042 },

	{ 0x021b081c, 0x33333333 },
	{ 0x021b0820, 0x33333333 },
	{ 0x021b0824, 0x33333333 },
	{ 0x021b0828, 0x33333333 },
	{ 0x021b481c, 0x33333333 },
	{ 0x021b4820, 0x33333333 },
	{ 0x021b4824, 0x33333333 },
	{ 0x021b4828, 0x33333333 },

	{ 0x021b08c0, 0x24912489 },
	{ 0x021b48c0, 0x24914452 },

	{ 0x021b08b8, 0x00000800 },
	{ 0x021b48b8, 0x00000800 },

	{ 0x021b0004, 0x00020036 },
	{ 0x021b0008, 0x09444040 },
	{ 0x021b000c, 0x898E7955 },
	{ 0x021b0010, 0xFF328F64 },
	{ 0x021b0014, 0x01FF00DB },

	{ 0x021b0018, 0x00011740 },
	{ 0x021b001c, 0x00008000 },
	{ 0x021b002c, 0x000026D2 },
	{ 0x021b0030, 0x008E1023 },
	{ 0x021b0040, 0x00000047 },

	{ 0x021b0400, 0x14420000 },
	{ 0x021b0000, 0x841A0000 },
	{ 0x021b0890, 0x00400c58 },

	{ 0x00bb0008, 0x00000000 },
	{ 0x00bb000c, 0x2891E41A },
	{ 0x00bb0038, 0x00000564 },
	{ 0x00bb0014, 0x00000040 },
	{ 0x00bb0028, 0x00000020 },
	{ 0x00bb002c, 0x00000020 },

	{ 0x021b001c, 0x02088032 },
	{ 0x021b001c, 0x00008033 },
	{ 0x021b001c, 0x00048031 },
	{ 0x021b001c, 0x19408030 },
	{ 0x021b001c, 0x04008040 },

	{ 0x021b0020, 0x00007800 },

	{ 0x021b0818, 0x00022227 },
	{ 0x021b4818, 0x00022227 },

	{ 0x021b0004, 0x00025576 },

	{ 0x021b0404, 0x00011006 },

	{ 0x021b001c, 0x00000000 },
};

static const struct reginit imx6q_dcd[] = {
	{ 0x020e0798, 0x000C0000 },
	{ 0x020e0758, 0x00000000 },
	{ 0x020e0588, 0x00000030 },
	{ 0x020e0594, 0x00000030 },
	{ 0x020e056c, 0x00000030 },
	{ 0x020e0578, 0x00000030 },
	{ 0x020e074c, 0x00000030 },
	{ 0x020e057c, 0x00000030 },
	{ 0x020e058c, 0x00000000 },
	{ 0x020e059c, 0x00000030 },
	{ 0x020e05a0, 0x00000030 },
	{ 0x020e078c, 0x00000030 },
	{ 0x020e0750, 0x00020000 },
	{ 0x020e05a8, 0x00000028 },
	{ 0x020e05b0, 0x00000028 },
	{ 0x020e0524, 0x00000028 },
	{ 0x020e051c, 0x00000028 },
	{ 0x020e0518, 0x00000028 },
	{ 0x020e050c, 0x00000028 },
	{ 0x020e05b8, 0x00000028 },
	{ 0x020e05c0, 0x00000028 },
	{ 0x020e0774, 0x00020000 },
	{ 0x020e0784, 0x00000028 },
	{ 0x020e0788, 0x00000028 },
	{ 0x020e0794, 0x00000028 },
	{ 0x020e079c, 0x00000028 },
	{ 0x020e07a0, 0x00000028 },
	{ 0x020e07a4, 0x00000028 },
	{ 0x020e07a8, 0x00000028 },
	{ 0x020e0748, 0x00000028 },
	{ 0x020e05ac, 0x00000028 },
	{ 0x020e05b4, 0x00000028 },
	{ 0x020e0528, 0x00000028 },
	{ 0x020e0520, 0x00000028 },
	{ 0x020e0514, 0x00000028 },
	{ 0x020e0510, 0x00000028 },
	{ 0x020e05bc, 0x00000028 },
	{ 0x020e05c4, 0x00000028 },
	{ 0x021b0800, 0xa1390003 },
	{ 0x021b080c, 0x001F001F },
	{ 0x021b0810, 0x001F001F },
	{ 0x021b480c, 0x001F001F },
	{ 0x021b4810, 0x001F001F },
	{ 0x021b083c, 0x43260335 },
	{ 0x021b0840, 0x031A030B },
	{ 0x021b483c, 0x4323033B },
	{ 0x021b4840, 0x0323026F },
	{ 0x021b0848, 0x483D4545 },
	{ 0x021b4848, 0x44433E48 },
	{ 0x021b0850, 0x41444840 },
	{ 0x021b4850, 0x4835483E },
	{ 0x021b081c, 0x33333333 },
	{ 0x021b0820, 0x33333333 },
	{ 0x021b0824, 0x33333333 },
	{ 0x021b0828, 0x33333333 },
	{ 0x021b481c, 0x33333333 },
	{ 0x021b4820, 0x33333333 },
	{ 0x021b4824, 0x33333333 },
	{ 0x021b4828, 0x33333333 },
	{ 0x021b08b8, 0x00000800 },
	{ 0x021b48b8, 0x00000800 },
	{ 0x021b0004, 0x00020036 },
	{ 0x021b0008, 0x09444040 },
	{ 0x021b000c, 0x8A8F7955 },
	{ 0x021b0010, 0xFF328F64 },
	{ 0x021b0014, 0x01FF00DB },
	{ 0x021b0018, 0x00001740 },
	{ 0x021b001c, 0x00008000 },
	{ 0x021b002c, 0x000026d2 },
	{ 0x021b0030, 0x008F1023 },
	{ 0x021b0040, 0x00000047 },
	{ 0x021b0000, 0x841A0000 },
	{ 0x021b001c, 0x04088032 },
	{ 0x021b001c, 0x00008033 },
	{ 0x021b001c, 0x00048031 },
	{ 0x021b001c, 0x09408030 },
	{ 0x021b001c, 0x04008040 },
	{ 0x021b0020, 0x00005800 },
	{ 0x021b0818, 0x00011117 },
	{ 0x021b4818, 0x00011117 },
	{ 0x021b0004, 0x00025576 },
	{ 0x021b0404, 0x00011006 },
	{ 0x021b001c, 0x00000000 },
};

static inline void write_regs(const struct reginit *initvals, int count)
{
	int i;

	for (i = 0; i < count; i++)
		writel(initvals[i].value, initvals[i].address);
}

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);

	imx_setup_pad(iomuxbase, MX6Q_PAD_CSI0_DAT10__UART1_TXD);

	imx6_uart_setup_ll();

	putc_ll('>');
}

extern char __dtb_z_imx6q_zii_rdu2_start[];
extern char __dtb_z_imx6qp_zii_rdu2_start[];

static noinline void rdu2_sram_setup(void)
{
	enum bootsource bootsrc;
	int instance;

	arm_setup_stack(0x00920000);
	relocate_to_current_adr();
	setup_c();

	if (__imx6_cpu_type() == IMX6_CPUTYPE_IMX6QP)
		write_regs(imx6qp_dcd, ARRAY_SIZE(imx6qp_dcd));
	else
		write_regs(imx6q_dcd, ARRAY_SIZE(imx6q_dcd));

	imx6_get_boot_source(&bootsrc, &instance);
	if (bootsrc == BOOTSOURCE_SPI_NOR)
		imx6_spi_start_image(0);
	else
		imx6_esdhc_start_image(instance);
}

ENTRY_FUNCTION(start_imx6_zii_rdu2, r0, r1, r2)
{
	imx6_cpu_lowlevel_init();

	imx6_ungate_all_peripherals();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	/*
	 * When still running in SRAM, we need to setup the DRAM now and load
	 * the remaining image.
	 */
	if (get_pc() < MX6_MMDC_PORT01_BASE_ADDR)
		rdu2_sram_setup();

	if (__imx6_cpu_type() == IMX6_CPUTYPE_IMX6QP)
		imx6q_barebox_entry(__dtb_z_imx6qp_zii_rdu2_start +
				    get_runtime_offset());
	else
		imx6q_barebox_entry(__dtb_z_imx6q_zii_rdu2_start +
				    get_runtime_offset());
}
