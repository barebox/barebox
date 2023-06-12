// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 * Copyright (C) 2021 Ahmad Fatoum, Pengutronix
 *
 * Common RISC-V core initcalls.
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
#include <memory.h>
#include <asm/barebox-riscv.h>
#include <asm-generic/memory_layout.h>
#include <globalvar.h>
#include <magicvar.h>
#include <asm/system.h>
#include <io.h>

static int riscv_request_stack(void)
{
	extern unsigned long riscv_stack_top;
	return PTR_ERR_OR_ZERO(request_sdram_region("stack", riscv_stack_top - STACK_SIZE, STACK_SIZE));
}
coredevice_initcall(riscv_request_stack);

static struct device timer_dev;
static struct device serial_sbi_dev;

static s64 hartid;

BAREBOX_MAGICVAR(global.hartid, "RISC-V hartid");

static int riscv_fixup_cpus(struct device_node *root, void *context)
{
	struct device_node *cpus_node, *np, *tmp;

	cpus_node = of_find_node_by_name_address(root, "cpus");
	if (!cpus_node)
		return 0;

	for_each_child_of_node_safe(cpus_node, tmp, np) {
		u32 cpu_index;

		if (of_property_read_u32(np, "reg", &cpu_index))
			continue;

		if (cpu_index != hartid)
			of_delete_node(np);
	}

	return 0;
}

static int riscv_probe(struct device *parent)
{
	int ret;

	/* Each hart has a timer, but we only need one */
	if (IS_ENABLED(CONFIG_RISCV_TIMER) && !timer_dev.parent) {
		timer_dev.id = DEVICE_ID_SINGLE;
		timer_dev.of_node = parent->of_node;
		timer_dev.parent = parent;
		dev_set_name(&timer_dev, "riscv-timer");

		ret = platform_device_register(&timer_dev);
		if (ret)
			return ret;
	}

	if (IS_ENABLED(CONFIG_SERIAL_SBI) && !serial_sbi_dev.parent) {
		serial_sbi_dev.id = DEVICE_ID_SINGLE;
		serial_sbi_dev.of_node = 0;
		serial_sbi_dev.parent = parent;
		dev_set_name(&serial_sbi_dev, "riscv-serial-sbi");

		ret = platform_device_register(&serial_sbi_dev);
		if (ret)
			return ret;
	}

	hartid = riscv_hartid();
	if (hartid >= 0)
		globalvar_add_simple_uint64("hartid", &hartid, "%llu");

	return of_register_fixup(riscv_fixup_cpus, NULL);
}

static struct of_device_id riscv_dt_ids[] = {
	{ .compatible = "riscv", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, riscv_dt_ids);

static struct driver riscv_driver = {
	.name = "riscv",
	.probe = riscv_probe,
	.of_compatible = riscv_dt_ids,
};
postcore_platform_driver(riscv_driver);

static void arch_shutdown(void)
{
	sync_caches_for_execution();
}
archshutdown_exitcall(arch_shutdown);
