// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <mach/at91/hardware.h>
#include <asm/barebox-arm.h>
#include <mach/at91/at91sam9_sdramc.h>
#include <asm/memory.h>
#include <pbl.h>
#include <io.h>

void __noreturn at91sam9260_barebox_entry(void *boarddata)
{
	barebox_arm_entry(AT91_CHIPSELECT_1,
			  at91_get_sdram_size(IOMEM(AT91SAM9260_BASE_SDRAMC)),
			  boarddata);
}

static int at91_sdramc_probe(struct device *dev)
{
	struct resource *iores;
	void __iomem *base;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	return arm_add_mem_device("ram0", AT91_CHIPSELECT_1,
				  at91_get_sdram_size(base));
}

static struct of_device_id at91_sdramc_dt_ids[] = {
	{ .compatible = "atmel,at91sam9260-sdramc" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, at91_sdramc_dt_ids);

static struct driver at91_sdramc_driver = {
	.name   = "at91sam9260-sdramc",
	.probe  = at91_sdramc_probe,
	.of_compatible = at91_sdramc_dt_ids,
};
mem_platform_driver(at91_sdramc_driver);
