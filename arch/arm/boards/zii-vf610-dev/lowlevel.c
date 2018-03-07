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

#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/vf610-regs.h>
#include <mach/clock-vf610.h>
#include <mach/iomux-vf610.h>
#include <debug_ll.h>

static inline void setup_uart(void)
{
	void __iomem *iomux = IOMEM(VF610_IOMUXC_BASE_ADDR);

	vf610_ungate_all_peripherals();
	vf610_setup_pad(iomux, VF610_PAD_PTB10__UART0_TX);
	vf610_uart_setup_ll();

	putc_ll('>');
}

enum zii_platform_vf610_type {
	ZII_PLATFORM_VF610_DEV_REV_B	= 0x01,
	ZII_PLATFORM_VF610_SCU4_AIB	= 0x02,
	ZII_PLATFORM_VF610_SPU3		= 0x03,
	ZII_PLATFORM_VF610_CFU1		= 0x04,
	ZII_PLATFORM_VF610_DEV_REV_C	= 0x05,
};

unsigned int get_system_type(void)
{
#define GPIO_PDIR 0x10

	u32 pdir;
	void __iomem *gpio2 = IOMEM(VF610_GPIO2_BASE_ADDR);
	void __iomem *iomux = IOMEM(VF610_IOMUXC_BASE_ADDR);
	unsigned low, high;

	/*
	 * System type is encoded as a 4-bit number specified by the
	 * following pins (pulled up or down with resistors on the
	 * board).
	*/
	vf610_setup_pad(iomux, VF610_PAD_PTD16__GPIO_78);
	vf610_setup_pad(iomux, VF610_PAD_PTD17__GPIO_77);
	vf610_setup_pad(iomux, VF610_PAD_PTD18__GPIO_76);
	vf610_setup_pad(iomux, VF610_PAD_PTD19__GPIO_75);

	pdir = readl(gpio2 + GPIO_PDIR);

	low  = 75 % 32;
	high = 78 % 32;

	pdir &= GENMASK(high, low);
	pdir >>= low;

	return pdir;
}

extern char __dtb_vf610_zii_dev_rev_b_start[];
extern char __dtb_vf610_zii_dev_rev_c_start[];
extern char __dtb_vf610_zii_cfu1_rev_a_start[];
extern char __dtb_vf610_zii_spu3_rev_a_start[];
extern char __dtb_vf610_zii_scu4_aib_rev_c_start[];

ENTRY_FUNCTION(start_zii_vf610_dev, r0, r1, r2)
{
	void *fdt;
	const unsigned int system_type = get_system_type();

	vf610_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	switch (system_type) {
	default:
		/*
		 * GCC can be smart enough to, when DEBUG_LL is
		 * disabled, reduce this switch statement to a LUT
		 * fetch. Unfortunately here, this early in the boot
		 * process before any relocation/address fixups could
		 * happen, the address of that LUT used by the code is
		 * incorrect and any access to it would result in
		 * bogus values.
		 *
		 * Adding the following barrier() statement seem to
		 * force the compiler to always translate this block
		 * to a sequence of consecutive checks and jumps with
		 * relative fetches, which should work with or without
		 * relocation/fixups.
		 */
		barrier();

		if (IS_ENABLED(CONFIG_DEBUG_LL)) {
			relocate_to_current_adr();
			setup_c();
			puts_ll("*********************************\n");
			puts_ll("* Unknown system type: ");
			puthex_ll(system_type);
			puts_ll("\n* Assuming devboard revision B\n");
			puts_ll("*********************************\n");
		}
	case ZII_PLATFORM_VF610_DEV_REV_B: /* FALLTHROUGH */
		fdt = __dtb_vf610_zii_dev_rev_b_start;
		break;
	case ZII_PLATFORM_VF610_SCU4_AIB:
		fdt = __dtb_vf610_zii_scu4_aib_rev_c_start;
		break;
	case ZII_PLATFORM_VF610_DEV_REV_C:
		fdt = __dtb_vf610_zii_dev_rev_c_start;
		break;
	case ZII_PLATFORM_VF610_CFU1:
		fdt = __dtb_vf610_zii_cfu1_rev_a_start;
		break;
	case ZII_PLATFORM_VF610_SPU3:
		fdt = __dtb_vf610_zii_spu3_rev_a_start;
		break;
	}

	barebox_arm_entry(0x80000000, SZ_512M, fdt + get_runtime_offset());
}
