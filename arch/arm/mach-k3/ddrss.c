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

#define CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_316 0x4f0
#define BANK_DIFF_1	GENMASK(25, 24)
#define BANK_DIFF_0	GENMASK(17, 16)

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

static unsigned int am625_get_banks_count(unsigned int regval)
{
	/*
	 * The BANK_DIFF_x are only described in the Reference Manual as:
	 *
	 * "Encoded number of banks on the DRAM[s]"
	 *
	 * From putting different configurations into the TI DDR configuration
	 * tool it seems that a register value of 0 means 16 banks and 1 means
	 * 8 banks.
	 */
	switch (regval) {
	case 0:
		return 16;
	case 1:
		return 8;
	default:
		return 0;
	}
}

u64 am625_sdram_size(void)
{
	void __iomem *base = IOMEM(AM625_DDRSS_BASE);
	u32 ctl3 = readl(base + CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_3);
	u32 ctl316 = readl(base + CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_316);
	u32 ctl317 = readl(base + CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_317);
	u32 ctl327 = readl(base + CTLPHY_CTL_CFG_CTLCFG_DENALI_CTL_327);
	unsigned int cols, rows, banks;
	u64 size = 0;

	if (ctl327 & BIT(0)) {
		cols = FIELD_GET(MAX_COL, ctl3) - FIELD_GET(COL_DIFF_0, ctl317);
		rows = FIELD_GET(MAX_ROW, ctl3) - FIELD_GET(ROW_DIFF_0, ctl317);
		banks = am625_get_banks_count(FIELD_GET(BANK_DIFF_0, ctl316));

		size += memory_sdram_size(cols, rows, banks, 2);
	}

	if (ctl327 & BIT(1)) {
		cols = FIELD_GET(MAX_COL, ctl3) - FIELD_GET(COL_DIFF_1, ctl317);
		rows = FIELD_GET(MAX_ROW, ctl3) - FIELD_GET(ROW_DIFF_1, ctl317);
		banks = am625_get_banks_count(FIELD_GET(BANK_DIFF_1, ctl316));

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
