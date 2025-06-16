// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 */

#define pr_fmt(fmt) "rockchip-dmc: " fmt

#include <common.h>
#include <init.h>
#include <asm/barebox-arm.h>
#include <asm/memory.h>
#include <pbl.h>
#include <io.h>
#include <linux/regmap.h>
#include <mfd/syscon.h>
#include <mach/rockchip/dmc.h>
#include <mach/rockchip/rk3399-regs.h>
#include <mach/rockchip/rk3568-regs.h>

#define RK3399_PMUGRF_OS_REG2		0x308
#define RK3399_PMUGRF_OS_REG3		0x30C

#define RK3568_PMUGRF_OS_REG2           0x208
#define RK3568_PMUGRF_OS_REG3           0x20c

#define RK3399_INT_REG_START		0xf0000000
#define RK3568_INT_REG_START		RK3399_INT_REG_START
#define RK3588_INT_REG_START		RK3399_INT_REG_START

/* RK3588 has two known memory gaps when using 16+ GiB DRAM */
#define DRAM_GAP1_START         0x3fc000000
#define DRAM_GAP1_END           0x3fc500000
#define DRAM_GAP2_START         0x3fff00000
#define DRAM_GAP2_END           0x400000000

struct rockchip_dmc_drvdata {
	unsigned int os_reg2;
	unsigned int os_reg3;
	unsigned int os_reg4;
	unsigned int os_reg5;
	resource_size_t internal_registers_start;
};

static resource_size_t rockchip_sdram_size(u32 sys_reg2, u32 sys_reg3)
{
	u32 rank, cs0_col, bk, cs0_row, cs1_row, bw, row_3_4;
	resource_size_t chipsize_mb, size_mb = 0;
	u32 ch;
	u32 cs1_col;
	u32 bg = 0;
	u32 dbw, dram_type;
	u32 ch_num = 1 + FIELD_GET(SYS_REG_NUM_CH, sys_reg2);
	u32 version = FIELD_GET(SYS_REG_VERSION, sys_reg3);

	pr_debug("%s(reg2=%x, reg3=%x)\n", __func__, sys_reg2, sys_reg3);

	dram_type = FIELD_GET(SYS_REG_DDRTYPE, sys_reg2);

	if (version >= 3)
		dram_type |= FIELD_GET(SYS_REG_EXTEND_DDRTYPE, sys_reg3) << 3;

	for (ch = 0; ch < ch_num; ch++) {
		rank = 1 + (sys_reg2 >> SYS_REG_RANK_SHIFT(ch) & SYS_REG_RANK_MASK);
		cs0_col = 9 + (sys_reg2 >> SYS_REG_COL_SHIFT(ch) & SYS_REG_COL_MASK);
		cs1_col = cs0_col;

		if (dram_type == LPDDR5)
			/* LPDDR5: 0:8bank(bk=3), 1:16bank(bk=4) */
			bk = 3 + ((sys_reg2 >> SYS_REG_BK_SHIFT(ch)) & SYS_REG_BK_MASK);
		else
			/* Other: 0:8bank(bk=3), 1:4bank(bk=2) */
			bk = 3 - ((sys_reg2 >> SYS_REG_BK_SHIFT(ch)) & SYS_REG_BK_MASK);

		cs0_row = sys_reg2 >> SYS_REG_CS0_ROW_SHIFT(ch) & SYS_REG_CS0_ROW_MASK;
		cs1_row = sys_reg2 >> SYS_REG_CS1_ROW_SHIFT(ch) & SYS_REG_CS1_ROW_MASK;

		if (version >= 2) {
			cs1_col = 9 + (sys_reg3 >> SYS_REG_CS1_COL_SHIFT(ch) &
				  SYS_REG_CS1_COL_MASK);

			cs0_row |= (sys_reg3 >> SYS_REG_EXTEND_CS0_ROW_SHIFT(ch) &
					SYS_REG_EXTEND_CS0_ROW_MASK) << 2;

			if (cs0_row == 7)
				cs0_row = 12;
			else
				cs0_row += 13;

			cs1_row |= (sys_reg3 >> SYS_REG_EXTEND_CS1_ROW_SHIFT(ch) &
					SYS_REG_EXTEND_CS1_ROW_MASK) << 2;

			if (cs1_row == 7)
				cs1_row = 12;
			else
				cs1_row += 13;
		} else {
			cs0_row += 13;
			cs1_row += 13;
		}

		bw = (2 >> ((sys_reg2 >> SYS_REG_BW_SHIFT(ch)) & SYS_REG_BW_MASK));
		row_3_4 = sys_reg2 >> SYS_REG_ROW_3_4_SHIFT(ch) & SYS_REG_ROW_3_4_MASK;

		if (dram_type == DDR4) {
			dbw = (sys_reg2 >> SYS_REG_DBW_SHIFT(ch)) & SYS_REG_DBW_MASK;
			bg = (dbw == 2) ? 2 : 1;
		}

		chipsize_mb = (1 << (cs0_row + cs0_col + bk + bg + bw - 20));

		if (rank > 1)
			chipsize_mb += chipsize_mb >> ((cs0_row - cs1_row) +
				       (cs0_col - cs1_col));
		if (row_3_4)
			chipsize_mb = chipsize_mb * 3 / 4;

		size_mb += chipsize_mb;

		if (rank > 1)
			pr_debug("rank %d cs0_col %d cs1_col %d bk %d cs0_row %d "
				 "cs1_row %d bw %d row_3_4 %d\n",
				 rank, cs0_col, cs1_col, bk, cs0_row,
				 cs1_row, bw, row_3_4);
		else
			pr_debug("rank %d cs0_col %d bk %d cs0_row %d "
				 "bw %d row_3_4 %d\n",
				 rank, cs0_col, bk, cs0_row,
				 bw, row_3_4);
	}

	return (resource_size_t)size_mb << 20;
}

resource_size_t rk3399_ram0_size(void)
{
	void __iomem *pmugrf = IOMEM(RK3399_PMUGRF_BASE);
	u32 sys_reg2, sys_reg3;
	resource_size_t size;

	sys_reg2 = readl(pmugrf + RK3399_PMUGRF_OS_REG2);
	sys_reg3 = readl(pmugrf + RK3399_PMUGRF_OS_REG3);

	size = rockchip_sdram_size(sys_reg2, sys_reg3);
	size = min_t(resource_size_t, RK3399_INT_REG_START, size);

	pr_debug("%s() = %llu\n", __func__, (u64)size);

	return size;
}

resource_size_t rk3568_ram0_size(void)
{
	void __iomem *pmugrf = IOMEM(RK3568_PMUGRF_BASE);
	u32 sys_reg2, sys_reg3;
	resource_size_t size;

	sys_reg2 = readl(pmugrf + RK3568_PMUGRF_OS_REG2);
	sys_reg3 = readl(pmugrf + RK3568_PMUGRF_OS_REG3);

	size = rockchip_sdram_size(sys_reg2, sys_reg3);
	size = min_t(resource_size_t, RK3568_INT_REG_START, size);

	pr_debug("%s() = %llu\n", __func__, (u64)size);

	return size;
}

#define RK3588_PMUGRF_BASE 0xfd58a000
#define RK3588_PMUGRF_OS_REG2           0x208
#define RK3588_PMUGRF_OS_REG3           0x20c
#define RK3588_PMUGRF_OS_REG4           0x210
#define RK3588_PMUGRF_OS_REG5           0x214

size_t rk3588_ram_sizes(phys_addr_t *base, resource_size_t *size, size_t n)
{
	void __iomem *pmugrf = IOMEM(RK3588_PMUGRF_BASE);
	u32 sys_reg2, sys_reg3, sys_reg4, sys_reg5;
	resource_size_t memsize, size1, size2;
	size_t i = 0;

	sys_reg2 = readl(pmugrf + RK3588_PMUGRF_OS_REG2);
	sys_reg3 = readl(pmugrf + RK3588_PMUGRF_OS_REG3);
	sys_reg4 = readl(pmugrf + RK3588_PMUGRF_OS_REG4);
	sys_reg5 = readl(pmugrf + RK3588_PMUGRF_OS_REG5);

	size1 = rockchip_sdram_size(sys_reg2, sys_reg3);
	size2 = rockchip_sdram_size(sys_reg4, sys_reg5);

	pr_info("%s() size1 = 0x%08llx, size2 = 0x%08llx\n", __func__, (u64)size1, (u64)size2);

	memsize = size1 + size2;

	base[i] = 0xa00000;
	size[i] = min_t(resource_size_t, RK3588_INT_REG_START, memsize) - 0xa00000;
	i++;

	if (i < n && memsize > SZ_4G) {
		base[i] = SZ_4G;
		size[i] = min_t(unsigned long, DRAM_GAP1_START, memsize) - SZ_4G;
		i++;
	}
	if (i < n && memsize > DRAM_GAP1_END) {
		base[i] = DRAM_GAP1_END;
		size[i] = min_t(unsigned long, DRAM_GAP2_START, memsize) - DRAM_GAP1_END;
		i++;
	}
	if (i < n && memsize > DRAM_GAP2_END) {
		base[i] = DRAM_GAP2_END;
		size[i] = memsize - DRAM_GAP2_END;
		i++;
	}

	return i;
}

resource_size_t rk3588_ram0_size(void)
{
	phys_addr_t base;
	resource_size_t size;

	rk3588_ram_sizes(&base, &size, 1);

	return size;
}

static int rockchip_dmc_probe(struct device *dev)
{
	const struct rockchip_dmc_drvdata *drvdata;
	resource_size_t membase, memsize;
	struct regmap *regmap;
	u32 sys_rega, sys_regb;

	regmap = syscon_regmap_lookup_by_phandle(dev->of_node, "rockchip,pmu");
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	drvdata = device_get_match_data(dev);
	if (!drvdata)
		return -ENOENT;

	regmap_read(regmap, drvdata->os_reg2, &sys_rega);
	regmap_read(regmap, drvdata->os_reg3, &sys_regb);
	memsize = rockchip_sdram_size(sys_rega, sys_regb);

	if (drvdata->os_reg4) {
		regmap_read(regmap, drvdata->os_reg4, &sys_rega);
		regmap_read(regmap, drvdata->os_reg5, &sys_regb);
		memsize += rockchip_sdram_size(sys_rega, sys_regb);
	}

	dev_info(dev, "Detected memory size: 0x%08llx\n", (u64)memsize);

	/* lowest 10M are shaved off for secure world firmware */
	membase = 0xa00000;

	/* ram0, from 0xa00000 up to SoC internal register space start */
	arm_add_mem_device("ram0", membase,
		min_t(resource_size_t, drvdata->internal_registers_start, memsize) - membase);

	/* ram1, RAM beyond 32bit space up to first gap */
	if (memsize > SZ_4G)
		arm_add_mem_device("ram1", SZ_4G,
			min_t(resource_size_t, DRAM_GAP1_START, memsize) - SZ_4G);

	/* ram2, RAM between first and second gap */
	if (memsize > DRAM_GAP1_END)
		arm_add_mem_device("ram2", DRAM_GAP1_END,
			min_t(resource_size_t, DRAM_GAP2_START, memsize) - DRAM_GAP1_END);

	/* ram3, remaining RAM after second gap */
	if (memsize > DRAM_GAP2_END)
		arm_add_mem_device("ram3", DRAM_GAP2_END, memsize - DRAM_GAP2_END);

	return 0;
}

static const struct rockchip_dmc_drvdata rk3399_drvdata = {
	.os_reg2 = RK3399_PMUGRF_OS_REG2,
	.os_reg3 = RK3399_PMUGRF_OS_REG3,
	.internal_registers_start = RK3399_INT_REG_START,
};

static const struct rockchip_dmc_drvdata rk3568_drvdata = {
	.os_reg2 = RK3568_PMUGRF_OS_REG2,
	.os_reg3 = RK3568_PMUGRF_OS_REG3,
	.internal_registers_start = RK3568_INT_REG_START,
};

static const struct rockchip_dmc_drvdata rk3588_drvdata = {
	.os_reg2 = RK3588_PMUGRF_OS_REG2,
	.os_reg3 = RK3588_PMUGRF_OS_REG3,
	.os_reg4 = RK3588_PMUGRF_OS_REG4,
	.os_reg5 = RK3588_PMUGRF_OS_REG5,
	.internal_registers_start = RK3588_INT_REG_START,
};

static struct of_device_id rockchip_dmc_dt_ids[] = {
	{
		.compatible = "rockchip,rk3399-dmc",
		.data = &rk3399_drvdata,
	},
	{
		.compatible = "rockchip,rk3568-dmc",
		.data = &rk3568_drvdata,
	},
	{
		.compatible = "rockchip,rk3588-dmc",
		.data = &rk3588_drvdata,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, rockchip_dmc_dt_ids);

static struct driver rockchip_dmc_driver = {
	.name   = "rockchip-dmc",
	.probe  = rockchip_dmc_probe,
	.of_compatible = rockchip_dmc_dt_ids,
};
mem_platform_driver(rockchip_dmc_driver);
