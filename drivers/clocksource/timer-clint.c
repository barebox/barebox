// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Western Digital Corporation or its affiliates.
 *
 * Most of the M-mode (i.e. NoMMU) RISC-V systems usually have a
 * CLINT MMIO timer device.
 */

#define pr_fmt(fmt) "clint: " fmt

#include <common.h>
#include <init.h>
#include <clock.h>
#include <errno.h>
#include <of.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <io.h>
#include <asm/timer.h>
#include <asm/system.h>

#define CLINT_TIMER_VAL_OFF	0xbff8

#ifdef CONFIG_64BIT
#define clint_get_cycles()	readq(clint_timer_val)
#else
#define clint_get_cycles()	readl(clint_timer_val)
#define clint_get_cycles_hi()	readl(((u32 *)clint_timer_val) + 1)
#endif

static void __iomem *clint_timer_val;

#ifdef CONFIG_64BIT
static u64 notrace clint_get_cycles64(void)
{
	return clint_get_cycles();
}
#else /* CONFIG_64BIT */
static u64 notrace clint_get_cycles64(void)
{
	u32 hi, lo;

	do {
		hi = clint_get_cycles_hi();
		lo = clint_get_cycles();
	} while (hi != clint_get_cycles_hi());

	return ((u64)hi << 32) | lo;
}
#endif /* CONFIG_64BIT */

static u64 clint_rdtime(void)
{
	return clint_get_cycles64();
}

static struct clocksource clint_clocksource = {
	.read		= clint_rdtime,
	.mask		= CLOCKSOURCE_MASK(64),
	.priority	= 200,
};

static int clint_timer_init_dt(struct device * dev)
{
	struct resource *iores;

	/* one timer is enough. Only M-Mode */
	if (clint_timer_val || riscv_mode() != RISCV_M_MODE)
		return 0;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	clint_timer_val = IOMEM(iores->start) + CLINT_TIMER_VAL_OFF;

	dev_dbg(dev, "running at %lu Hz\n", riscv_timebase);

	clint_clocksource.mult = clocksource_hz2mult(riscv_timebase, clint_clocksource.shift);

	return init_clock(&clint_clocksource);
}

static struct of_device_id timer_clint_dt_ids[] = {
	{ .compatible = "riscv,clint0", },
	{ .compatible = "sifive,clint0" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, timer_clint_dt_ids);

static struct driver clint_timer_driver = {
	.name = "clint-timer",
	.probe = clint_timer_init_dt,
	.of_compatible = timer_clint_dt_ids,
};
postcore_platform_driver(clint_timer_driver);
