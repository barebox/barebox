// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2012 Alexander Shiyan <shc_work@mail.ru>

#include <common.h>
#include <init.h>

#include <asm/barebox-arm-head.h>

#include <mach/clps711x.h>

#ifdef CONFIG_CLPS711X_RAISE_CPUFREQ
# define CLPS711X_CPU_PLL_MULT	50
#else
# define CLPS711X_CPU_PLL_MULT	40
#endif

void __naked __bare_init barebox_arm_reset_vector(uint32_t r0, uint32_t r1, uint32_t r2)
{
	arm_cpu_lowlevel_init();

	clps711x_barebox_entry(CLPS711X_CPU_PLL_MULT, NULL);
}
