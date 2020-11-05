// SPDX-License-Identifier: GPL-2.0+
/*
* Copyright 2018 NXP
*/

#define pr_fmt(fmt) "imx8m-ddr: " fmt

#include <common.h>
#include <errno.h>
#include <io.h>
#include <linux/iopoll.h>
#include <soc/imx8m/ddr.h>
#include <mach/imx8m-regs.h>
#include <mach/imx8m-ccm-regs.h>

void ddrc_phy_load_firmware(void __iomem *phy,
			    enum ddrc_phy_firmware_offset offset,
			    const u16 *blob, size_t size)
{
	while (size) {
		writew(*blob++, phy + DDRC_PHY_REG(offset));
		offset++;
		size -= sizeof(*blob);
	}
}

enum pmc_constants {
	PMC_MESSAGE_ID,
	PMC_MESSAGE_STREAM,

	PMC_TRAIN_SUCCESS	= 0x07,
	PMC_TRAIN_STREAM_START	= 0x08,
	PMC_TRAIN_FAIL		= 0xff,
};

static u32 ddrc_phy_get_message(void __iomem *phy, int type)
{
	u32 r, message;

	/*
	 * When BIT0 set to 0, the PMU has a message for the user
	 * Wait for it indefinitely.
	 */
	readl_poll_timeout(phy + DDRC_PHY_REG(0xd0004),
			   r, !(r & BIT(0)), 0);

	switch (type) {
	case PMC_MESSAGE_ID:
		/*
		 * Get the major message ID
		 */
		message = readl(phy + DDRC_PHY_REG(0xd0032));
		break;
	case PMC_MESSAGE_STREAM:
		message = readl(phy + DDRC_PHY_REG(0xd0034));
		message <<= 16;
		message |= readl(phy + DDRC_PHY_REG(0xd0032));
		break;
	}

	/*
	 * By setting this register to 0, the user acknowledges the
	 * receipt of the message.
	 */
	writel(0x00000000, phy + DDRC_PHY_REG(0xd0031));
	/*
	 * When BIT0 set to 0, the PMU has a message for the user
	 */
	readl_poll_timeout(phy + DDRC_PHY_REG(0xd0004),
			   r, r & BIT(0), 0);

	writel(0x00000001, phy + DDRC_PHY_REG(0xd0031));

	return message;
}

static void ddrc_phy_fetch_streaming_message(void __iomem *phy)
{
	const u16 index = ddrc_phy_get_message(phy, PMC_MESSAGE_STREAM);
	u16 i;

	for (i = 0; i < index; i++)
		ddrc_phy_get_message(phy, PMC_MESSAGE_STREAM);
}

int wait_ddrphy_training_complete(void)
{
	void __iomem *phy = IOMEM(MX8M_DDRC_PHY_BASE_ADDR);

	for (;;) {
		const u32 m = ddrc_phy_get_message(phy, PMC_MESSAGE_ID);

		switch (m) {
		case PMC_TRAIN_STREAM_START:
			ddrc_phy_fetch_streaming_message(phy);
			break;
		case PMC_TRAIN_SUCCESS:
			return 0;
		case PMC_TRAIN_FAIL:
			hang();
		}
	}
}

struct dram_bypass_clk_setting {
	ulong clk;
	int alt_root_sel;
	int alt_pre_div;
	int apb_root_sel;
	int apb_pre_div;
};

#define MHZ(x)	(1000000UL * (x))

static struct dram_bypass_clk_setting imx8mq_dram_bypass_tbl[] = {
	{
		.clk = MHZ(100),
		.alt_root_sel = 2,
		.alt_pre_div = 1 - 1,
		.apb_root_sel = 2,
		.apb_pre_div = 2 - 1,
	} , {
		.clk = MHZ(250),
		.alt_root_sel = 3,
		.alt_pre_div = 2 - 1,
		.apb_root_sel = 2,
		.apb_pre_div = 2 - 1,
	}, {
		.clk = MHZ(400),
		.alt_root_sel = 1,
		.alt_pre_div = 2 - 1,
		.apb_root_sel = 3,
		.apb_pre_div = 2 - 1,
	},
};

static void dram_enable_bypass(ulong clk_val)
{
	int i;
	struct dram_bypass_clk_setting *config;

	for (i = 0; i < ARRAY_SIZE(imx8mq_dram_bypass_tbl); i++) {
		if (clk_val == imx8mq_dram_bypass_tbl[i].clk)
			break;
	}

	if (i == ARRAY_SIZE(imx8mq_dram_bypass_tbl)) {
		printf("No matched freq table %lu\n", clk_val);
		return;
	}

	config = &imx8mq_dram_bypass_tbl[i];

	imx8m_clock_set_target_val(IMX8M_DRAM_ALT_CLK_ROOT,
				   IMX8M_CCM_TARGET_ROOTn_ENABLE |
				   IMX8M_CCM_TARGET_ROOTn_MUX(config->alt_root_sel) |
				   IMX8M_CCM_TARGET_ROOTn_PRE_DIV(config->alt_pre_div));
        imx8m_clock_set_target_val(IMX8M_DRAM_APB_CLK_ROOT,
				   IMX8M_CCM_TARGET_ROOTn_ENABLE |
				   IMX8M_CCM_TARGET_ROOTn_MUX(config->apb_root_sel) |
				   IMX8M_CCM_TARGET_ROOTn_PRE_DIV(config->apb_pre_div));
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

struct imx_int_pll_rate_table {
	u32 rate;
	u32 r1;
	u32 r2;
};

#define MDIV(x) ((x) << 12)
#define PDIV(x) ((x) << 4)
#define SDIV(x) ((x) << 0)

#define LOCK_STATUS     BIT(31)
#define LOCK_SEL_MASK   BIT(29)
#define CLKE_MASK       BIT(11)
#define RST_MASK        BIT(9)
#define BYPASS_MASK     BIT(4)

static struct imx_int_pll_rate_table imx8mm_fracpll_tbl[] = {
	{ .rate = 1000000000U, .r1 = MDIV(250) | PDIV(3) | SDIV(1), .r2 = 0 },
	{ .rate = 800000000U,  .r1 = MDIV(300) | PDIV(9) | SDIV(0), .r2 = 0 },
	{ .rate = 750000000U,  .r1 = MDIV(250) | PDIV(8) | SDIV(0), .r2 = 0 },
	{ .rate = 650000000U,  .r1 = MDIV(325) | PDIV(3) | SDIV(2), .r2 = 0 },
	{ .rate = 600000000U,  .r1 = MDIV(300) | PDIV(3) | SDIV(2), .r2 = 0 },
	{ .rate = 594000000U,  .r1 = MDIV( 99) | PDIV(1) | SDIV(2), .r2 = 0 },
	{ .rate = 400000000U,  .r1 = MDIV(300) | PDIV(9) | SDIV(1), .r2 = 0 },
	{ .rate = 266666667U,  .r1 = MDIV(400) | PDIV(9) | SDIV(2), .r2 = 0 },
	{ .rate = 167000000U,  .r1 = MDIV(334) | PDIV(3) | SDIV(4), .r2 = 0 },
	{ .rate = 100000000U,  .r1 = MDIV(300) | PDIV(9) | SDIV(3), .r2 = 0 },
};

static struct imx_int_pll_rate_table *fracpll(u32 freq)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(imx8mm_fracpll_tbl); i++)
		if (freq == imx8mm_fracpll_tbl[i].rate)
			return &imx8mm_fracpll_tbl[i];

	return NULL;
}

static int dram_pll_init(u32 freq)
{
	volatile int i;
	u32 tmp;
	void *pll_base;
	struct imx_int_pll_rate_table *rate;

	rate = fracpll(freq);
	if (!rate) {
		printf("No matched freq table %u\n", freq);
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

	writel(rate->r1, pll_base + 4);
	writel(rate->r2, pll_base + 8);

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

void ddrphy_init_set_dfi_clk(unsigned int drate)
{
	switch (drate) {
	case 4000:
		dram_pll_init(MHZ(1000));
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
	case 2400:
		dram_pll_init(MHZ(600));
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
	case 400:
		dram_enable_bypass(MHZ(400));
		break;
	case 100:
		dram_enable_bypass(MHZ(100));
		break;
	default:
		return;
	}
}

void ddrphy_init_read_msg_block(enum fw_type type)
{
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

void get_trained_CDD(u32 fsp)
{
	unsigned int i, ddr_type, tmp;
	unsigned int cdd_cha[12], cdd_chb[12];
	unsigned int cdd_cha_rr_max, cdd_cha_rw_max, cdd_cha_wr_max, cdd_cha_ww_max;
	unsigned int cdd_chb_rr_max, cdd_chb_rw_max, cdd_chb_wr_max, cdd_chb_ww_max;

	ddr_type = reg32_read(DDRC_MSTR(0)) & 0x3f;
	if (ddr_type == 0x20) {
		for (i = 0; i < 6; i++) {
			tmp = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) +
					 (0x54013UL + i) * 4);
			cdd_cha[i * 2] = tmp & 0xff;
			cdd_cha[i * 2 + 1] = (tmp >> 8) & 0xff;
		}

		for (i = 0; i < 7; i++) {
			tmp = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) +
					 (0x5402cUL + i) * 4);
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
			tmp = reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) +
					 (0x54012UL + i) * 4);
			ddr4_cdd[i * 2] = tmp & 0xff;
			ddr4_cdd[i * 2 + 1] = (tmp >> 8) & 0xff;
		}

		g_cdd_rr_max[fsp] = look_for_max(ddr4_cdd, 1, 12);
		g_cdd_ww_max[fsp] = look_for_max(ddr4_cdd, 13, 24);
		g_cdd_rw_max[fsp] = look_for_max(ddr4_cdd, 25, 40);
		g_cdd_wr_max[fsp] = look_for_max(ddr4_cdd, 41, 56);
	}
}

void update_umctl2_rank_space_setting(unsigned int pstat_num,
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
