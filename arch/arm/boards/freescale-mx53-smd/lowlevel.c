// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <mach/imx53-regs.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>

void __naked barebox_arm_reset_vector(uint32_t r0, uint32_t r1, uint32_t r2)
{
	imx5_cpu_lowlevel_init();
	arm_setup_stack(MX53_IRAM_BASE_ADDR + MX53_IRAM_SIZE);
	imx53_barebox_entry(NULL);
}
