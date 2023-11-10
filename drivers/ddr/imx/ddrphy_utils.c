// SPDX-License-Identifier: GPL-2.0-or-later
/*
* Copyright 2018 NXP
*/

#define pr_fmt(fmt) "imx8m-ddr: " fmt

#include <common.h>
#include <errno.h>
#include <io.h>
#include <linux/iopoll.h>
#include <soc/imx8m/ddr.h>
#include <mach/imx/imx8m-regs.h>
#include <mach/imx/imx8m-ccm-regs.h>

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

void ddrphy_init_set_dfi_clk(unsigned int drate_mhz, enum ddrc_type type)
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
		dram_pll_init(drate, type);
		dram_disable_bypass();
	} else {
		dram_enable_bypass(drate);
	}
}

void ddrphy_init_read_msg_block(enum fw_type type)
{
}
