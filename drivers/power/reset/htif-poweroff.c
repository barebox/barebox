// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <poweroff.h>
#include <asm/htif.h>

static void __iomem *htif = IOMEM(HTIF_DEFAULT_BASE_ADDR);

static void __noreturn riscvemu_poweroff(struct poweroff_handler *pwr)
{
	shutdown_barebox();

	__htif_tohost(htif, HTIF_DEV_SYSCALL, 0, 1);

	__builtin_unreachable();
}

static int htif_poweroff_probe(struct device *dev)
{
	struct resource *iores;

	iores = dev_request_mem_resource(dev, 0);
	if (!IS_ERR(iores))
		htif = IOMEM(iores->start);
	else if (PTR_ERR(iores) != -ENOENT)
		return PTR_ERR(iores);

	return poweroff_handler_register_fn(riscvemu_poweroff);
}


static const struct of_device_id htif_poweroff_of_match[] = {
	{ .compatible = "ucb,htif0" },
	{}
};
MODULE_DEVICE_TABLE(of, htif_poweroff_of_match);

static struct driver htif_poweroff_driver = {
	.name = "htif-poweroff",
	.of_compatible = htif_poweroff_of_match,
	.probe = htif_poweroff_probe,
};
coredevice_platform_driver(htif_poweroff_driver);
