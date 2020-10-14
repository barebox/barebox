// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <mach/ddramc.h>
#include <mach/hardware.h>
#include <asm/barebox-arm.h>
#include <mach/at91_ddrsdrc.h>
#include <mach/sama5_bootsource.h>
#include <asm/memory.h>
#include <pbl.h>
#include <io.h>

static unsigned sama5_ramsize(void __iomem *base)
{
	return at91_get_ddram_size(base, true);
}

void __noreturn sama5d2_barebox_entry(unsigned int r4, void *boarddata)
{
	__sama5d2_stashed_bootrom_r4 = r4;
	barebox_arm_entry(SAMA5_DDRCS, sama5_ramsize(SAMA5D2_BASE_MPDDRC),
			  boarddata);
}

static int sama5_ddr_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *base;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	arm_add_mem_device("ram0", SAMA5_DDRCS, sama5_ramsize(base));

	return 0;
}

static struct of_device_id sama5_ddr_dt_ids[] = {
	{ .compatible = "atmel,sama5d3-ddramc" },
	{ /* sentinel */ }
};

static struct driver_d sama5_ddr_driver = {
	.name   = "sama5-ddramc",
	.probe  = sama5_ddr_probe,
	.of_compatible = sama5_ddr_dt_ids,
};

mem_platform_driver(sama5_ddr_driver);
