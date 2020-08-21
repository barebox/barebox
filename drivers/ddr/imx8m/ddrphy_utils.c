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
