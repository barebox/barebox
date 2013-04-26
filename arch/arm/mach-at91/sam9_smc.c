/*
 * linux/arch/arm/mach-at91/sam9_smc.c
 *
 * Copyright (C) 2008 Andrew Victor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/hardware.h>
#include <mach/io.h>

#include <mach/at91sam9_smc.h>

#define AT91_SMC_CS(id, n)	(smc_base_addr[id] + ((n) * 0x10))

static void __iomem *smc_base_addr[2];

static void sam9_smc_cs_write_mode(void __iomem *base,
					struct sam9_smc_config *config)
{
	__raw_writel(config->mode
		   | AT91_SMC_TDF_(config->tdf_cycles),
		   base + AT91_SMC_MODE);
}

void sam9_smc_write_mode(int id, int cs,
					struct sam9_smc_config *config)
{
	sam9_smc_cs_write_mode(AT91_SMC_CS(id, cs), config);
}

static void sam9_smc_cs_configure(void __iomem *base,
					struct sam9_smc_config *config)
{

	/* Setup register */
	__raw_writel(AT91_SMC_NWESETUP_(config->nwe_setup)
		   | AT91_SMC_NCS_WRSETUP_(config->ncs_write_setup)
		   | AT91_SMC_NRDSETUP_(config->nrd_setup)
		   | AT91_SMC_NCS_RDSETUP_(config->ncs_read_setup),
		   base + AT91_SMC_SETUP);

	/* Pulse register */
	__raw_writel(AT91_SMC_NWEPULSE_(config->nwe_pulse)
		   | AT91_SMC_NCS_WRPULSE_(config->ncs_write_pulse)
		   | AT91_SMC_NRDPULSE_(config->nrd_pulse)
		   | AT91_SMC_NCS_RDPULSE_(config->ncs_read_pulse),
		   base + AT91_SMC_PULSE);

	/* Cycle register */
	__raw_writel(AT91_SMC_NWECYCLE_(config->write_cycle)
		   | AT91_SMC_NRDCYCLE_(config->read_cycle),
		   base + AT91_SMC_CYCLE);

	/* Mode register */
	sam9_smc_cs_write_mode(base, config);
}

void sam9_smc_configure(int id, int cs,
					struct sam9_smc_config *config)
{
	sam9_smc_cs_configure(AT91_SMC_CS(id, cs), config);
}

static void sam9_smc_cs_read_mode(void __iomem *base,
					struct sam9_smc_config *config)
{
	u32 val = __raw_readl(base + AT91_SMC_MODE);

	config->mode = (val & ~AT91_SMC_NWECYCLE);
	config->tdf_cycles = (val & AT91_SMC_NWECYCLE) >> 16 ;
}

void sam9_smc_read_mode(int id, int cs,
					struct sam9_smc_config *config)
{
	sam9_smc_cs_read_mode(AT91_SMC_CS(id, cs), config);
}

static void sam9_smc_cs_read(void __iomem *base,
					struct sam9_smc_config *config)
{
	u32 val;

	/* Setup register */
	val = __raw_readl(base + AT91_SMC_SETUP);

	config->nwe_setup = val & AT91_SMC_NWESETUP;
	config->ncs_write_setup = (val & AT91_SMC_NCS_WRSETUP) >> 8;
	config->nrd_setup = (val & AT91_SMC_NRDSETUP) >> 16;
	config->ncs_read_setup = (val & AT91_SMC_NCS_RDSETUP) >> 24;

	/* Pulse register */
	val = __raw_readl(base + AT91_SMC_PULSE);

	config->nwe_setup = val & AT91_SMC_NWEPULSE;
	config->ncs_write_pulse = (val & AT91_SMC_NCS_WRPULSE) >> 8;
	config->nrd_pulse = (val & AT91_SMC_NRDPULSE) >> 16;
	config->ncs_read_pulse = (val & AT91_SMC_NCS_RDPULSE) >> 24;

	/* Cycle register */
	val = __raw_readl(base + AT91_SMC_CYCLE);

	config->write_cycle = val & AT91_SMC_NWECYCLE;
	config->read_cycle = (val & AT91_SMC_NRDCYCLE) >> 16;

	/* Mode register */
	sam9_smc_cs_read_mode(base, config);
}

void sam9_smc_read(int id, int cs, struct sam9_smc_config *config)
{
	sam9_smc_cs_read(AT91_SMC_CS(id, cs), config);
}

static int at91sam9_smc_probe(struct device_d *dev)
{
	int id = dev->id;

	if (id < 0) {
		id = 0;
	} else if (id > 1) {
		dev_warn(dev, "id > 1\n");
		return -EIO;
	}

	smc_base_addr[id] = dev_request_mem_region(dev, 0);
	if (!smc_base_addr[id]) {
		dev_err(dev, "Impossible to request smc.%d\n", id);
		return -ENOMEM;
	}

	return 0;
}

static struct driver_d at91sam9_smc_driver = {
	.name = "at91sam9-smc",
	.probe = at91sam9_smc_probe,
};

static int at91sam9_smc_init(void)
{
	return platform_driver_register(&at91sam9_smc_driver);
}
coredevice_initcall(at91sam9_smc_init);
