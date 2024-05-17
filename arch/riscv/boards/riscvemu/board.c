// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <envfs.h>
#include <poweroff.h>
#include <restart.h>
#include <deep-probe.h>
#include <asm/system.h>
#include <asm/barebox-riscv.h>

struct riscvemu_priv {
	struct restart_handler rst;
	void __noreturn (*restart)(unsigned long, void *);

};

static void __noreturn riscvemu_restart(struct restart_handler *rst)
{
	struct riscvemu_priv *priv = container_of(rst, struct riscvemu_priv, rst);

	/* clear screen on graphic console */
	puts("\e[J");

	/*
	 * barebox PBL relocates itself to end of RAM early on, so unless
	 * something explicitly scrubbed the initial PBL, we can jump back to
	 * the reset vector to "reset".
	 */
	priv->restart(riscv_hartid(), barebox_riscv_boot_dtb());
}

extern char __dtbo_riscvemu_sram_start[];

static int riscvemu_probe(struct device *dev)
{
	struct device_node *of_chosen;
	struct riscvemu_priv *priv;
	u64 start;

	of_overlay_apply_dtbo(dev->of_node, __dtbo_riscvemu_sram_start);
	/* of_probe() will happen later at of_populate_initcall */

	if (IS_ENABLED(CONFIG_CMD_TUTORIAL))
		defaultenv_append_directory(defaultenv_riscvemu);

	of_chosen = of_find_node_by_path("/chosen");

	if (of_property_read_u64(of_chosen, "riscv,kernel-start", &start))
		return 0;

	priv = xzalloc(sizeof(*priv));

	priv->restart = (void *)(uintptr_t)start;
	priv->rst.restart = riscvemu_restart;
	priv->rst.name = "vector";

	return restart_handler_register(&priv->rst);
}

static const struct of_device_id riscvemu_of_match[] = {
	{ .compatible = "ucbbar,riscvemu-bar_dev" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(riscvemu_of_match);

static struct driver riscvemu_board_driver = {
	.name = "board-riscvemu",
	.probe = riscvemu_probe,
	.of_compatible = riscvemu_of_match,
};
device_platform_driver(riscvemu_board_driver);
