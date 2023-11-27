// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2018-2019 NXP
 */

#define pr_fmt(fmt) "imx8m-ddr: " fmt

#include <common.h>
#include <errno.h>
#include <io.h>
#include <soc/imx8m/ddr.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx8m-regs.h>
#include <mach/imx/imx8m-ccm-regs.h>

struct dram_controller imx8m_dram_controller = {
	.phy_base = IOMEM(IP2APB_DDRPHY_IPS_BASE_ADDR(0)),
};

static void ddr_cfg_umctl2(struct dram_controller *dram, struct dram_cfg_param *ddrc_cfg, int num)
{
	int i = 0;

	dram->imx8m_ddr_old_spreadsheet = true;

	for (i = 0; i < num; i++) {
		if (ddrc_cfg->reg == DDRC_ADDRMAP7(0))
		    dram->imx8m_ddr_old_spreadsheet = false;
		reg32_write((unsigned long)ddrc_cfg->reg, ddrc_cfg->val);
		ddrc_cfg++;
	}

	/*
	 * Older NXP DDR configuration spreadsheets don't initialize ADDRMAP7,
	 * which falsifies the memory size read back from the controller
	 * in barebox proper.
	 */
	if (dram->imx8m_ddr_old_spreadsheet) {
		pr_warn("Working around old spreadsheet. Please regenerate\n");
		/*
		 * Alternatively, stick { DDRC_ADDRMAP7(0), 0xf0f } into
		 * struct dram_timing_info::ddrc_cfg of your old timing file
		 */
		reg32_write(DDRC_ADDRMAP7(0), 0xf0f);
	}
}

static unsigned int g_cdd_rr_max[4];
static unsigned int g_cdd_rw_max[4];
static unsigned int g_cdd_wr_max[4];
static unsigned int g_cdd_ww_max[4];

static unsigned int look_for_max(unsigned int data[], unsigned int addr_start,
				 unsigned int addr_end)
{
	unsigned int i, imax = 0;

	for (i = addr_start; i <= addr_end; i++) {
		if (((data[i] >> 7) == 0) && (data[i] > imax))
			imax = data[i];
	}

	return imax;
}

static void get_trained_CDD(struct dram_controller *dram, u32 fsp)
{
	unsigned int i, ddr_type, tmp;
	unsigned int cdd_cha[12], cdd_chb[12];
	unsigned int cdd_cha_rr_max, cdd_cha_rw_max, cdd_cha_wr_max, cdd_cha_ww_max;
	unsigned int cdd_chb_rr_max, cdd_chb_rw_max, cdd_chb_wr_max, cdd_chb_ww_max;

	ddr_type = reg32_read(DDRC_MSTR(0)) & 0x3f;
	if (ddr_type == 0x20) {
		for (i = 0; i < 6; i++) {
			tmp = dwc_ddrphy_apb_rd(dram, 0x54013UL + i);
			cdd_cha[i * 2] = tmp & 0xff;
			cdd_cha[i * 2 + 1] = (tmp >> 8) & 0xff;
		}

		for (i = 0; i < 7; i++) {
			tmp = dwc_ddrphy_apb_rd(dram, 0x5402cUL + i);
			if (i == 0) {
				cdd_cha[0] = (tmp >> 8) & 0xff;
			} else if (i == 6) {
				cdd_cha[11] = tmp & 0xff;
			} else {
				cdd_chb[ i * 2 - 1] = tmp & 0xff;
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
	} else {
		unsigned int ddr4_cdd[64];

		for( i = 0; i < 29; i++) {
			tmp = dwc_ddrphy_apb_rd(dram, 0x54012UL + i);
			ddr4_cdd[i * 2] = tmp & 0xff;
			ddr4_cdd[i * 2 + 1] = (tmp >> 8) & 0xff;
		}

		g_cdd_rr_max[fsp] = look_for_max(ddr4_cdd, 1, 12);
		g_cdd_ww_max[fsp] = look_for_max(ddr4_cdd, 13, 24);
		g_cdd_rw_max[fsp] = look_for_max(ddr4_cdd, 25, 40);
		g_cdd_wr_max[fsp] = look_for_max(ddr4_cdd, 41, 56);
	}
}

static void update_umctl2_rank_space_setting(unsigned int pstat_num,
					     enum ddrc_type type)
{
	unsigned int i,ddr_type;
	unsigned int rdata, tmp, tmp_t;
	unsigned int ddrc_w2r,ddrc_r2w,ddrc_wr_gap,ddrc_rd_gap;
	unsigned long addr_slot;

	ddr_type = reg32_read(DDRC_MSTR(0)) & 0x3f;
	for (i = 0; i < pstat_num; i++) {
		addr_slot = i ? (i + 1) * 0x1000 : 0;
		if (ddr_type == 0x20) {
			/* update r2w:[13:8], w2r:[5:0] */
			rdata = reg32_read(DDRC_DRAMTMG2(0) + addr_slot);
			ddrc_w2r = rdata & 0x3f;
			if (type == DDRC_TYPE_MP)
				tmp = ddrc_w2r + (g_cdd_wr_max[i] >> 1);
			else
				tmp = ddrc_w2r + (g_cdd_wr_max[i] >> 1) + 1;
			ddrc_w2r = (tmp > 0x3f) ? 0x3f : tmp;

			ddrc_r2w = (rdata >> 8) & 0x3f;
			if (type == DDRC_TYPE_MP)
				tmp = ddrc_r2w + (g_cdd_rw_max[i] >> 1);
			else
				tmp = ddrc_r2w + (g_cdd_rw_max[i] >> 1) + 1;
			ddrc_r2w = (tmp > 0x3f) ? 0x3f : tmp;

			tmp_t = (rdata & 0xffffc0c0) | (ddrc_r2w << 8) | ddrc_w2r;
			reg32_write((DDRC_DRAMTMG2(0) + addr_slot), tmp_t);
		} else {
			/* update w2r:[5:0] */
			rdata = reg32_read(DDRC_DRAMTMG9(0) + addr_slot);
			ddrc_w2r = rdata & 0x3f;
			if (type == DDRC_TYPE_MP)
				tmp = ddrc_w2r + (g_cdd_wr_max[i] >> 1);
			else
				tmp = ddrc_w2r + (g_cdd_wr_max[i] >> 1) + 1;
			ddrc_w2r = (tmp > 0x3f) ? 0x3f : tmp;
			tmp_t = (rdata & 0xffffffc0) | ddrc_w2r;
			reg32_write((DDRC_DRAMTMG9(0) + addr_slot), tmp_t);

			/* update r2w:[13:8] */
			rdata = reg32_read(DDRC_DRAMTMG2(0) + addr_slot);
			ddrc_r2w = (rdata >> 8) & 0x3f;
			if (type == DDRC_TYPE_MP)
				tmp = ddrc_r2w + (g_cdd_rw_max[i] >> 1);
			else
				tmp = ddrc_r2w + (g_cdd_rw_max[i] >> 1) + 1;
			ddrc_r2w = (tmp > 0x3f) ? 0x3f : tmp;

			tmp_t = (rdata & 0xffffc0ff) | (ddrc_r2w << 8);
			reg32_write((DDRC_DRAMTMG2(0) + addr_slot), tmp_t);
		}

		if (type != DDRC_TYPE_MQ) {
			/* update rankctl: wr_gap:11:8; rd:gap:7:4; quasi-dymic, doc wrong(static) */
			rdata = reg32_read(DDRC_RANKCTL(0) + addr_slot);
			ddrc_wr_gap = (rdata >> 8) & 0xf;
			if (type == DDRC_TYPE_MP)
				tmp = ddrc_wr_gap + (g_cdd_ww_max[i] >> 1);
			else
				tmp = ddrc_wr_gap + (g_cdd_ww_max[i] >> 1) + 1;
			ddrc_wr_gap = (tmp > 0xf) ? 0xf : tmp;

			ddrc_rd_gap = (rdata >> 4) & 0xf;
			if (type == DDRC_TYPE_MP)
				tmp = ddrc_rd_gap + (g_cdd_rr_max[i] >> 1);
			else
				tmp = ddrc_rd_gap + (g_cdd_rr_max[i] >> 1) + 1;
			ddrc_rd_gap = (tmp > 0xf) ? 0xf : tmp;

			tmp_t = (rdata & 0xfffff00f) | (ddrc_wr_gap << 8) | (ddrc_rd_gap << 4);
			reg32_write((DDRC_RANKCTL(0) + addr_slot), tmp_t);
		}
	}

	if (type == DDRC_TYPE_MQ) {
		/* update rankctl: wr_gap:11:8; rd:gap:7:4; quasi-dymic, doc wrong(static) */
		rdata = reg32_read(DDRC_RANKCTL(0));
		ddrc_wr_gap = (rdata >> 8) & 0xf;
		tmp = ddrc_wr_gap + (g_cdd_ww_max[0] >> 1) + 1;
		ddrc_wr_gap = (tmp > 0xf) ? 0xf : tmp;

		ddrc_rd_gap = (rdata >> 4) & 0xf;
		tmp = ddrc_rd_gap + (g_cdd_rr_max[0] >> 1) + 1;
		ddrc_rd_gap = (tmp > 0xf) ? 0xf : tmp;

		tmp_t = (rdata & 0xfffff00f) | (ddrc_wr_gap << 8) | (ddrc_rd_gap << 4);
		reg32_write(DDRC_RANKCTL(0), tmp_t);
	}
}


/* DDR Transfer rate, bus clock is transfer rate / 2, and the DDRC runs at bus
 * clock / 2, which is therefor transfer rate / 4.  */
enum ddr_rate {
	DDR_4000,
	DDR_3720,
	DDR_3200,
	DDR_3000,
	DDR_2600, /* Unused */
	DDR_2400,
	DDR_2376, /* Unused */
	DDR_1600,
	DDR_1000, /* Unused */
	DDR_1066,
	DDR_667,
	DDR_400,
	DDR_250, /* Unused */
	DDR_100,
	DDR_NUM_RATES
};

/* PLL config for IMX8MM type DRAM PLL.  This PLL type isn't documented, but
 * it looks like it is a basically a fractional PLL:
 * Frequency = Ref (24 MHz) / P * M / 2^S
 * Note: Divider is equal to register value
 */
#define MDIV(x) ((x) << 12)
#define PDIV(x) ((x) << 4)
#define SDIV(x) ((x) << 0)

#define LOCK_STATUS     BIT(31)
#define LOCK_SEL_MASK   BIT(29)
#define CLKE_MASK       BIT(11)
#define RST_MASK        BIT(9)
#define BYPASS_MASK     BIT(4)

static const struct imx8mm_fracpll_config {
	uint32_t r1, r2;
	bool valid;
} imx8mm_fracpll_table[DDR_NUM_RATES] = {
	[DDR_4000] = { .valid = true, .r1 = MDIV(250) | PDIV(3) | SDIV(1), .r2 = 0 },
	[DDR_3720] = { .valid = true, .r1 = MDIV(310) | PDIV(2) | SDIV(2), .r2 = 0 },
	[DDR_3200] = { .valid = true, .r1 = MDIV(300) | PDIV(9) | SDIV(0), .r2 = 0 },
	[DDR_3000] = { .valid = true, .r1 = MDIV(250) | PDIV(8) | SDIV(0), .r2 = 0 },
	[DDR_2600] = { .valid = true, .r1 = MDIV(325) | PDIV(3) | SDIV(2), .r2 = 0 },
	[DDR_2400] = { .valid = true, .r1 = MDIV(300) | PDIV(3) | SDIV(2), .r2 = 0 },
	[DDR_2376] = { .valid = true, .r1 = MDIV( 99) | PDIV(1) | SDIV(2), .r2 = 0 },
	[DDR_1600] = { .valid = true, .r1 = MDIV(300) | PDIV(9) | SDIV(1), .r2 = 0 },
	[DDR_1066] = { .valid = true, .r1 = MDIV(400) | PDIV(9) | SDIV(2), .r2 = 0 },
	[DDR_667]  = { .valid = true, .r1 = MDIV(334) | PDIV(3) | SDIV(4), .r2 = 0 },
	[DDR_400]  = { .valid = true, .r1 = MDIV(300) | PDIV(9) | SDIV(3), .r2 = 0 },
};

/* PLL config for IMX8MQ type DRAM PLL.  This is SSCG_PLL:
 * Frequency = Ref (25 MHz) / divr1 * (2*divf1) / divr2 * divf2 / divq
 * Note: IMX8MQ RM, §5.1.5.4.4 Fig. 5-8 shows ÷2 on divf2, but this is not true.
 * Note: divider is register value + 1
 */
#define SSCG_PLL_LOCK			BIT(31)
#define SSCG_PLL_DRAM_PLL_CLKE		BIT(9)
#define SSCG_PLL_PD			BIT(7)
#define SSCG_PLL_BYPASS1		BIT(5)
#define SSCG_PLL_BYPASS2		BIT(4)

#define SSCG_PLL_REF_DIVR2_MASK		(0x3f << 19)
#define SSCG_PLL_REF_DIVR2_VAL(n)	(((n) << 19) & SSCG_PLL_REF_DIVR2_MASK)
#define SSCG_PLL_FEEDBACK_DIV_F1_MASK	(0x3f << 13)
#define SSCG_PLL_FEEDBACK_DIV_F1_VAL(n)	(((n) << 13) & SSCG_PLL_FEEDBACK_DIV_F1_MASK)
#define SSCG_PLL_FEEDBACK_DIV_F2_MASK	(0x3f << 7)
#define SSCG_PLL_FEEDBACK_DIV_F2_VAL(n)	(((n) << 7) & SSCG_PLL_FEEDBACK_DIV_F2_MASK)
#define SSCG_PLL_OUTPUT_DIV_VAL_MASK	(0x3f << 1)
#define SSCG_PLL_OUTPUT_DIV_VAL(n)	(((n) << 1) & SSCG_PLL_OUTPUT_DIV_VAL_MASK)

#define SSCG_PLL_CFG2(divf1, divr2, divf2, divq) \
	(SSCG_PLL_FEEDBACK_DIV_F1_VAL(divf1) | SSCG_PLL_FEEDBACK_DIV_F2_VAL(divf2) | \
	SSCG_PLL_REF_DIVR2_VAL(divr2) | SSCG_PLL_OUTPUT_DIV_VAL(divq))

static const struct imx8mq_ssgcpll_config {
	uint32_t val;
	bool valid;
} imx8mq_ssgcpll_table[DDR_NUM_RATES] = {
	[DDR_3200] = { .valid = true, .val = SSCG_PLL_CFG2(39, 29, 11, 0) },
	[DDR_2400] = { .valid = true, .val = SSCG_PLL_CFG2(39, 29, 17, 1) },
	[DDR_1600] = { .valid = true, .val = SSCG_PLL_CFG2(39, 29, 11, 1) },
	[DDR_667]  = { .valid = true, .val = SSCG_PLL_CFG2(45, 30, 8, 3) }, /* ~166.935 MHz = 667.74 */
};

/* IMX8M Bypass clock config.  These configure dram_alt1_clk and the dram apb
 * clock.  For the bypass config, clock rate = DRAM tranfer rate, rather than
 * clock = dram / 4
 */

/* prediv is actual divider, register will be set to divider - 1 */
#define CCM_ROOT_CFG(mux, prediv) (IMX8M_CCM_TARGET_ROOTn_ENABLE | \
	IMX8M_CCM_TARGET_ROOTn_MUX(mux) | IMX8M_CCM_TARGET_ROOTn_PRE_DIV(prediv-1))

static const struct imx8m_bypass_config {
	uint32_t alt_clk;
	uint32_t apb_clk;
	bool valid;
} imx8m_bypass_table[DDR_NUM_RATES] = {
	[DDR_400] = { .valid = true, .alt_clk = CCM_ROOT_CFG(1, 2), .apb_clk = CCM_ROOT_CFG(3, 2) },
	[DDR_250] = { .valid = true, .alt_clk = CCM_ROOT_CFG(3, 2), .apb_clk = CCM_ROOT_CFG(2, 2) },
	[DDR_100] = { .valid = true, .alt_clk = CCM_ROOT_CFG(2, 1), .apb_clk = CCM_ROOT_CFG(2, 2) },
};

static void dram_enable_bypass(enum ddr_rate drate)
{
	const struct imx8m_bypass_config *config = &imx8m_bypass_table[drate];

	if (!config->valid) {
		pr_warn("No matched freq table entry %u\n", drate);
		return;
	}

	imx8m_clock_set_target_val(IMX8M_DRAM_ALT_CLK_ROOT, config->alt_clk);
	imx8m_clock_set_target_val(IMX8M_DRAM_APB_CLK_ROOT, config->apb_clk);
	imx8m_clock_set_target_val(IMX8M_DRAM_SEL_CFG, IMX8M_CCM_TARGET_ROOTn_ENABLE |
				   IMX8M_CCM_TARGET_ROOTn_MUX(1));
}

static void dram_disable_bypass(void)
{
	imx8m_clock_set_target_val(IMX8M_DRAM_SEL_CFG,
				   IMX8M_CCM_TARGET_ROOTn_ENABLE |
				   IMX8M_CCM_TARGET_ROOTn_MUX(0));
	imx8m_clock_set_target_val(IMX8M_DRAM_APB_CLK_ROOT,
				   IMX8M_CCM_TARGET_ROOTn_ENABLE |
				   IMX8M_CCM_TARGET_ROOTn_MUX(4) |
				   IMX8M_CCM_TARGET_ROOTn_PRE_DIV(5 - 1));
}

static int dram_frac_pll_init(enum ddr_rate drate)
{
	volatile int i;
	u32 tmp;
	void *pll_base;
	const struct imx8mm_fracpll_config *config = &imx8mm_fracpll_table[drate];

	if (!config->valid) {
		pr_warn("No matched freq table entry %u\n", drate);
		return -EINVAL;
	}

	setbits_le32(MX8M_GPC_BASE_ADDR + 0xec, 1 << 7);
	setbits_le32(MX8M_GPC_BASE_ADDR + 0xf8, 1 << 5);
	writel(0x8F000000UL, MX8M_SRC_BASE_ADDR + 0x1004);

	pll_base = IOMEM(MX8M_ANATOP_BASE_ADDR) + 0x50;

	/* Bypass clock and set lock to pll output lock */
	tmp = readl(pll_base);
	tmp |= BYPASS_MASK;
	writel(tmp, pll_base);

	/* Enable RST */
	tmp &= ~RST_MASK;
	writel(tmp, pll_base);

	writel(config->r1, pll_base + 4);
	writel(config->r2, pll_base + 8);

	for (i = 0; i < 1000; i++);

	/* Disable RST */
	tmp |= RST_MASK;
	writel(tmp, pll_base);

	/* Wait Lock*/
	while (!(readl(pll_base) & LOCK_STATUS));

	/* Bypass */
	tmp &= ~BYPASS_MASK;
	writel(tmp, pll_base);

	return 0;
}

static int dram_sscg_pll_init(enum ddr_rate drate)
{
	u32 val;
	void __iomem *pll_base = IOMEM(MX8M_ANATOP_BASE_ADDR) + 0x60;
	const struct imx8mq_ssgcpll_config *config = &imx8mq_ssgcpll_table[drate];

	if (!config->valid) {
		pr_warn("No matched freq table entry %u\n", drate);
		return -EINVAL;
	}

	/* Bypass */
	setbits_le32(pll_base, SSCG_PLL_BYPASS1 | SSCG_PLL_BYPASS2);

	val = readl(pll_base + 0x8);
	val &= ~(SSCG_PLL_OUTPUT_DIV_VAL_MASK |
		 SSCG_PLL_FEEDBACK_DIV_F2_MASK |
		 SSCG_PLL_FEEDBACK_DIV_F1_MASK |
		 SSCG_PLL_REF_DIVR2_MASK);
	val |= config->val;
	writel(val, pll_base + 0x8);

	/* Clear power down bit */
	clrbits_le32(pll_base, SSCG_PLL_PD);
	/* Enable PLL  */
	setbits_le32(pll_base, SSCG_PLL_DRAM_PLL_CLKE);

	/* Clear bypass */
	clrbits_le32(pll_base, SSCG_PLL_BYPASS1);
	udelay(100);
	clrbits_le32(pll_base, SSCG_PLL_BYPASS2);
	/* Wait lock */
	while (!(readl(pll_base) & SSCG_PLL_LOCK))
		;

	return 0;
}

static int dram_pll_init(enum ddr_rate drate, enum ddrc_type type)
{
	switch (type) {
	case DDRC_TYPE_MQ:
		return dram_sscg_pll_init(drate);
	case DDRC_TYPE_MM:
	case DDRC_TYPE_MN:
	case DDRC_TYPE_MP:
		return dram_frac_pll_init(drate);
	default:
		return -ENODEV;
	}
}

static void ddrphy_init_set_dfi_clk(struct dram_controller *dram, unsigned int drate_mhz)
{
	enum ddr_rate drate;

	switch (drate_mhz) {
	case 4000: drate = DDR_4000; break;
	case 3720: drate = DDR_3720; break;
	case 3200: drate = DDR_3200; break;
	case 3000: drate = DDR_3000; break;
	case 2400: drate = DDR_2400; break;
	case 1600: drate = DDR_1600; break;
	case 1066: drate = DDR_1066; break;
	case 667: drate = DDR_667; break;
	case 400: drate = DDR_400; break;
	case 100: drate = DDR_100; break;
	default:
		pr_warn("Unsupported frequency %u\n", drate_mhz);
		return;
	}

	if (drate_mhz > 400) {
		dram_pll_init(drate, dram->ddrc_type);
		dram_disable_bypass();
	} else {
		dram_enable_bypass(drate);
	}
}

/*
 * We store the timing parameters here. the TF-A will pick these up.
 * Note that the timing used we leave the driver with is a PLL bypass 25MHz
 * mode. So if your board runs horribly slow you'll likely have to provide a
 * TF-A binary.
 */
#define IMX8M_SAVED_DRAM_TIMING_BASE		0x180000

int imx8m_ddr_init(struct dram_controller *dram, struct dram_timing_info *dram_timing)
{
	unsigned long src_ddrc_rcr = MX8M_SRC_DDRC_RCR_ADDR;
	unsigned int tmp, initial_drate, target_freq;
	int ret;

	pr_debug("start DRAM init\n");

	dram->get_trained_CDD = get_trained_CDD;
	dram->set_dfi_clk = ddrphy_init_set_dfi_clk;

	/* Step1: Follow the power up procedure */
	switch (dram->ddrc_type) {
	case DDRC_TYPE_MQ:
		reg32_write(src_ddrc_rcr + 0x04, 0x8f00000f);
		reg32_write(src_ddrc_rcr, 0x8f00000f);
		reg32_write(src_ddrc_rcr + 0x04, 0x8f000000);
		break;
	case DDRC_TYPE_MM:
	case DDRC_TYPE_MN:
	case DDRC_TYPE_MP:
		reg32_write(src_ddrc_rcr, 0x8f00001f);
		reg32_write(src_ddrc_rcr, 0x8f00000f);
		break;
	}

	pr_debug("cfg clk\n");

	/* disable iso */
	reg32_write(0x303A00EC, 0x0000ffff); /* PGC_CPU_MAPPING */
	reg32setbit(0x303A00F8, 5); /* PU_PGC_SW_PUP_REQ */

	initial_drate = dram_timing->fsp_msg[0].drate;
	/* default to the frequency point 0 clock */
	dram->set_dfi_clk(dram, initial_drate);

	/* D-aasert the presetn */
	reg32_write(src_ddrc_rcr, 0x8F000006);

	/* Step2: Program the dwc_ddr_umctl2 registers */
	pr_debug("ddrc config start\n");
	ddr_cfg_umctl2(dram, dram_timing->ddrc_cfg, dram_timing->ddrc_cfg_num);
	pr_debug("ddrc config done\n");

	/* Step3: De-assert reset signal(core_ddrc_rstn & aresetn_n) */
	reg32_write(src_ddrc_rcr, 0x8F000004);
	reg32_write(src_ddrc_rcr, 0x8F000000);

	/*
	 * Step4: Disable auto-refreshes, self-refresh, powerdown, and
	 * assertion of dfi_dram_clk_disable by setting RFSHCTL3.dis_auto_refresh = 1,
	 * PWRCTL.powerdown_en = 0, and PWRCTL.selfref_en = 0,
	 * PWRCTL.en_dfi_dram_clk_disable = 0
	 */
	reg32_write(DDRC_DBG1(0), 0x00000000);
	reg32_write(DDRC_RFSHCTL3(0), 0x0000001);
	reg32_write(DDRC_PWRCTL(0), 0xa0);

	pr_debug("checking ddr type\n");
	/*
	 * below is first read, so if boot hangs here, imx8m*_early_clock_init()
	 * might not have been called
	 */
	tmp = reg32_read(DDRC_MSTR(0));
	if (tmp & (0x1 << 5) && dram->ddrc_type != DDRC_TYPE_MN)
		reg32_write(MX8M_DDRC_DDR_SS_GPR0, 0x01); /* LPDDR4 mode */

	/* determine the initial boot frequency */
	target_freq = reg32_read(DDRC_MSTR2(0)) & 0x3;
	target_freq = (tmp & (0x1 << 29)) ? target_freq : 0x0;

	/* Step5: Set SWCT.sw_done to 0 */
	reg32_write(DDRC_SWCTL(0), 0x00000000);

	/* Set the default boot frequency point */
	clrsetbits_le32(DDRC_DFIMISC(0), (0x1f << 8), target_freq << 8);
	/* Step6: Set DFIMISC.dfi_init_complete_en to 0 */
	clrbits_le32(DDRC_DFIMISC(0), 0x1);

	/* Step7: Set SWCTL.sw_done to 1; need to polling SWSTAT.sw_done_ack */
	reg32_write(DDRC_SWCTL(0), 0x00000001);
	do {
		tmp = reg32_read(DDRC_SWSTAT(0));
	} while ((tmp & 0x1) == 0x0);

	/*
	 * Step8 ~ Step13: Start PHY initialization and training by
	 * accessing relevant PUB registers
	 */
	pr_debug("ddrphy config start\n");

	ret = ddr_cfg_phy(dram, dram_timing);
	if (ret)
		return ret;

	pr_debug("ddrphy config done\n");

	/*
	 * step14 CalBusy.0 =1, indicates the calibrator is actively
	 * calibrating. Wait Calibrating done.
	 */
	do {
		tmp = reg32_read(DDRPHY_CalBusy(0));
	} while ((tmp & 0x1));

	pr_debug("ddrphy calibration done\n");

	/* Step15: Set SWCTL.sw_done to 0 */
	reg32_write(DDRC_SWCTL(0), 0x00000000);

	/* Apply rank-to-rank workaround */
	update_umctl2_rank_space_setting(dram_timing->fsp_msg_num - 1, dram->ddrc_type);

	/* Step16: Set DFIMISC.dfi_init_start to 1 */
	setbits_le32(DDRC_DFIMISC(0), (0x1 << 5));

	/* Step17: Set SWCTL.sw_done to 1; need to polling SWSTAT.sw_done_ack */
	reg32_write(DDRC_SWCTL(0), 0x00000001);
	do {
		tmp = reg32_read(DDRC_SWSTAT(0));
	} while ((tmp & 0x1) == 0x0);

	/* Step18: Polling DFISTAT.dfi_init_complete = 1 */
	do {
		tmp = reg32_read(DDRC_DFISTAT(0));
	} while ((tmp & 0x1) == 0x0);

	/* Step19: Set SWCTL.sw_done to 0 */
	reg32_write(DDRC_SWCTL(0), 0x00000000);

	/* Step20: Set DFIMISC.dfi_init_start to 0 */
	clrbits_le32(DDRC_DFIMISC(0), (0x1 << 5));

	/* Step21: optional */

	/* Step22: Set DFIMISC.dfi_init_complete_en to 1 */
	setbits_le32(DDRC_DFIMISC(0), 0x1);

	/* Step23: Set PWRCTL.selfref_sw to 0 */
	clrbits_le32(DDRC_PWRCTL(0), (0x1 << 5));

	/* Step24: Set SWCTL.sw_done to 1; need polling SWSTAT.sw_done_ack */
	reg32_write(DDRC_SWCTL(0), 0x00000001);
	do {
		tmp = reg32_read(DDRC_SWSTAT(0));
	} while ((tmp & 0x1) == 0x0);

	/* Step25: Wait for dwc_ddr_umctl2 to move to normal operating mode by monitoring
	 * STAT.operating_mode signal */
	do {
		tmp = reg32_read(DDRC_STAT(0));
	} while ((tmp & 0x3) != 0x1);

	/* Step26: Set back register in Step4 to the original values if desired */
	reg32_write(DDRC_RFSHCTL3(0), 0x0000000);
	/* enable selfref_en by default */
	setbits_le32(DDRC_PWRCTL(0), 0x1);

	/* enable port 0 */
	reg32_write(DDRC_PCTRL_0(0), 0x00000001);
	pr_debug("ddrmix config done\n");

	/* save the dram timing config into memory */
	dram_config_save(dram, dram_timing, IMX8M_SAVED_DRAM_TIMING_BASE);

	return 0;
}
