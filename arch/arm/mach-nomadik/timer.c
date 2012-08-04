/*
 * linux/arch/arm/mach-nomadik/timer.c
 *
 * Copyright (C) 2008 STMicroelectronics
 * Copyright (C) 2009 Alessandro Rubini, somewhat based on at91sam926x
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */
#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/hardware.h>

/* Initial value for SRC control register: all timers use MXTAL/8 source */
#define SRC_CR_INIT_MASK	0x00007fff
#define SRC_CR_INIT_VAL		0x2aaa8000

static int st8815_timer_init(void)
{
	u32 src_cr;

	/* Configure timer sources in "system reset controller" ctrl reg */
	src_cr = readl(NOMADIK_SRC_BASE);
	src_cr &= SRC_CR_INIT_MASK;
	src_cr |= SRC_CR_INIT_VAL;
	writel(src_cr, NOMADIK_SRC_BASE);

	add_generic_device("nomadik_mtu", DEVICE_ID_SINGLE, NULL, NOMADIK_MTU0_BASE, 0x1000, IORESOURCE_MEM, NULL);
	return 0;
}
coredevice_initcall(st8815_timer_init);
