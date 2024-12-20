// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/errata.h>
#include <mach/zynq/init.h>

void zynq_cpu_lowlevel_init(void)
{
	arm_cpu_lowlevel_init();

	enable_arm_errata_761320_war();
	enable_arm_errata_794072_war();
	enable_arm_errata_845369_war();
}
