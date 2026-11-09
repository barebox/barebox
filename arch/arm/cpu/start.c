// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#define pr_fmt(fmt) "start.c: " fmt

#ifdef CONFIG_DEBUG_INITCALLS
#define DEBUG
#endif

#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <of.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/secure.h>
#include <asm/unaligned.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <memory.h>
#include <pbl/barebox-proper.h>
#include <pbl/handoff-data.h>
#include <asm/armv7r-mpu.h>

#include <debug_ll.h>

#include "entry.h"

u32 barebox_arm_machine(void)
{
	size_t size;
	unsigned int *machine;

	machine = handoff_data_get_entry(HANDOFF_DATA_ARM_MACHINE, &size);

	if (machine)
		return *machine;

	return 0;
}

#ifdef CONFIG_ARMV7R_MPU
void arch_adjust_malloc_end(unsigned long *malloc_end)
{
	*malloc_end = ALIGN_DOWN(*malloc_end - SZ_8M, SZ_8M);

	armv7r_mpu_init_coherent(*malloc_end, REGION_8MB);
}
#endif

__noreturn void arm_barebox_non_pbl_start(unsigned long membase,
					  unsigned long memsize,
					  struct handoff_data *hd)
{
	unsigned long endmem = membase + memsize;
	unsigned long barebox_base = arm_mem_barebox_image(membase, endmem,
							   barebox_image_size,
							   hd);

	if (IS_ENABLED(CONFIG_CPU_V7VE))
		armv7_hyp_install();

	pbl_barebox_break();

	register_barebox_area(barebox_base,
			      arm_mem_barebox_image_end(endmem) - barebox_base);

	barebox_non_pbl_start(membase, memsize, hd);
}
