// SPDX-License-Identifier: GPL-2.0-only
#include <memory.h>
#include <linux/bitfield.h>
#include <mach/k3/common.h>
#include <linux/bits.h>
#include <io.h>
#include <asm/memory.h>
#include <linux/kernel.h>

#define CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_0 0x0

#define DRAM_CLASS	GENMASK(11, 8)

#define CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_327 0x51c

#define CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_317 0x4f4

#define ROW_DIFF_0	GENMASK(2, 0)
#define ROW_DIFF_1	GENMASK(10, 8)
#define COL_DIFF_0	GENMASK(19, 16)
#define COL_DIFF_1	GENMASK(27, 24)

#define CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_3 0xc

#define MAX_ROW		GENMASK(4, 0)
#define MAX_COL		GENMASK(11, 8)

#define AM625_DDRSS_BASE	0x0f308000

#define DENALI_CTL_0_DRAM_CLASS_DDR4		0xa
#define DENALI_CTL_0_DRAM_CLASS_LPDDR4		0xb

u64 am625_sdram_size(void)
{
	void __iomem *base = IOMEM(AM625_DDRSS_BASE);
	u32 ctl0 = readl(base + CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_0);
	u32 ctl3 = readl(base + CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_3);
	u32 ctl317 = readl(base + CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_317);
	u32 ctl327 = readl(base + CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_327);
	unsigned int cols, rows, banks;
	u64 size = 0;

	if (FIELD_GET(DRAM_CLASS, ctl0) == DENALI_CTL_0_DRAM_CLASS_LPDDR4)
		banks = 8;
	else if (FIELD_GET(DRAM_CLASS, ctl0) == DENALI_CTL_0_DRAM_CLASS_DDR4)
		banks = 16;
	else
		return 0;

	if (ctl327 & BIT(0)) {
		cols = FIELD_GET(MAX_COL, ctl3) - FIELD_GET(COL_DIFF_0, ctl317);
		rows = FIELD_GET(MAX_ROW, ctl3) - FIELD_GET(ROW_DIFF_0, ctl317);
		size += memory_sdram_size(cols, rows, banks, 2);
	}

	if (ctl327 & BIT(1)) {
		cols = FIELD_GET(MAX_COL, ctl3) - FIELD_GET(COL_DIFF_1, ctl317);
		rows = FIELD_GET(MAX_ROW, ctl3) - FIELD_GET(ROW_DIFF_1, ctl317);
		size += memory_sdram_size(cols, rows, banks, 2);
	}

	return size;
}

void am625_register_dram(void)
{
	u64 size = am625_sdram_size();
	u64 lowmem = min_t(u64, size, SZ_2G);

	arm_add_mem_device("ram0", 0x80000000, lowmem);

#ifdef CONFIG_64BIT
	if (size - lowmem)
		arm_add_mem_device("ram0", 0x880000000ULL, size - lowmem);
#endif
}
