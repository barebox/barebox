// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <mach/stm32.h>
#include <mach/ddr_regs.h>
#include <mach/entry.h>
#include <mach/stm32.h>
#include <asm/barebox-arm.h>
#include <asm/memory.h>
#include <pbl.h>
#include <io.h>

#define ADDRMAP1_BANK_B0	GENMASK( 5,  0)
#define ADDRMAP1_BANK_B1	GENMASK(13,  8)
#define ADDRMAP1_BANK_B2	GENMASK(21, 16)

#define ADDRMAP2_COL_B2		GENMASK( 3,  0)
#define ADDRMAP2_COL_B3		GENMASK(11,  8)
#define ADDRMAP2_COL_B4		GENMASK(19, 16)
#define ADDRMAP2_COL_B5		GENMASK(27, 24)

#define ADDRMAP3_COL_B6		GENMASK( 3,  0)
#define ADDRMAP3_COL_B7		GENMASK(12,  8)
#define ADDRMAP3_COL_B8		GENMASK(20, 16)
#define ADDRMAP3_COL_B9		GENMASK(28, 24)

#define ADDRMAP4_COL_B10	GENMASK( 4,  0)
#define ADDRMAP4_COL_B11	GENMASK(12,  8)

#define ADDRMAP5_ROW_B0		GENMASK( 3,  0)
#define ADDRMAP5_ROW_B1		GENMASK(11,  8)
#define ADDRMAP5_ROW_B2_10	GENMASK(19, 16)
#define ADDRMAP5_ROW_B11	GENMASK(27, 24)

#define ADDRMAP6_ROW_B12	GENMASK( 3,  0)
#define ADDRMAP6_ROW_B13	GENMASK(11,  8)
#define ADDRMAP6_ROW_B14	GENMASK(19, 16)
#define ADDRMAP6_ROW_B15	GENMASK(27, 24)

#define ADDRMAP9_ROW_B2		GENMASK( 3,  0)
#define ADDRMAP9_ROW_B3		GENMASK(11,  8)
#define ADDRMAP9_ROW_B4		GENMASK(19, 16)
#define ADDRMAP9_ROW_B5		GENMASK(27, 24)

#define ADDRMAP10_ROW_B6	GENMASK( 3,  0)
#define ADDRMAP10_ROW_B7	GENMASK(11,  8)
#define ADDRMAP10_ROW_B8	GENMASK(19, 16)
#define ADDRMAP10_ROW_B9	GENMASK(27, 24)

#define ADDRMAP11_ROW_B10	GENMASK( 3, 0)

#define LINE_UNUSED(reg, mask) (((reg) & (mask)) == (mask))

enum ddrctrl_buswidth {
	BUSWIDTH_FULL = 0,
	BUSWIDTH_HALF = 1,
	BUSWIDTH_QUARTER = 2
};

static unsigned long ddrctrl_addrmap_ramsize(struct stm32mp1_ddrctl __iomem *d,
					     enum ddrctrl_buswidth buswidth)
{
	unsigned banks = 3, cols = 12, rows = 16;
	u32 reg;

	cols += buswidth;

	reg = readl(&d->addrmap1);
	if (LINE_UNUSED(reg, ADDRMAP1_BANK_B2)) banks--;
	if (LINE_UNUSED(reg, ADDRMAP1_BANK_B1)) banks--;
	if (LINE_UNUSED(reg, ADDRMAP1_BANK_B0)) banks--;

	reg = readl(&d->addrmap2);
	if (LINE_UNUSED(reg, ADDRMAP2_COL_B5)) cols--;
	if (LINE_UNUSED(reg, ADDRMAP2_COL_B4)) cols--;
	if (LINE_UNUSED(reg, ADDRMAP2_COL_B3)) cols--;
	if (LINE_UNUSED(reg, ADDRMAP2_COL_B2)) cols--;

	reg = readl(&d->addrmap3);
	if (LINE_UNUSED(reg, ADDRMAP3_COL_B9)) cols--;
	if (LINE_UNUSED(reg, ADDRMAP3_COL_B8)) cols--;
	if (LINE_UNUSED(reg, ADDRMAP3_COL_B7)) cols--;
	if (LINE_UNUSED(reg, ADDRMAP3_COL_B6)) cols--;

	reg = readl(&d->addrmap4);
	if (LINE_UNUSED(reg, ADDRMAP4_COL_B11)) cols--;
	if (LINE_UNUSED(reg, ADDRMAP4_COL_B10)) cols--;

	reg = readl(&d->addrmap5);
	if (LINE_UNUSED(reg, ADDRMAP5_ROW_B11)) rows--;

	reg = readl(&d->addrmap6);
	if (LINE_UNUSED(reg, ADDRMAP6_ROW_B15)) rows--;
	if (LINE_UNUSED(reg, ADDRMAP6_ROW_B14)) rows--;
	if (LINE_UNUSED(reg, ADDRMAP6_ROW_B13)) rows--;
	if (LINE_UNUSED(reg, ADDRMAP6_ROW_B12)) rows--;

	return memory_sdram_size(cols, rows, BIT(banks), 4 / BIT(buswidth));
}

static inline unsigned ddrctrl_ramsize(void __iomem *base)
{
	struct stm32mp1_ddrctl __iomem *ddrctl = base;
	unsigned buswidth = readl(&ddrctl->mstr) & DDRCTRL_MSTR_DATA_BUS_WIDTH_MASK;
	buswidth >>= DDRCTRL_MSTR_DATA_BUS_WIDTH_SHIFT;

	return ddrctrl_addrmap_ramsize(ddrctl, buswidth);
}

static inline unsigned stm32mp1_ddrctrl_ramsize(void)
{
	return ddrctrl_ramsize(IOMEM(STM32_DDRCTL_BASE));
}

void __noreturn stm32mp1_barebox_entry(void *boarddata)
{
	barebox_arm_entry(STM32_DDR_BASE, stm32mp1_ddrctrl_ramsize(), boarddata);
}


static int stm32mp1_ddr_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *base;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	arm_add_mem_device("ram0", STM32_DDR_BASE, ddrctrl_ramsize(base));

	return 0;
}

static __maybe_unused struct of_device_id stm32mp1_ddr_dt_ids[] = {
	{ .compatible = "st,stm32mp1-ddr" },
	{ /* sentinel */ }
};

static struct driver_d stm32mp1_ddr_driver = {
	.name   = "stm32mp1-ddr",
	.probe  = stm32mp1_ddr_probe,
	.of_compatible = DRV_OF_COMPAT(stm32mp1_ddr_dt_ids),
};

mem_platform_driver(stm32mp1_ddr_driver);
