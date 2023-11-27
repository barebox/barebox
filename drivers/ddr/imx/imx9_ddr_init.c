// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 */

#define pr_fmt(fmt) "imx9-ddr: " fmt

#include <common.h>
#include <errno.h>
#include <io.h>
#include <soc/imx9/ddr.h>
#include <mach/imx/generic.h>
#include <linux/iopoll.h>
#include <soc/imx/clk-fracn-gppll.h>
#include <mach/imx/imx9-regs.h>

#define MX9_SRC_DPHY_BASE_ADDR	(MX9_SRC_BASE_ADDR + 0x1400)
#define REG_DDR_SDRAM_MD_CNTL	(MX9_DDR_CTL_BASE + 0x120)
#define REG_DDR_CS0_BNDS        (MX9_DDR_CTL_BASE + 0x0)
#define REG_DDR_CS1_BNDS        (MX9_DDR_CTL_BASE + 0x8)
#define REG_DDRDSR_2		(MX9_DDR_CTL_BASE + 0xB24)
#define REG_DDR_TIMING_CFG_0	(MX9_DDR_CTL_BASE + 0x104)
#define REG_DDR_SDRAM_CFG	(MX9_DDR_CTL_BASE + 0x110)
#define REG_DDR_TIMING_CFG_4	(MX9_DDR_CTL_BASE + 0x160)
#define REG_DDR_DEBUG_19	(MX9_DDR_CTL_BASE + 0xF48)
#define REG_DDR_SDRAM_CFG_3	(MX9_DDR_CTL_BASE + 0x260)
#define REG_DDR_SDRAM_CFG_4	(MX9_DDR_CTL_BASE + 0x264)
#define REG_DDR_SDRAM_MD_CNTL_2	(MX9_DDR_CTL_BASE + 0x270)
#define REG_DDR_SDRAM_MPR4	(MX9_DDR_CTL_BASE + 0x28C)
#define REG_DDR_SDRAM_MPR5	(MX9_DDR_CTL_BASE + 0x290)

#define REG_DDR_ERR_EN		(MX9_DDR_CTL_BASE + 0x1000)
#define REG_SRC_DPHY_SW_CTRL			(MX9_SRC_DPHY_BASE_ADDR + 0x20)
#define REG_SRC_DPHY_SINGLE_RESET_SW_CTRL	(MX9_SRC_DPHY_BASE_ADDR + 0x24)

#define IMX9_SAVED_DRAM_TIMING_BASE 0x2051C000

static unsigned int g_cdd_rr_max[4];
static unsigned int g_cdd_rw_max[4];
static unsigned int g_cdd_wr_max[4];
static unsigned int g_cdd_ww_max[4];

static void ddrphy_coldreset(void)
{
	/* dramphy_apb_n default 1 , assert -> 0, de_assert -> 1 */
	/* dramphy_reset_n default 0 , assert -> 0, de_assert -> 1 */
	/* dramphy_PwrOKIn default 0 , assert -> 1, de_assert -> 0 */

	/* src_gen_dphy_apb_sw_rst_de_assert */
	clrbits_le32(REG_SRC_DPHY_SW_CTRL, BIT(0));
	/* src_gen_dphy_sw_rst_de_assert */
	clrbits_le32(REG_SRC_DPHY_SINGLE_RESET_SW_CTRL, BIT(2));
	/* src_gen_dphy_PwrOKIn_sw_rst_de_assert() */
	setbits_le32(REG_SRC_DPHY_SINGLE_RESET_SW_CTRL, BIT(0));
	mdelay(10);

	/* src_gen_dphy_apb_sw_rst_assert */
	setbits_le32(REG_SRC_DPHY_SW_CTRL, BIT(0));
	/* src_gen_dphy_sw_rst_assert */
	setbits_le32(REG_SRC_DPHY_SINGLE_RESET_SW_CTRL, BIT(2));
	mdelay(10);
	/* src_gen_dphy_PwrOKIn_sw_rst_assert */
	clrbits_le32(REG_SRC_DPHY_SINGLE_RESET_SW_CTRL, BIT(0));
	mdelay(10);

	/* src_gen_dphy_apb_sw_rst_de_assert */
	clrbits_le32(REG_SRC_DPHY_SW_CTRL, BIT(0));
	/* src_gen_dphy_sw_rst_de_assert() */
	clrbits_le32(REG_SRC_DPHY_SINGLE_RESET_SW_CTRL, BIT(2));
}

static void check_ddrc_idle(void)
{
	u32 regval;

	readl_poll_timeout(REG_DDRDSR_2, regval, regval & BIT(31), 0);
}

static void check_dfi_init_complete(void)
{
	u32 regval;

	readl_poll_timeout(REG_DDRDSR_2, regval, regval & BIT(2), 0);

	setbits_le32(REG_DDRDSR_2, BIT(2));
}

static void ddrc_config(struct dram_timing_info *dram_timing)
{
	u32 num = dram_timing->ddrc_cfg_num;
	struct dram_cfg_param *ddrc_config;
	int i = 0;

	ddrc_config = dram_timing->ddrc_cfg;
	for (i = 0; i < num; i++) {
		writel(ddrc_config->val, (ulong)ddrc_config->reg);
		ddrc_config++;
	}

	if (dram_timing->fsp_cfg) {
		ddrc_config = dram_timing->fsp_cfg[0].ddrc_cfg;
		while (ddrc_config->reg != 0) {
			writel(ddrc_config->val, (ulong)ddrc_config->reg);
			ddrc_config++;
		}
	}
}

static unsigned int look_for_max(unsigned int data[], unsigned int addr_start,
				 unsigned int addr_end)
{
	unsigned int i, imax = 0;

	for (i = addr_start; i <= addr_end; i++) {
		if (((data[i] >> 7) == 0) && data[i] > imax)
			imax = data[i];
	}

	return imax;
}

static void get_trained_CDD(struct dram_controller *dram, u32 fsp)
{
	unsigned int i, tmp;
	unsigned int cdd_cha[12], cdd_chb[12];
	unsigned int cdd_cha_rr_max, cdd_cha_rw_max, cdd_cha_wr_max, cdd_cha_ww_max;
	unsigned int cdd_chb_rr_max, cdd_chb_rw_max, cdd_chb_wr_max, cdd_chb_ww_max;

	for (i = 0; i < 6; i++) {
		tmp = dwc_ddrphy_apb_rd(dram, 0x54013 + i);
		cdd_cha[i * 2] = tmp & 0xff;
		cdd_cha[i * 2 + 1] = (tmp >> 8) & 0xff;
	}

	for (i = 0; i < 7; i++) {
		tmp = dwc_ddrphy_apb_rd(dram, 0x5402c + i);

		if (i == 0) {
			cdd_chb[0] = (tmp >> 8) & 0xff;
		} else if (i == 6) {
			cdd_chb[11] = tmp & 0xff;
		} else {
			cdd_chb[i * 2 - 1] = tmp & 0xff;
			cdd_chb[i * 2] = (tmp >> 8) & 0xff;
		}
	}

	cdd_cha_rr_max = look_for_max(cdd_cha, 0, 1);
	cdd_cha_rw_max = look_for_max(cdd_cha, 2, 5);
	cdd_cha_wr_max = look_for_max(cdd_cha, 6, 9);
	cdd_cha_ww_max = look_for_max(cdd_cha, 10, 11);
	cdd_chb_rr_max = look_for_max(cdd_chb, 0, 1);
	cdd_chb_rw_max = look_for_max(cdd_chb, 2, 5);
	cdd_chb_wr_max = look_for_max(cdd_chb, 6, 9);
	cdd_chb_ww_max = look_for_max(cdd_chb, 10, 11);
	g_cdd_rr_max[fsp] =  cdd_cha_rr_max > cdd_chb_rr_max ? cdd_cha_rr_max : cdd_chb_rr_max;
	g_cdd_rw_max[fsp] =  cdd_cha_rw_max > cdd_chb_rw_max ? cdd_cha_rw_max : cdd_chb_rw_max;
	g_cdd_wr_max[fsp] =  cdd_cha_wr_max > cdd_chb_wr_max ? cdd_cha_wr_max : cdd_chb_wr_max;
	g_cdd_ww_max[fsp] =  cdd_cha_ww_max > cdd_chb_ww_max ? cdd_cha_ww_max : cdd_chb_ww_max;
}

static u32 ddrc_get_fsp_reg_setting(struct dram_cfg_param *ddrc_cfg, unsigned int cfg_num, u32 reg)
{
	unsigned int i;

	for (i = 0; i < cfg_num; i++) {
		if (reg == ddrc_cfg[i].reg)
			return ddrc_cfg[i].val;
	}

	return 0;
}

static void ddrc_update_fsp_reg_setting(struct dram_cfg_param *ddrc_cfg, int cfg_num,
					u32 reg, u32 val)
{
	unsigned int i;

	for (i = 0; i < cfg_num; i++) {
		if (reg == ddrc_cfg[i].reg) {
			ddrc_cfg[i].val = val;
			return;
		}
	}
}

static void update_umctl2_rank_space_setting(struct dram_timing_info *dram_timing,
					     unsigned int pstat_num)
{
	u32 tmp, tmp_t;
	u32 wwt, rrt, wrt, rwt;
	u32 ext_wwt, ext_rrt, ext_wrt, ext_rwt;
	u32 max_wwt, max_rrt, max_wrt, max_rwt;
	u32 i;

	for (i = 0; i < pstat_num; i++) {
		/* read wwt, rrt, wrt, rwt fields from timing_cfg_0 */
		if (!dram_timing->fsp_cfg_num) {
			tmp = ddrc_get_fsp_reg_setting(dram_timing->ddrc_cfg,
						       dram_timing->ddrc_cfg_num,
						       REG_DDR_TIMING_CFG_0);
		} else {
			tmp = ddrc_get_fsp_reg_setting(dram_timing->fsp_cfg[i].ddrc_cfg,
						       ARRAY_SIZE(dram_timing->fsp_cfg[i].ddrc_cfg),
						       REG_DDR_TIMING_CFG_0);
		}
		wwt = (tmp >> 24) & 0x3;
		rrt = (tmp >> 26) & 0x3;
		wrt = (tmp >> 28) & 0x3;
		rwt = (tmp >> 30) & 0x3;

		/* read rxt_wwt, ext_rrt, ext_wrt, ext_rwt fields from timing_cfg_4 */
		if (!dram_timing->fsp_cfg_num) {
			tmp_t = ddrc_get_fsp_reg_setting(dram_timing->ddrc_cfg,
							 dram_timing->ddrc_cfg_num,
							 REG_DDR_TIMING_CFG_4);
		} else {
			tmp_t = ddrc_get_fsp_reg_setting(dram_timing->fsp_cfg[i].ddrc_cfg,
							 ARRAY_SIZE(dram_timing->fsp_cfg[i].ddrc_cfg),
							 REG_DDR_TIMING_CFG_4);
		}
		ext_wwt = (tmp_t >> 8)  & 0x3;
		ext_rrt = (tmp_t >> 10) & 0x3;
		ext_wrt = (tmp_t >> 12) & 0x3;
		ext_rwt = (tmp_t >> 14) & 0x3;

		wwt = (ext_wwt << 2) | wwt;
		rrt = (ext_rrt << 2) | rrt;
		wrt = (ext_wrt << 2) | wrt;
		rwt = (ext_rwt << 2) | rwt;

		max_wwt = max(g_cdd_ww_max[0], wwt);
		max_rrt = max(g_cdd_rr_max[0], rrt);
		max_wrt = max(g_cdd_wr_max[0], wrt);
		max_rwt = max(g_cdd_rw_max[0], rwt);
		/* verify values to see if are bigger then 15 (4 bits) */
		if (max_wwt > 15)
			max_wwt = 15;
		if (max_rrt > 15)
			max_rrt = 15;
		if (max_wrt > 15)
			max_wrt = 15;
		if (max_rwt > 15)
			max_rwt = 15;

		/* recalculate timings for controller registers */
		wwt = max_wwt & 0x3;
		rrt = max_rrt & 0x3;
		wrt = max_wrt & 0x3;
		rwt = max_rwt & 0x3;

		ext_wwt = (max_wwt & 0xC) >> 2;
		ext_rrt = (max_rrt & 0xC) >> 2;
		ext_wrt = (max_wrt & 0xC) >> 2;
		ext_rwt = (max_rwt & 0xC) >> 2;

		/* update timing_cfg_0 and timing_cfg_4 */
		tmp = (tmp & 0x00ffffff) | (rwt << 30) | (wrt << 28) |
			(rrt << 26) | (wwt << 24);
		tmp_t = (tmp_t & 0xFFFF00FF) | (ext_rwt << 14) |
			(ext_wrt << 12) | (ext_rrt << 10) | (ext_wwt << 8);

		if (!dram_timing->fsp_cfg_num) {
			ddrc_update_fsp_reg_setting(dram_timing->ddrc_cfg,
						    dram_timing->ddrc_cfg_num,
						    REG_DDR_TIMING_CFG_0, tmp);
			ddrc_update_fsp_reg_setting(dram_timing->ddrc_cfg,
						    dram_timing->ddrc_cfg_num,
						    REG_DDR_TIMING_CFG_4, tmp_t);
		} else {
			ddrc_update_fsp_reg_setting(dram_timing->fsp_cfg[i].ddrc_cfg,
						    ARRAY_SIZE(dram_timing->fsp_cfg[i].ddrc_cfg),
						    REG_DDR_TIMING_CFG_0, tmp);
			ddrc_update_fsp_reg_setting(dram_timing->fsp_cfg[i].ddrc_cfg,
						    ARRAY_SIZE(dram_timing->fsp_cfg[i].ddrc_cfg),
						    REG_DDR_TIMING_CFG_4, tmp_t);
		}
	}
}

static u32 ddrc_mrr(u32 chip_select, u32 mode_reg_num, u32 *mode_reg_val)
{
	u32 temp;

	writel(0x80000000, REG_DDR_SDRAM_MD_CNTL_2);
	temp = 0x80000000 | (chip_select << 28) | (mode_reg_num << 0);
	writel(temp, REG_DDR_SDRAM_MD_CNTL);
	while ((readl(REG_DDR_SDRAM_MD_CNTL) & 0x80000000) == 0x80000000)
		;
	while (!(readl(REG_DDR_SDRAM_MPR5)))
		;
	*mode_reg_val = (readl(REG_DDR_SDRAM_MPR4) & 0xFF0000) >> 16;
	writel(0x0, REG_DDR_SDRAM_MPR5);
	while ((readl(REG_DDR_SDRAM_MPR5)))
		;
	writel(0x0, REG_DDR_SDRAM_MPR4);
	writel(0x0, REG_DDR_SDRAM_MD_CNTL_2);

	return 0;
}

static void ddrc_mrs(u32 cs_sel, u32 opcode, u32 mr)
{
	u32 regval;

	regval = (cs_sel << 28) | (opcode << 6) | (mr);
	writel(regval, REG_DDR_SDRAM_MD_CNTL);
	setbits_le32(REG_DDR_SDRAM_MD_CNTL, BIT(31));
	check_ddrc_idle();
}

static u32 lpddr4_mr_read(u32 mr_rank, u32 mr_addr)
{
	u32 chip_select, regval;

	if (mr_rank == 1)
		chip_select = 0; /* CS0 */
	else if (mr_rank == 2)
		chip_select = 1; /* CS1 */
	else
		chip_select = 4; /* CS0 & CS1 */

	ddrc_mrr(chip_select, mr_addr, &regval);

	return regval;
}

static void update_mr_fsp_op0(struct dram_cfg_param *cfg, unsigned int num)
{
	int i;

	ddrc_mrs(0x4, 0x88, 13); /* FSP-OP->1, FSP-WR->0, VRCG=1, DMD=0 */
	for (i = 0; i < num; i++) {
		if (cfg[i].reg)
			ddrc_mrs(0x4, cfg[i].val, cfg[i].reg);
	}
	ddrc_mrs(0x4, 0xc0, 13); /* FSP-OP->1, FSP-WR->1, VRCG=0, DMD=0 */
}

static void save_trained_mr12_14(struct dram_cfg_param *cfg, u32 cfg_num, u32 mr12, u32 mr14)
{
	int i;

	for (i = 0; i < cfg_num; i++) {
		if (cfg->reg == 12)
			cfg->val = mr12;
		else if (cfg->reg == 14)
			cfg->val = mr14;
		cfg++;
	}
}

#define MHZ(x)        ((x) * 1000000UL)

#define SHARED_GPR_DRAM_CLK 2
#define SHARED_GPR_DRAM_CLK_SEL_PLL 0
#define SHARED_GPR_DRAM_CLK_SEL_CCM BIT(0)

static struct imx_fracn_gppll_rate_table imx9_fracpll_tbl[] = {
	{ .rate = 1000000000U, .rdiv = 1, .mfi = 166, .odiv = 4, .mfn = 2, .mfd = 3 }, /* 1000MHz */
	{ .rate = 933000000U, .rdiv = 1, .mfi = 155, .odiv = 4, .mfn = 1, .mfd = 2 }, /* 933MHz */
	{ .rate = 700000000U, .rdiv = 1, .mfi = 145, .odiv = 5, .mfn = 5, .mfd = 6 }, /* 700MHz */
	{ .rate = 484000000U, .rdiv = 1, .mfi = 121, .odiv = 6, .mfn = 0, .mfd = 1 }, /* 480MHz */
	{ .rate = 445333333U, .rdiv = 1, .mfi = 167, .odiv = 9, .mfn = 0, .mfd = 1 },
	{ .rate = 466000000U, .rdiv = 1, .mfi = 155, .odiv = 8, .mfn = 1, .mfd = 3 }, /* 466MHz */
	{ .rate = 400000000U, .rdiv = 1, .mfi = 200, .odiv = 12, .mfn = 0, .mfd = 1 }, /* 400MHz */
	{ .rate = 300000000U, .rdiv = 1, .mfi = 150, .odiv = 12, .mfn = 0, .mfd = 1 },
};

static int dram_pll_init(u32 freq)
{
	return fracn_gppll_set_rate(IOMEM(MX9_ANATOP_DRAM_PLL_BASE_ADDR),
				    CLK_FRACN_GPPLL_FRACN, imx9_fracpll_tbl,
				    ARRAY_SIZE(imx9_fracpll_tbl), freq);
}

static void ccm_shared_gpr_set(u32 gpr, u32 val)
{
	writel(val, IOMEM(MX9_CCM_BASE_ADDR + 0x4800));
}

#define DRAM_ALT_CLK_ROOT	76
#define DRAM_APB_CLK_ROOT	77

#define CLK_ROOT_MUX	GENMASK(9, 8)
#define CLK_ROOT_DIV	GENMASK(9, 0)

static void ccm_clk_root_cfg(u32 clk_root_id, int mux, u32 div)
{
	void __iomem *base = IOMEM(MX9_CCM_BASE_ADDR) + clk_root_id * 0x80;

	writel(FIELD_PREP(CLK_ROOT_MUX, mux) | FIELD_PREP(CLK_ROOT_DIV, div - 1), base);
};

static void dram_enable_bypass(ulong clk_val)
{
	switch (clk_val) {
	case MHZ(625):
		ccm_clk_root_cfg(DRAM_ALT_CLK_ROOT, 3, 1);
		break;
	case MHZ(400):
		ccm_clk_root_cfg(DRAM_ALT_CLK_ROOT, 2, 2);
		break;
	case MHZ(333):
		ccm_clk_root_cfg(DRAM_ALT_CLK_ROOT, 1, 3);
		break;
	case MHZ(200):
		ccm_clk_root_cfg(DRAM_ALT_CLK_ROOT, 2, 4);
		break;
	case MHZ(100):
		ccm_clk_root_cfg(DRAM_ALT_CLK_ROOT, 2, 8);
		break;
	default:
		printf("No matched freq table %lu\n", clk_val);
		return;
	}

	/* Set DRAM APB to 133Mhz */
	ccm_clk_root_cfg(DRAM_APB_CLK_ROOT, 2, 3);
	/* Switch from DRAM  clock root from PLL to CCM */
	ccm_shared_gpr_set(SHARED_GPR_DRAM_CLK, SHARED_GPR_DRAM_CLK_SEL_CCM);
}

static void dram_disable_bypass(void)
{
	/* Set DRAM APB to 133Mhz */
	ccm_clk_root_cfg(DRAM_APB_CLK_ROOT, 2, 3);
	/* Switch from DRAM  clock root from CCM to PLL */
	ccm_shared_gpr_set(SHARED_GPR_DRAM_CLK, SHARED_GPR_DRAM_CLK_SEL_PLL);
}

static void ddrphy_init_set_dfi_clk(struct dram_controller *dram, unsigned int drate_mhz)
{
	switch (drate_mhz) {
	case 4000:
		dram_pll_init(MHZ(1000));
		dram_disable_bypass();
		break;
	case 3733:
	case 3732:
		dram_pll_init(MHZ(933));
		dram_disable_bypass();
		break;
	case 3200:
		dram_pll_init(MHZ(800));
		dram_disable_bypass();
		break;
	case 3000:
		dram_pll_init(MHZ(750));
		dram_disable_bypass();
		break;
	case 2800:
		dram_pll_init(MHZ(700));
		dram_disable_bypass();
		break;
	case 2400:
		dram_pll_init(MHZ(600));
		dram_disable_bypass();
		break;
	case 1866:
		dram_pll_init(MHZ(466));
		dram_disable_bypass();
		break;
	case 1600:
		dram_pll_init(MHZ(400));
		dram_disable_bypass();
		break;
	case 1066:
		dram_pll_init(MHZ(266));
		dram_disable_bypass();
		break;
	case 667:
		dram_pll_init(MHZ(167));
		dram_disable_bypass();
		break;
	case 625:
		dram_enable_bypass(MHZ(625));
		break;
	case 400:
		dram_enable_bypass(MHZ(400));
		break;
	case 333:
		dram_enable_bypass(MHZ(333));
		break;
	case 200:
		dram_enable_bypass(MHZ(200));
		break;
	case 100:
		dram_enable_bypass(MHZ(100));
		break;
	default:
		return;
	}
}

static u32 ddrphy_addr_remap(u32 paddr_apb_from_ctlr)
{
	u32 paddr_apb_qual;
	u32 paddr_apb_unqual_dec_22_13;
	u32 paddr_apb_unqual_dec_19_13;
	u32 paddr_apb_unqual_dec_12_1;
	u32 paddr_apb_unqual;
	u32 paddr_apb_phy;

	paddr_apb_qual = (paddr_apb_from_ctlr << 1);
	paddr_apb_unqual_dec_22_13 = ((paddr_apb_qual & 0x7fe000) >> 13);
	paddr_apb_unqual_dec_12_1  = ((paddr_apb_qual & 0x1ffe) >> 1);

	switch (paddr_apb_unqual_dec_22_13) {
	case 0x000 ... 0x00b:
		paddr_apb_unqual_dec_19_13 = paddr_apb_unqual_dec_22_13;
		break;
	case 0x100 ... 0x10b:
		paddr_apb_unqual_dec_19_13 = paddr_apb_unqual_dec_22_13 - 0x100 + 0xc;
		break;
	case 0x200 ... 0x20b:
		paddr_apb_unqual_dec_19_13 = paddr_apb_unqual_dec_22_13 - 0x200 + 0x18;
		break;
	case 0x300 ... 0x30b:
		paddr_apb_unqual_dec_19_13 = paddr_apb_unqual_dec_22_13 - 0x300 + 0x24;
		break;
	case 0x010 ... 0x019:
		paddr_apb_unqual_dec_19_13 = paddr_apb_unqual_dec_22_13 - 0x10 + 0x30;
		break;
	case 0x110 ... 0x119:
		paddr_apb_unqual_dec_19_13 = paddr_apb_unqual_dec_22_13 - 0x110 + 0x3a;
		break;
	case 0x210 ... 0x219:
		paddr_apb_unqual_dec_19_13 = paddr_apb_unqual_dec_22_13 - 0x210 + 0x44;
		break;
	case 0x310 ... 0x319:
		paddr_apb_unqual_dec_19_13 = paddr_apb_unqual_dec_22_13 - 0x310 + 0x4e;
		break;
	case 0x020:
		paddr_apb_unqual_dec_19_13 = 0x58;
		break;
	case 0x120:
		paddr_apb_unqual_dec_19_13 = 0x59;
		break;
	case 0x220:
		paddr_apb_unqual_dec_19_13 = 0x5a;
		break;
	case 0x320:
		paddr_apb_unqual_dec_19_13 = 0x5b;
		break;
	case 0x040:
		paddr_apb_unqual_dec_19_13 = 0x5c;
		break;
	case 0x140:
		paddr_apb_unqual_dec_19_13 = 0x5d;
		break;
	case 0x240:
		paddr_apb_unqual_dec_19_13 = 0x5e;
		break;
	case 0x340:
		paddr_apb_unqual_dec_19_13 = 0x5f;
		break;
	case 0x050:
		paddr_apb_unqual_dec_19_13 = 0x60;
		break;
	case 0x051:
		paddr_apb_unqual_dec_19_13 = 0x61;
		break;
	case 0x052:
		paddr_apb_unqual_dec_19_13 = 0x62;
		break;
	case 0x053:
		paddr_apb_unqual_dec_19_13 = 0x63;
		break;
	case 0x054:
		paddr_apb_unqual_dec_19_13 = 0x64;
		break;
	case 0x055:
		paddr_apb_unqual_dec_19_13 = 0x65;
		break;
	case 0x056:
		paddr_apb_unqual_dec_19_13 = 0x66;
		break;
	case 0x057:
		paddr_apb_unqual_dec_19_13 = 0x67;
		break;
	case 0x070:
		paddr_apb_unqual_dec_19_13 = 0x68;
		break;
	case 0x090:
		paddr_apb_unqual_dec_19_13 = 0x69;
		break;
	case 0x190:
		paddr_apb_unqual_dec_19_13 = 0x6a;
		break;
	case 0x290:
		paddr_apb_unqual_dec_19_13 = 0x6b;
		break;
	case 0x390:
		paddr_apb_unqual_dec_19_13 = 0x6c;
		break;
	case 0x0c0:
		paddr_apb_unqual_dec_19_13 = 0x6d;
		break;
	case 0x0d0:
		paddr_apb_unqual_dec_19_13 = 0x6e;
		break;
	default:
		paddr_apb_unqual_dec_19_13 = 0x00;
		break;
	}

	paddr_apb_unqual = (paddr_apb_unqual_dec_19_13 << 13) | (paddr_apb_unqual_dec_12_1 << 1);

	paddr_apb_phy = paddr_apb_unqual << 1;

	return paddr_apb_phy;
}

struct dram_controller imx9_dram_controller = {
	.phy_base = IOMEM(MX9_DDR_PHY_BASE),
	.phy_remap = ddrphy_addr_remap,
	.get_trained_CDD = get_trained_CDD,
	.set_dfi_clk = ddrphy_init_set_dfi_clk,
};

int imx9_ddr_init(struct dram_timing_info *dram_timing, enum dram_type dram_type)
{
	unsigned int initial_drate;
	struct dram_timing_info *saved_timing;
	void *fsp;
	int ret;
	u32 mr12, mr14;
	u32 regval;
	struct dram_controller *dram = &imx9_dram_controller;

	debug("DDRINFO: start DRAM init\n");

	dram->dram_type = dram_type;

	/* reset ddrphy */
	ddrphy_coldreset();

	debug("DDRINFO: cfg clk\n");

	initial_drate = dram_timing->fsp_msg[0].drate;
	/* default to the frequency point 0 clock */
	ddrphy_init_set_dfi_clk(dram, initial_drate);

	/*
	 * Start PHY initialization and training by
	 * accessing relevant PUB registers
	 */
	debug("DDRINFO:ddrphy config start\n");

	ret = ddr_cfg_phy(dram, dram_timing);
	if (ret)
		return ret;

	debug("DDRINFO: ddrphy config done\n");

	update_umctl2_rank_space_setting(dram_timing, dram_timing->fsp_msg_num - 1);

	/* rogram the ddrc registers */
	debug("DDRINFO: ddrc config start\n");
	ddrc_config(dram_timing);
	debug("DDRINFO: ddrc config done\n");

	writel(0x200000, REG_DDR_DEBUG_19);

	check_dfi_init_complete();

	regval = readl(REG_DDR_SDRAM_CFG);
	writel((regval | 0x80000000), REG_DDR_SDRAM_CFG);

	check_ddrc_idle();

	mr12 = lpddr4_mr_read(1, 12);
	mr14 = lpddr4_mr_read(1, 14);

	/* save the dram timing config into memory */
	fsp = dram_config_save(dram, dram_timing, IMX9_SAVED_DRAM_TIMING_BASE);

	saved_timing = (struct dram_timing_info *)IMX9_SAVED_DRAM_TIMING_BASE;
	saved_timing->fsp_cfg = fsp;
	saved_timing->fsp_cfg_num = dram_timing->fsp_cfg_num;
	if (saved_timing->fsp_cfg_num) {
		memcpy(saved_timing->fsp_cfg, dram_timing->fsp_cfg,
		       dram_timing->fsp_cfg_num * sizeof(struct dram_fsp_cfg));

		save_trained_mr12_14(saved_timing->fsp_cfg[0].mr_cfg,
				     ARRAY_SIZE(saved_timing->fsp_cfg[0].mr_cfg), mr12, mr14);
		/*
		 * Configure mode registers in fsp1 to mode register 0 because DDRC
		 * doesn't automatically set.
		 */
		if (saved_timing->fsp_cfg_num > 1)
			update_mr_fsp_op0(saved_timing->fsp_cfg[1].mr_cfg,
					  ARRAY_SIZE(saved_timing->fsp_cfg[1].mr_cfg));
	}

	return 0;
}
