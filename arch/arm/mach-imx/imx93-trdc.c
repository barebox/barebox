// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 */
#define pr_fmt(fmt) "trdc: " fmt

#include <common.h>
#include <io.h>
#include <mach/imx/ele.h>
#include <mach/imx/trdc.h>
#include <mach/imx/imx9-regs.h>

#define DID_NUM 16
#define MBC_MAX_NUM 4
#define MRC_MAX_NUM 2
#define MBC_NUM(HWCFG) ((HWCFG >> 16) & 0xF)
#define MRC_NUM(HWCFG) ((HWCFG >> 24) & 0x1F)

struct mbc_mem_dom {
	u32 mem_glbcfg[4];
	u32 nse_blk_index;
	u32 nse_blk_set;
	u32 nse_blk_clr;
	u32 nsr_blk_clr_all;
	u32 memn_glbac[8];
	/* The upper only existed in the beginning of each MBC */
	u32 mem0_blk_cfg_w[64];
	u32 mem0_blk_nse_w[16];
	u32 mem1_blk_cfg_w[8];
	u32 mem1_blk_nse_w[2];
	u32 mem2_blk_cfg_w[8];
	u32 mem2_blk_nse_w[2];
	u32 mem3_blk_cfg_w[8];
	u32 mem3_blk_nse_w[2];/*0x1F0, 0x1F4 */
	u32 reserved[2];
};

struct mrc_rgn_dom {
	u32 mrc_glbcfg[4];
	u32 nse_rgn_indirect;
	u32 nse_rgn_set;
	u32 nse_rgn_clr;
	u32 nse_rgn_clr_all;
	u32 memn_glbac[8];
	/* The upper only existed in the beginning of each MRC */
	u32 rgn_desc_words[16][2]; /* 16  regions at max, 2 words per region */
	u32 rgn_nse;
	u32 reserved2[15];
};

struct mda_inst {
	u32 mda_w[8];
};

struct trdc_mgr {
	u32 trdc_cr;
	u32 res0[59];
	u32 trdc_hwcfg0;
	u32 trdc_hwcfg1;
	u32 res1[450];
	struct mda_inst mda[8];
	u32 res2[15808];
};

struct trdc_mbc {
	struct mbc_mem_dom mem_dom[DID_NUM];
};

struct trdc_mrc {
	struct mrc_rgn_dom mrc_dom[DID_NUM];
};

static void *trdc_get_mbc_base(ulong trdc_reg, u32 mbc_x)
{
	struct trdc_mgr *trdc_base = (struct trdc_mgr *)trdc_reg;
	u32 mbc_num = MBC_NUM(trdc_base->trdc_hwcfg0);

	if (mbc_x >= mbc_num)
		return 0;

	return (void *)trdc_reg + 0x10000 + 0x2000 * mbc_x;
}

static void *trdc_get_mrc_base(ulong trdc_reg, u32 mrc_x)
{
	struct trdc_mgr *trdc_base = (struct trdc_mgr *)trdc_reg;
	u32 mbc_num = MBC_NUM(trdc_base->trdc_hwcfg0);
	u32 mrc_num = MRC_NUM(trdc_base->trdc_hwcfg0);

	if (mrc_x >= mrc_num)
		return 0;

	return (void *)trdc_reg + 0x10000 + 0x2000 * mbc_num + 0x1000 * mrc_x;
}

static int trdc_mbc_set_control(ulong trdc_reg, u32 mbc_x, u32 glbac_id,
			 u32 glbac_val)
{
	struct trdc_mbc *mbc_base = trdc_get_mbc_base(trdc_reg, mbc_x);
	struct mbc_mem_dom *mbc_dom;

	if (mbc_base == 0 || glbac_id >= 8)
		return -EINVAL;

	/* only first dom has the glbac */
	mbc_dom = &mbc_base->mem_dom[0];

	writel(glbac_val, &mbc_dom->memn_glbac[glbac_id]);

	return 0;
}

static int trdc_mbc_blk_config(ulong trdc_reg, u32 mbc_x, u32 dom_x, u32 mem_x,
			u32 blk_x, bool sec_access, u32 glbac_id)
{
	struct trdc_mbc *mbc_base = trdc_get_mbc_base(trdc_reg, mbc_x);
	struct mbc_mem_dom *mbc_dom;
	u32 *cfg_w, *nse_w;
	u32 index, offset, val;

	if (mbc_base == 0 || glbac_id >= 8)
		return -EINVAL;

	mbc_dom = &mbc_base->mem_dom[dom_x];

	switch (mem_x) {
	case 0:
		cfg_w = &mbc_dom->mem0_blk_cfg_w[blk_x / 8];
		nse_w = &mbc_dom->mem0_blk_nse_w[blk_x / 32];
		break;
	case 1:
		cfg_w = &mbc_dom->mem1_blk_cfg_w[blk_x / 8];
		nse_w = &mbc_dom->mem1_blk_nse_w[blk_x / 32];
		break;
	case 2:
		cfg_w = &mbc_dom->mem2_blk_cfg_w[blk_x / 8];
		nse_w = &mbc_dom->mem2_blk_nse_w[blk_x / 32];
		break;
	case 3:
		cfg_w = &mbc_dom->mem3_blk_cfg_w[blk_x / 8];
		nse_w = &mbc_dom->mem3_blk_nse_w[blk_x / 32];
		break;
	default:
		return -EINVAL;
	};

	index = blk_x % 8;
	offset = index * 4;

	val = readl((void __iomem *)cfg_w);

	val &= ~(0xFU << offset);

	/* MBC0-3
	 *  Global 0, 0x7777 secure pri/user read/write/execute, S400 has already set it.
	 *  So select MBC0_MEMN_GLBAC0
	 */
	if (sec_access) {
		val |= ((0x0 | (glbac_id & 0x7)) << offset);
		writel(val, (void __iomem *)cfg_w);
	} else {
		val |= ((0x8 | (glbac_id & 0x7)) << offset); /* nse bit set */
		writel(val, (void __iomem *)cfg_w);
	}

	return 0;
}

static int trdc_mrc_set_control(ulong trdc_reg, u32 mrc_x, u32 glbac_id, u32 glbac_val)
{
	struct trdc_mrc *mrc_base = trdc_get_mrc_base(trdc_reg, mrc_x);
	struct mrc_rgn_dom *mrc_dom;

	if (mrc_base == 0 || glbac_id >= 8)
		return -EINVAL;

	/* only first dom has the glbac */
	mrc_dom = &mrc_base->mrc_dom[0];

	pr_vdebug("mrc_dom 0x%lx\n", (ulong)mrc_dom);

	writel(glbac_val, &mrc_dom->memn_glbac[glbac_id]);

	return 0;
}

static int trdc_mrc_region_config(ulong trdc_reg, u32 mrc_x, u32 dom_x, u32 addr_start,
			   u32 addr_end, bool sec_access, u32 glbac_id)
{
	struct trdc_mrc *mrc_base = trdc_get_mrc_base(trdc_reg, mrc_x);
	struct mrc_rgn_dom *mrc_dom;
	u32 *desc_w;
	u32 start, end;
	u32 i, free = 8;
	bool vld, hit = false;

	if (mrc_base == 0 || glbac_id >= 8)
		return -EINVAL;

	mrc_dom = &mrc_base->mrc_dom[dom_x];

	addr_start &= ~0x3fff;
	addr_end &= ~0x3fff;

	for (i = 0; i < 8; i++) {
		desc_w = &mrc_dom->rgn_desc_words[i][0];

		start = readl((void __iomem *)desc_w) & (~0x3fff);
		end = readl((void __iomem *)(desc_w + 1));
		vld = end & 0x1;
		end = end & (~0x3fff);

		if (start == 0 && end == 0 && !vld && free >= 8)
			free = i;

		/* Check all the region descriptors, even overlap */
		if (addr_start >= end || addr_end <= start || !vld)
			continue;

		/* MRC0,1
		 *  Global 0, 0x7777 secure pri/user read/write/execute, S400 has already set it.
		 *  So select MRCx_MEMN_GLBAC0
		 */
		if (sec_access) {
			writel(start | (glbac_id & 0x7), (void __iomem *)desc_w);
			writel(end | 0x1, (void __iomem *)(desc_w + 1));
		} else {
			writel(start | (glbac_id & 0x7), (void __iomem *)desc_w);
			writel(end | 0x1 | 0x10, (void __iomem *)(desc_w + 1));
		}

		if (addr_start >= start && addr_end <= end)
			hit = true;
	}

	if (!hit) {
		if (free >= 8)
			return -EFAULT;

		desc_w = &mrc_dom->rgn_desc_words[free][0];

		if (sec_access) {
			writel(addr_start | (glbac_id & 0x7), (void __iomem *)desc_w);
			writel(addr_end | 0x1, (void __iomem *)(desc_w + 1));
		} else {
			writel(addr_start | (glbac_id & 0x7), (void __iomem *)desc_w);
			writel((addr_end | 0x1 | 0x10), (void __iomem *)(desc_w + 1));
		}
	}

	return 0;
}

static bool trdc_mrc_enabled(ulong trdc_base)
{
	return (!!(readl((void __iomem *)trdc_base) & 0x8000));
}

#define CORE_ID_A55	0x2

void imx9_trdc_init(void)
{
	unsigned long base = MX9_TRDC_NICMIX_BASE_ADDR;
	int ret = 0, i;

	ret |= ele_release_rdc(CORE_ID_A55, 0, NULL);
	ret |= ele_release_rdc(CORE_ID_A55, 2, NULL);
	ret |= ele_release_rdc(CORE_ID_A55, 1, NULL);
	ret |= ele_release_rdc(CORE_ID_A55, 3, NULL);

	if (ret)
		return;

	/* Set OCRAM to RWX for secure, when OEM_CLOSE, the image is RX only */
	trdc_mbc_set_control(base, 3, 0, 0x7700);

	for (i = 0; i < 40; i++)
		trdc_mbc_blk_config(base, 3, 3, 0, i, true, 0);

	for (i = 0; i < 40; i++)
		trdc_mbc_blk_config(base, 3, 3, 1, i, true, 0);

	for (i = 0; i < 40; i++)
		trdc_mbc_blk_config(base, 3, 0, 0, i, true, 0);

	for (i = 0; i < 40; i++)
		trdc_mbc_blk_config(base, 3, 0, 1, i, true, 0);

	/* TRDC mega */
	if (!trdc_mrc_enabled(base))
		return;

	/* DDR */
	trdc_mrc_set_control(base, 0, 0, 0x7777);
	/* S400*/
	trdc_mrc_region_config(base, 0, 0, 0x80000000, 0xFFFFFFFF, false, 0);
	/* MTR */
	trdc_mrc_region_config(base, 0, 1, 0x80000000, 0xFFFFFFFF, false, 0);
	/* M33 */
	trdc_mrc_region_config(base, 0, 2, 0x80000000, 0xFFFFFFFF, false, 0);
	/* A55*/
	trdc_mrc_region_config(base, 0, 3, 0x80000000, 0xFFFFFFFF, false, 0);
	/* For USDHC1 to DDR, USDHC1 is default force to non-secure */
	trdc_mrc_region_config(base, 0, 5, 0x80000000, 0xFFFFFFFF, false, 0);
	/* For USDHC2 to DDR, USDHC2 is default force to non-secure */
	trdc_mrc_region_config(base, 0, 6, 0x80000000, 0xFFFFFFFF, false, 0);
	/* eDMA */
	trdc_mrc_region_config(base, 0, 7, 0x80000000, 0xFFFFFFFF, false, 0);
	/* CoreSight, TestPort */
	trdc_mrc_region_config(base, 0, 8, 0x80000000, 0xFFFFFFFF, false, 0);
	/* DAP */
	trdc_mrc_region_config(base, 0, 9, 0x80000000, 0xFFFFFFFF, false, 0);
	/* SoC masters */
	trdc_mrc_region_config(base, 0, 10, 0x80000000, 0xFFFFFFFF, false, 0);
	/* USB */
	trdc_mrc_region_config(base, 0, 11, 0x80000000, 0xFFFFFFFF, false, 0);
}
