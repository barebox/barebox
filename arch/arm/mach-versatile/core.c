/*
 * Copyright (C) 2010 B Labs Ltd,
 * http://l4dev.org
 * Author: Alexey Zaytsev <alexey.zaytsev@gmail.com>
 *
 * Based on mach-nomadik
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Copyright (C) 1999 - 2003 ARM Limited
 * Copyright (C) 2000 Deep Blue Solutions Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <debug_ll.h>
#include <sizes.h>

#include <linux/clkdev.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/amba/bus.h>

#include <io.h>
#include <asm/hardware/arm_timer.h>
#include <asm/armlinux.h>

#include <mach/platform.h>
#include <mach/init.h>

void versatile_add_sdram(u32 size)
{
	arm_add_mem_device("ram0", 0x00000000, size);
}

struct clk {
	unsigned long rate;
};

static struct clk ref_clk_dummy;

static struct clk ref_clk_24 = {
	.rate = 24000000,
};

unsigned long clk_get_rate(struct clk *clk)
{
	return clk->rate;
}
EXPORT_SYMBOL(clk_get_rate);

/* enable and disable do nothing */
int clk_enable(struct clk *clk)
{
	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_disable);

/* Create a clock structure with the given name */
int vpb_clk_create(struct clk *clk, const char *dev_id)
{
	struct clk_lookup *clkdev;

	clkdev = clkdev_alloc(clk, NULL, dev_id);
	if (!clkdev)
		return -ENOMEM;

	clkdev_add(clkdev);
	return 0;
}

/* 1Mhz / 256 */
#define TIMER_FREQ (1000000/256)

#define TIMER0_BASE (VERSATILE_TIMER0_1_BASE)
#define TIMER1_BASE ((VERSATILE_TIMER0_1_BASE) + 0x20)
#define TIMER2_BASE (VERSATILE_TIMER2_3_BASE)
#define TIMER3_BASE ((VERSATILE_TIMER2_3_BASE) + 0x20)

static uint64_t vpb_clocksource_read(void)
{
	return ~readl(TIMER0_BASE + TIMER_VALUE);
}

static struct clocksource vpb_cs = {
	.read = vpb_clocksource_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 10,
};

/* From Linux v2.6.35
 * arch/arm/mach-versatile/core.c */
static void versatile_timer_init (void)
{
	u32 val;

	/*
	 * set clock frequency:
	 *      VERSATILE_REFCLK is 32KHz
	 *      VERSATILE_TIMCLK is 1MHz
	 */

	val = readl(VERSATILE_SCTL_BASE);
	val |= (VERSATILE_TIMCLK << VERSATILE_TIMER1_EnSel);
	writel(val, VERSATILE_SCTL_BASE);

	/*
	 * Disable all timers, just to be sure.
	 */

	writel(0, TIMER0_BASE + TIMER_CTRL);
	writel(0, TIMER1_BASE + TIMER_CTRL);
	writel(0, TIMER2_BASE + TIMER_CTRL);
	writel(0, TIMER3_BASE + TIMER_CTRL);

	writel(TIMER_CTRL_32BIT | TIMER_CTRL_ENABLE | TIMER_CTRL_DIV256,
					TIMER0_BASE + TIMER_CTRL);
}

static int vpb_clocksource_init(void)
{
	versatile_timer_init();
	vpb_cs.mult = clocksource_hz2mult(TIMER_FREQ, vpb_cs.shift);

	return init_clock(&vpb_cs);
}

core_initcall(vpb_clocksource_init);

static struct clk_lookup clocks_lookups[] = {
	CLKDEV_CON_ID("apb_pclk", &ref_clk_dummy),
	CLKDEV_DEV_ID("uart-pl0110", &ref_clk_24),
	CLKDEV_DEV_ID("uart-pl0111", &ref_clk_24),
	CLKDEV_DEV_ID("uart-pl0112", &ref_clk_24),
	CLKDEV_DEV_ID("uart-pl0113", &ref_clk_24),
};

static int versatile_clkdev_init(void)
{
	clkdev_add_table(clocks_lookups, ARRAY_SIZE(clocks_lookups));

	return 0;
}
postcore_initcall(versatile_clkdev_init);

void versatile_register_uart(unsigned id)
{
	resource_size_t start;

	switch (id) {
	case 0:
		start = VERSATILE_UART0_BASE;
		break;
	case 1:
		start = VERSATILE_UART1_BASE;
		break;
	case 2:
		start = VERSATILE_UART2_BASE;
		break;
	case 3:
		start = VERSATILE_UART3_BASE;
		break;
	default:
		return;
	}
	amba_apb_device_add(NULL, "uart-pl011", id, start, 4096, NULL, 0);
}

void versatile_register_i2c(void)
{
	add_generic_device("versatile-i2c", DEVICE_ID_DYNAMIC, NULL,
			VERSATILE_I2C_BASE, SZ_4K, IORESOURCE_MEM, NULL);
}

void __noreturn reset_cpu (unsigned long ignored)
{
	u32 val;

	val = __raw_readl(VERSATILE_SYS_RESETCTL) & ~0x7;
	val |= 0x105;

	__raw_writel(0xa05f, VERSATILE_SYS_LOCK);
	__raw_writel(val, VERSATILE_SYS_RESETCTL);
	__raw_writel(0, VERSATILE_SYS_LOCK);

	while(1);
}
EXPORT_SYMBOL(reset_cpu);

static int versatile_init(void)
{
	amba_apb_device_add(NULL, "pl061_gpio", 0, 0x101e4000, 4096, NULL, 0);
	amba_apb_device_add(NULL, "pl061_gpio", 1, 0x101e5000, 4096, NULL, 0);
	amba_apb_device_add(NULL, "pl061_gpio", 2, 0x101e6000, 4096, NULL, 0);
	amba_apb_device_add(NULL, "pl061_gpio", 3, 0x101e7000, 4096, NULL, 0);
	return 0;
}
coredevice_initcall(versatile_init);
