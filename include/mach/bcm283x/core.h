// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2009 Carlo Caione <carlo@carlocaione.org>
 */

#ifndef __BCM2835_CORE_H__
#define __BCM2835_CORE_H__

#include <common.h>
#include <linux/types.h>
#include <linux/sizes.h>
#include <asm/memory.h>
#include <mach/bcm283x/platform.h>

static void inline bcm2835_add_device_sdram(u32 size)
{
	arm_add_mem_device("ram0", BCM2835_SDRAM_BASE, size);
}

static void inline bcm2835_register_fb(void)
{
	add_generic_device("bcm2835_fb", 0, NULL, 0, 0, 0, NULL);
}

void __iomem *bcm2835_get_mmio_base_by_cpuid(void);

#endif
