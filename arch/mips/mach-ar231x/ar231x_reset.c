// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <restart.h>
#include <linux/err.h>

#include <mach/ar2312_regs.h>
#include <mach/ar231x_platform.h>

static void __iomem *reset_base;

static void __noreturn ar2312x_restart_soc(struct restart_handler *rst)
{
	printf("reseting cpu\n");
	__raw_writel(0x10000,
		(char *)KSEG1ADDR(AR2312_WD_TIMER));
	__raw_writel(AR2312_WD_CTRL_RESET,
		(char *)KSEG1ADDR(AR2312_WD_CTRL));

	hang();
}

static u32 ar231x_reset_readl(void)
{
	return __raw_readl(reset_base);
}

static void ar231x_reset_writel(u32 val)
{
	__raw_writel(val, reset_base);
}

void ar231x_reset_bit(u32 val, enum reset_state state)
{
	u32 tmp;

	tmp = ar231x_reset_readl();

	if (state == REMOVE)
		ar231x_reset_writel(tmp & ~val);
	else
		ar231x_reset_writel(tmp | val);
}
EXPORT_SYMBOL(ar231x_reset_bit);

static int ar231x_reset_probe(struct device_d *dev)
{
	struct resource *iores;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	reset_base = IOMEM(iores->start);

	return 0;
}

static struct driver_d ar231x_reset_driver = {
	.probe	= ar231x_reset_probe,
	.name	= "ar231x_reset",
};

static int ar231x_reset_init(void)
{
	restart_handler_register_fn(ar2312x_restart_soc);
	return platform_driver_register(&ar231x_reset_driver);
}
coredevice_initcall(ar231x_reset_init);
