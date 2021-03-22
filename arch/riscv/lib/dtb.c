// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
#include <common.h>
#include <init.h>
#include <of.h>
#include <asm/barebox-riscv.h>
#include <asm/timer.h>

static int of_riscv_init(void)
{
	void *fdt;

	/* See if we are provided a dtb in boarddata */
	fdt = barebox_riscv_boot_dtb();
	if (!fdt) {
		pr_err("No DTB found\n");
		return -ENODATA;
	}

	pr_debug("using boarddata provided DTB\n");


	barebox_register_fdt(fdt);

	/* do it now, before clocksource drivers run postcore */
	timer_init();

	return 0;
}
core_initcall(of_riscv_init);
