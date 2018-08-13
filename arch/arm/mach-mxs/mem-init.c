/*
 * Freescale i.MX28 RAM init
 *
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * Copyright 2013 Stefan Roese <sr@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <config.h>
#include <io.h>
#include <mach/imx-regs.h>
#include <linux/compiler.h>

#include <mach/init.h>
#include <mach/regs-power-mx28.h>
#if defined CONFIG_ARCH_IMX23
# include <mach/regs-clkctrl-mx23.h>
#endif
#if defined CONFIG_ARCH_IMX28
# include <mach/regs-clkctrl-mx28.h>
#endif

/* 1 second delay should be plenty of time for block reset. */
#define	RESET_MAX_TIMEOUT	1000000

#define	MXS_BLOCK_SFTRST	(1 << 31)
#define	MXS_BLOCK_CLKGATE	(1 << 30)

int mxs_early_wait_mask_set(struct mxs_register_32 *reg, uint32_t mask, unsigned
								int timeout)
{
	while (--timeout) {
		if ((readl(&reg->reg) & mask) == mask)
			break;
		mxs_early_delay(1);
	}

	return !timeout;
}

int mxs_early_wait_mask_clr(struct mxs_register_32 *reg, uint32_t mask, unsigned
								int timeout)
{
	while (--timeout) {
		if ((readl(&reg->reg) & mask) == 0)
			break;
		mxs_early_delay(1);
	}

	return !timeout;
}

int mxs_early_reset_block(struct mxs_register_32 *reg)
{
	/* Clear SFTRST */
	writel(MXS_BLOCK_SFTRST, &reg->reg_clr);

	if (mxs_early_wait_mask_clr(reg, MXS_BLOCK_SFTRST, RESET_MAX_TIMEOUT))
		return 1;

	/* Clear CLKGATE */
	writel(MXS_BLOCK_CLKGATE, &reg->reg_clr);

	/* Set SFTRST */
	writel(MXS_BLOCK_SFTRST, &reg->reg_set);

	/* Wait for CLKGATE being set */
	if (mxs_early_wait_mask_set(reg, MXS_BLOCK_CLKGATE, RESET_MAX_TIMEOUT))
		return 1;

	/* Clear SFTRST */
	writel(MXS_BLOCK_SFTRST, &reg->reg_clr);

	if (mxs_early_wait_mask_clr(reg, MXS_BLOCK_SFTRST, RESET_MAX_TIMEOUT))
		return 1;

	/* Clear CLKGATE */
	writel(MXS_BLOCK_CLKGATE, &reg->reg_clr);

	if (mxs_early_wait_mask_clr(reg, MXS_BLOCK_CLKGATE, RESET_MAX_TIMEOUT))
		return 1;

	return 0;
}

const uint32_t mx28_dram_vals_default[190] = {
/*
 * i.MX28 DDR2 at 200MHz
 */
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000100, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00010101, 0x01010101,
	0x000f0f01, 0x0f02020a, 0x00000000, 0x00010101,
	0x00000100, 0x00000100, 0x00000000, 0x00000002,
	0x01010000, 0x07080403, 0x06005003, 0x0a0000c8,
	0x02009c40, 0x0002030c, 0x0036a609, 0x031a0612,
	0x02030202, 0x00c8001c, 0x00000000, 0x00000000,
	0x00012100, 0xffff0303, 0x00012100, 0xffff0303,
	0x00012100, 0xffff0303, 0x00012100, 0xffff0303,
	0x00000003, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000612, 0x01000F02,
	0x06120612, 0x00000200, 0x00020007, 0xf4004a27,
	0xf4004a27, 0xf4004a27, 0xf4004a27, 0x07000300,
	0x07000300, 0x07400300, 0x07400300, 0x00000005,
	0x00000000, 0x00000000, 0x01000000, 0x01020408,
	0x08040201, 0x000f1133, 0x00000000, 0x00001f04,
	0x00001f04, 0x00001f04, 0x00001f04, 0x00001f04,
	0x00001f04, 0x00001f04, 0x00001f04, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00010000, 0x00030404,
	0x00000003, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x01010000,
	0x01000000, 0x03030000, 0x00010303, 0x01020202,
	0x00000000, 0x02040303, 0x21002103, 0x00061200,
	0x06120612, 0x04420442, 0x04420442, 0x00040004,
	0x00040004, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0xffffffff
};

/*
 * i.MX23 DDR at 133MHz
 */
uint32_t mx23_dram_vals[] = {
	0x01010001, 0x00010100, 0x01000101, 0x00000001,
	0x00000101, 0x00000000, 0x00010000, 0x01000001,
	0x00000000, 0x00000001, 0x07000200, 0x00070202,
	0x02020000, 0x04040a01, 0x00000201, 0x02040000,
	0x02000000, 0x19000f08, 0x0d0d0000, 0x02021313,
	0x02061521, 0x0000000a, 0x00080008, 0x00200020,
	0x00200020, 0x00200020, 0x000003f7, 0x00000000,
	0x00000000, 0x00000020, 0x00000020, 0x00c80000,
	0x000a23cd, 0x000000c8, 0x00006665, 0x00000000,
	0x00000101, 0x00040001, 0x00000000, 0x00000000,
	0x00010000
};

static void mx28_initialize_dram_values(const uint32_t dram_vals[190])
{
	int i;

	for (i = 0; i < 190; i++)
		writel(dram_vals[i], IMX_SDRAMC_BASE + (4 * i));
}

static void mx23_initialize_dram_values(void)
{
	int i;

	/*
	 * HW_DRAM_CTL27, HW_DRAM_CTL28 and HW_DRAM_CTL35 are not initialized as
	 * per FSL bootlets code.
	 *
	 * mx23 Reference Manual marks HW_DRAM_CTL27 and HW_DRAM_CTL28 as
	 * "reserved".
	 * HW_DRAM_CTL8 is setup as the last element.
	 * So skip the initialization of these HW_DRAM_CTL registers.
	 */
	for (i = 0; i < ARRAY_SIZE(mx23_dram_vals); i++) {
		if (i == 8 || i == 27 || i == 28 || i == 35)
			continue;
		writel(mx23_dram_vals[i], IMX_SDRAMC_BASE + (4 * i));
	}

	/*
	 * Enable tRAS lockout in HW_DRAM_CTL08 ; it must be the last
	 * element to be set
	 */
	writel((1 << 24), IMX_SDRAMC_BASE + (4 * 8));
}

/**
 * Set up the EMI clock.
 * @clk_emi_div: integer divider (prescaler), the DIV_EMI field in HW_CLKCTRL_EMI
 * @clk_emi_frac: fractional divider, the EMIFRAC field in HW_CLKCTRL_FRAC0
 */
void mxs_mem_init_clock(const uint8_t clk_emi_div, const uint8_t clk_emi_frac)
{
	struct mxs_clkctrl_regs *clkctrl_regs =
		(struct mxs_clkctrl_regs *)IMX_CCM_BASE;

	/* Gate EMI clock */
	writeb(CLKCTRL_FRAC_CLKGATE,
		&clkctrl_regs->hw_clkctrl_frac0_set[CLKCTRL_FRAC0_EMI]);

	/* Set fractional divider for ref_emi */
	writeb(CLKCTRL_FRAC_CLKGATE | (clk_emi_frac & CLKCTRL_FRAC_FRAC_MASK),
		&clkctrl_regs->hw_clkctrl_frac0[CLKCTRL_FRAC0_EMI]);

	/* Ungate EMI clock */
	writeb(CLKCTRL_FRAC_CLKGATE,
		&clkctrl_regs->hw_clkctrl_frac0_clr[CLKCTRL_FRAC0_EMI]);

	mxs_early_delay(11000);

	/* Set EMI clock prescaler */
	writel(((clk_emi_div & CLKCTRL_EMI_DIV_EMI_MASK) << CLKCTRL_EMI_DIV_EMI_OFFSET) |
		(1 << CLKCTRL_EMI_DIV_XTAL_OFFSET),
		&clkctrl_regs->hw_clkctrl_emi);

	/* Unbypass EMI */
	writel(CLKCTRL_CLKSEQ_BYPASS_EMI,
		&clkctrl_regs->hw_clkctrl_clkseq_clr);

	mxs_early_delay(10000);
}

void mxs_mem_setup_cpu_and_hbus(void)
{
	struct mxs_clkctrl_regs *clkctrl_regs =
		(struct mxs_clkctrl_regs *)IMX_CCM_BASE;

	/* Set fractional divider for ref_cpu to 480 * 18 / 19 = 454MHz
	 * and ungate CPU clock */
	writeb(19 & CLKCTRL_FRAC_FRAC_MASK,
		(uint8_t *)&clkctrl_regs->hw_clkctrl_frac0[CLKCTRL_FRAC0_CPU]);

	/* Set CPU bypass */
	writel(CLKCTRL_CLKSEQ_BYPASS_CPU,
		&clkctrl_regs->hw_clkctrl_clkseq_set);

	/* HBUS = 151MHz */
	writel(CLKCTRL_HBUS_DIV_MASK, &clkctrl_regs->hw_clkctrl_hbus_set);
	writel(((~3) << CLKCTRL_HBUS_DIV_OFFSET) & CLKCTRL_HBUS_DIV_MASK,
		&clkctrl_regs->hw_clkctrl_hbus_clr);

	mxs_early_delay(10000);

	/* CPU clock divider = 1 */
	clrsetbits_le32(&clkctrl_regs->hw_clkctrl_cpu,
			CLKCTRL_CPU_DIV_CPU_MASK, 1);

	/* Disable CPU bypass */
	writel(CLKCTRL_CLKSEQ_BYPASS_CPU,
		&clkctrl_regs->hw_clkctrl_clkseq_clr);

	mxs_early_delay(15000);
}

static void mx23_mem_setup_vddmem(void)
{
	struct mxs_power_regs *power_regs =
		(struct mxs_power_regs *)IMX_POWER_BASE;

	/* We must wait before and after disabling the current limiter! */
	mxs_early_delay(10000);

	clrbits_le32(&power_regs->hw_power_vddmemctrl,
		POWER_VDDMEMCTRL_ENABLE_ILIMIT);

	mxs_early_delay(10000);
}

void mx23_mem_init(void)
{
	mxs_early_delay(11000);

	/*
	 * Reset/ungate the EMI block. This is essential, otherwise the system
	 * suffers from memory instability. This thing is mx23 specific and is
	 * no longer present on mx28.
	 */
	mxs_early_reset_block((struct mxs_register_32 *)IMX_EMI_BASE);

	mx23_mem_setup_vddmem();

	/*
	 * Configure the DRAM registers
	 */

	/* Clear START and SREFRESH bit from DRAM_CTL8 */
	clrbits_le32(IMX_SDRAMC_BASE + 0x20, (1 << 16) | (1 << 8));

	mx23_initialize_dram_values();

	/* Set START bit in DRAM_CTL8 */
	setbits_le32(IMX_SDRAMC_BASE + 0x20, 1 << 16);

	clrbits_le32(IMX_SDRAMC_BASE + 0x40, 1 << 17);

	/* Wait for EMI_STAT bit DRAM_HALTED */
	for (;;) {
		if (!(readl(IMX_EMI_BASE + 0x10) & (1 << 1)))
			break;
		mxs_early_delay(1000);
	}

	/* Adjust EMI port priority. */
	clrsetbits_le32(0x80020000, 0x1f << 16, 0x2);
	mxs_early_delay(20000);

	setbits_le32(IMX_SDRAMC_BASE + 0x40, 1 << 19);
	setbits_le32(IMX_SDRAMC_BASE + 0x40, 1 << 11);

	mxs_early_delay(10000);

	mxs_mem_setup_cpu_and_hbus();
}

void mx28_mem_init(const int emi_ds_ctrl_ddr_mode, const uint32_t dram_vals[190])
{
	mxs_early_delay(11000);

	/* Set DDR mode */
	writel(emi_ds_ctrl_ddr_mode, IMX_IOMUXC_BASE + 0x1b80);

	/*
	 * Configure the DRAM registers
	 */

	/* Clear START bit from DRAM_CTL16 */
	clrbits_le32(IMX_SDRAMC_BASE + 0x40, 1);

	mx28_initialize_dram_values(dram_vals);

	/* Clear SREFRESH bit from DRAM_CTL17 */
	clrbits_le32(IMX_SDRAMC_BASE + 0x44, 1);

	/* Set START bit in DRAM_CTL16 */
	setbits_le32(IMX_SDRAMC_BASE + 0x40, 1);

	/* Wait for bit 20 (DRAM init complete) in DRAM_CTL58 */
	while (!(readl(IMX_SDRAMC_BASE + 0xe8) & (1 << 20)))
		;

	mxs_early_delay(10000);

	mxs_mem_setup_cpu_and_hbus();
}
