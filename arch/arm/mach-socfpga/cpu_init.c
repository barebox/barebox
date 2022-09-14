// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/errata.h>
#include <mach/init.h>

void arria10_cpu_lowlevel_init(void)
{
	enable_arm_errata_794072_war();
	enable_arm_errata_845369_war();
}
