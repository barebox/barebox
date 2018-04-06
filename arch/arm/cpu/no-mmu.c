/*
 * Copyright (c) 2015 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt)	"nommu: " fmt

#include <common.h>
#include <dma-dir.h>
#include <init.h>
#include <mmu.h>
#include <errno.h>
#include <linux/sizes.h>
#include <asm/memory.h>
#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <asm/cache.h>
#include <memory.h>
#include <asm/system_info.h>
#include <debug_ll.h>
#include <asm/sections.h>

#define __exceptions_size (__exceptions_stop - __exceptions_start)

static int nommu_v7_vectors_init(void)
{
	void *vectors;
	u32 cr;

	if (cpu_architecture() < CPU_ARCH_ARMv7)
		return 0;

	/*
	 * High vectors cannot be re-mapped, so we have to use normal
	 * vectors
	 */
	cr = get_cr();
	cr &= ~CR_V;
	set_cr(cr);

	arm_fixup_vectors();

	vectors = xmemalign(PAGE_SIZE, PAGE_SIZE);
	memset(vectors, 0, PAGE_SIZE);
	memcpy(vectors, __exceptions_start, __exceptions_size);

	set_vbar((unsigned int)vectors);

	return 0;
}
mmu_initcall(nommu_v7_vectors_init);
