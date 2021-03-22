// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 *
 * All RISC-V systems have a timer attached to every hart.  These timers can
 * either be read from the "time" and "timeh" CSRs, and can use the SBI to
 * setup events, or directly accessed using MMIO registers.
 */
#include <common.h>
#include <init.h>
#include <clock.h>
#include <errno.h>
#include <of.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <io.h>
#include <asm/timer.h>

unsigned long riscv_timebase;

int timer_init(void)
{
	struct device_node *cpu;
	u32 prop;

	cpu = of_find_node_by_path("/cpus");
	if (!cpu || of_property_read_u32(cpu, "timebase-frequency", &prop)) {
		pr_err("RISC-V system with no 'timebase-frequency' in DTS\n");
		return -ENOENT;
	}

	riscv_timebase = prop;

	of_platform_populate(cpu, NULL, NULL);

	return 0;
}
