/*
 * Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <io.h>

#include <mach/ar2312_regs.h>
#include <mach/ar231x_platform.h>

static void __iomem *reset_base;

void __noreturn reset_cpu(ulong addr)
{
	printf("reseting cpu\n");
	__raw_writel(0x10000,
		(char *)KSEG1ADDR(AR2312_WD_TIMER));
	__raw_writel(AR2312_WD_CTRL_RESET,
		(char *)KSEG1ADDR(AR2312_WD_CTRL));
	unreachable();
}
EXPORT_SYMBOL(reset_cpu);

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
	reset_base = dev_request_mem_region(dev, 0);
	if (!reset_base) {
		dev_err(dev, "could not get memory region\n");
		return -ENODEV;
	}

	return 0;
}

static struct driver_d ar231x_reset_driver = {
	.probe	= ar231x_reset_probe,
	.name	= "ar231x_reset",
};

static int ar231x_reset_init(void)
{
	return platform_driver_register(&ar231x_reset_driver);
}
coredevice_initcall(ar231x_reset_init);
