// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2015 Zodiac Inflight Innovation

/* Author: Andrey Smirnov <andrew.smirnov@gmail.com> */

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
#include <asm/cputype.h>

#define __exceptions_size (__exceptions_stop - __exceptions_start)

static bool has_vbar(void)
{
	u32 mainid;

	__asm__ __volatile__(
		"mrc    p15, 0, %0, c0, c0, 0   @ read control reg\n"
		: "=r" (mainid)
		:
		: "memory");

	if ((mainid & 0xfff0) == ARM_CPU_PART_CORTEX_R5)
		return false;

	return true;
}

static int nommu_v7_vectors_init(void)
{
	void *vectors;
	u32 cr;

	if (cpu_architecture() < CPU_ARCH_ARMv7)
		return 0;

	if (!has_vbar())
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
