// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2008 Andrew Victor

/* linux/arch/arm/mach-at91/sam9_smc.c */

#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/at91/hardware.h>
#include <mach/at91/cpu.h>
#include <linux/err.h>

#include <mach/at91/at91sam9_smc.h>

#define AT91_SAM9_SMC_CS_STRIDE		0x10
#define AT91_SAMA5_SMC_CS_STRIDE	0x14
#define AT91_SMC_CS_STRIDE	((at91_soc_initdata.type == AT91_SOC_SAMA5D3 \
				 || at91_soc_initdata.type == AT91_SOC_SAMA5D4) \
				 ? AT91_SAMA5_SMC_CS_STRIDE : AT91_SAM9_SMC_CS_STRIDE)
#define AT91_SMC_CS(id, n)	(smc_base_addr[id] + ((n) * AT91_SMC_CS_STRIDE))

static void __iomem *smc_base_addr[2];

static void sam9_smc_cs_write_mode(void __iomem *base,
					struct sam9_smc_config *config)
{
	void __iomem *mode_reg;

	switch (at91_soc_initdata.type) {
	case AT91_SOC_SAMA5D3:
	case AT91_SOC_SAMA5D4:
		mode_reg = base + AT91_SAMA5_SMC_MODE;
		break;
	default:
		mode_reg = base + AT91_SAM9_SMC_MODE;
		break;
	}

	writel(config->mode
	       | AT91_SMC_TDF_(config->tdf_cycles),
	       mode_reg);
}

static void sam9_smc_cs_write_timings(void __iomem *base,
					struct sam9_smc_config *config)
{
	writel(AT91_SMC_TCLR_(config->tclr)
	       | AT91_SMC_TADL_(config->tadl)
	       | AT91_SMC_TAR_(config->tar)
	       | AT91_SMC_OCMS_(config->ocms)
	       | AT91_SMC_TRR_(config->trr)
	       | AT91_SMC_TWB_(config->twb)
	       | AT91_SMC_RBNSEL_(config->rbnsel)
	       | AT91_SMC_NFSEL_(config->nfsel),
	       base + AT91_SAMA5_SMC_TIMINGS);
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
	writel(AT91_SMC_NWESETUP_(config->nwe_setup)
	       | AT91_SMC_NCS_WRSETUP_(config->ncs_write_setup)
	       | AT91_SMC_NRDSETUP_(config->nrd_setup)
	       | AT91_SMC_NCS_RDSETUP_(config->ncs_read_setup),
	       base + AT91_SMC_SETUP);

	/* Pulse register */
	writel(AT91_SMC_NWEPULSE_(config->nwe_pulse)
	       | AT91_SMC_NCS_WRPULSE_(config->ncs_write_pulse)
	       | AT91_SMC_NRDPULSE_(config->nrd_pulse)
	       | AT91_SMC_NCS_RDPULSE_(config->ncs_read_pulse),
	       base + AT91_SMC_PULSE);

	/* Cycle register */
	writel(AT91_SMC_NWECYCLE_(config->write_cycle)
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
	u32 val;
	void __iomem *mode_reg;

	switch (at91_soc_initdata.type) {
	case AT91_SOC_SAMA5D2:
	case AT91_SOC_SAMA5D3:
	case AT91_SOC_SAMA5D4:
		mode_reg = base + AT91_SAMA5_SMC_MODE;
		break;
	default:
		mode_reg = base + AT91_SAM9_SMC_MODE;
		break;
	}

	val = readl(mode_reg);

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
	val = readl(base + AT91_SMC_SETUP);

	config->nwe_setup = val & AT91_SMC_NWESETUP;
	config->ncs_write_setup = (val & AT91_SMC_NCS_WRSETUP) >> 8;
	config->nrd_setup = (val & AT91_SMC_NRDSETUP) >> 16;
	config->ncs_read_setup = (val & AT91_SMC_NCS_RDSETUP) >> 24;

	/* Pulse register */
	val = readl(base + AT91_SMC_PULSE);

	config->nwe_setup = val & AT91_SMC_NWEPULSE;
	config->ncs_write_pulse = (val & AT91_SMC_NCS_WRPULSE) >> 8;
	config->nrd_pulse = (val & AT91_SMC_NRDPULSE) >> 16;
	config->ncs_read_pulse = (val & AT91_SMC_NCS_RDPULSE) >> 24;

	/* Cycle register */
	val = readl(base + AT91_SMC_CYCLE);

	config->write_cycle = val & AT91_SMC_NWECYCLE;
	config->read_cycle = (val & AT91_SMC_NRDCYCLE) >> 16;

	/* Mode register */
	sam9_smc_cs_read_mode(base, config);
}

void sam9_smc_read(int id, int cs, struct sam9_smc_config *config)
{
	sam9_smc_cs_read(AT91_SMC_CS(id, cs), config);
}

void sama5_smc_configure(int id, int cs, struct sam9_smc_config *config)
{
        sam9_smc_configure(id, cs, config);

        sam9_smc_cs_write_timings(AT91_SMC_CS(id, cs), config);
}

static int at91sam9_smc_probe(struct device *dev)
{
	struct resource *iores;
	int id = dev->id;

	if (id < 0) {
		id = 0;
	} else if (id > 1) {
		dev_warn(dev, "id > 1\n");
		return -EIO;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "Impossible to request smc.%d\n", id);
		return PTR_ERR(iores);
	}
	smc_base_addr[id] = IOMEM(iores->start);

	return 0;
}

static struct driver at91sam9_smc_driver = {
	.name = "at91sam9-smc",
	.probe = at91sam9_smc_probe,
};

postcore_platform_driver(at91sam9_smc_driver);
