// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#include <init.h>
#include <common.h>
#include <poweroff.h>

static void __noreturn kvx_poweroff(struct poweroff_handler *handler)
{
	register int status asm("r0") = 0;

	shutdown_barebox();

	asm volatile ("scall 0xfff\n\t;;"
			: : "r"(status)
			: "r1", "r2", "r3", "r4", "r5", "r6", "r7",
							"r8", "memory");
	hang();
}

static int kvx_scall_poweroff_probe(struct device *dev)
{
	poweroff_handler_register_fn(kvx_poweroff);

	return 0;
}

static __maybe_unused struct of_device_id kvx_scall_poweroff_id[] = {
	{
		.compatible = "kalray,kvx-scall-poweroff",
	}, {
	}
};
MODULE_DEVICE_TABLE(of, kvx_scall_poweroff_id);

static struct driver kvx_scall_poweroff = {
	.name  = "kvx_scall_poweroff",
	.probe = kvx_scall_poweroff_probe,
	.of_compatible = DRV_OF_COMPAT(kvx_scall_poweroff_id),
};

device_platform_driver(kvx_scall_poweroff);
