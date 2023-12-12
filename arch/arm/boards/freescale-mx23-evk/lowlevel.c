// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <linux/sizes.h>
#include <asm/mach-types.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/mxs/imx23-regs.h>

static noinline void continue_imx_entry(size_t size)
{
	static struct barebox_arm_boarddata boarddata = {
		.magic = BAREBOX_ARM_BOARDDATA_MAGIC,
		.machine = MACH_TYPE_MX23EVK,
	};

	barebox_arm_entry(IMX_MEMORY_BASE, size, &boarddata);
}

ENTRY_FUNCTION(start_imx23_evk, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	continue_imx_entry(SZ_32M);
}
