/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * @brief Device driver for the Tegra 20 power management controller.
 */

#include <common.h>
#include <init.h>
#include <io.h>

#include <mach/tegra20-pmc.h>

static void __iomem *pmc_base;

/* main SoC reset trigger */
void __noreturn reset_cpu(ulong addr)
{
	writel(PMC_CNTRL_MAIN_RST, pmc_base + PMC_CNTRL);

	unreachable();
}
EXPORT_SYMBOL(reset_cpu);

static int tegra20_pmc_probe(struct device_d *dev)
{
	pmc_base = dev_request_mem_region(dev, 0);
	if (!pmc_base) {
		dev_err(dev, "could not get memory region\n");
		return -ENODEV;
	}

	return 0;
}

static __maybe_unused struct of_device_id tegra20_pmc_dt_ids[] = {
	{
		.compatible = "nvidia,tegra20-pmc",
	}, {
		/* sentinel */
	}
};

static struct driver_d tegra20_pmc_driver = {
	.probe	= tegra20_pmc_probe,
	.name	= "tegra20-pmc",
	.of_compatible = DRV_OF_COMPAT(tegra20_pmc_dt_ids),
};

static int tegra20_pmc_init(void)
{
	return platform_driver_register(&tegra20_pmc_driver);
}
coredevice_initcall(tegra20_pmc_init);
