// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/mxs/imx23-regs.h>
#include <asm/mach-types.h>

static noinline void continue_imx_entry(size_t size)
{
	handoff_add_arm_machine(MACH_TYPE_CHUMBY);

	barebox_arm_entry(IMX_MEMORY_BASE, size, NULL);
}

ENTRY_FUNCTION(start_chumby_falconwing, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	continue_imx_entry(SZ_64M);
}
