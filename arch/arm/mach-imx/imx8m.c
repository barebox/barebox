// SPDX-License-Identifier: GPL-2.0-or-later

#include <init.h>
#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <asm/syscounter.h>
#include <asm/system.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <soc/imx8m/clk-early.h>

#include <linux/bitfield.h>
#include <linux/iopoll.h>

#define IMX_SIP_BUILDINFO			0xC2000003
#define IMX_SIP_BUILDINFO_GET_COMMITHASH	0x00

void imx8m_clock_set_target_val(int clock_id, u32 val)
{
	void __iomem *ccm = IOMEM(MX8M_CCM_BASE_ADDR);

	writel(val, ccm + IMX8M_CCM_TARGET_ROOTn(clock_id));
}

void imx8m_ccgr_clock_enable(int index)
{
	void __iomem *ccm = IOMEM(MX8M_CCM_BASE_ADDR);

	writel(IMX8M_CCM_CCGR_SETTINGn_NEEDED(0),
	       ccm + IMX8M_CCM_CCGRn_SET(index));
}

void imx8m_ccgr_clock_disable(int index)
{
	void __iomem *ccm = IOMEM(MX8M_CCM_BASE_ADDR);

	writel(IMX8M_CCM_CCGR_SETTINGn_NEEDED(0),
	       ccm + IMX8M_CCM_CCGRn_CLR(index));
}

#define INTPLL_DIV20_CLKE_MASK                  BIT(27)
#define INTPLL_DIV10_CLKE_MASK                  BIT(25)
#define INTPLL_DIV8_CLKE_MASK                   BIT(23)
#define INTPLL_DIV6_CLKE_MASK                   BIT(21)
#define INTPLL_DIV5_CLKE_MASK                   BIT(19)
#define INTPLL_DIV4_CLKE_MASK                   BIT(17)
#define INTPLL_DIV3_CLKE_MASK                   BIT(15)
#define INTPLL_DIV2_CLKE_MASK                   BIT(13)
#define INTPLL_CLKE_MASK                        BIT(11)

#define CCM_TARGET_ROOT0_DIV  GENMASK(1, 0)

#define IMX8MM_CCM_ANALOG_ARM_PLL_GEN_CTRL	0x84
#define IMX8MM_CCM_ANALOG_SYS_PLL1_GEN_CTRL	0x94
#define IMX8MM_CCM_ANALOG_SYS_PLL2_GEN_CTRL	0x104
#define IMX8MM_CCM_ANALOG_SYS_PLL3_GEN_CTRL	0x114

static void __imx8m_early_clock_init(int cpu_type)
{
	unsigned long pll3_freq;
	void __iomem *ana = IOMEM(MX8M_ANATOP_BASE_ADDR);
	void __iomem *ccm = IOMEM(MX8M_CCM_BASE_ADDR);
	u32 val;

	imx8m_ccgr_clock_disable(IMX8M_CCM_CCGR_DDR1);

	imx8m_clock_set_target_val(IMX8M_DRAM_ALT_CLK_ROOT,
				   IMX8M_CCM_TARGET_ROOTn_ENABLE |
				   IMX8M_CCM_TARGET_ROOTn_MUX(1));

	/* change the clock source of dram_apb_clk_root: source 4 800MHz /4 = 200MHz */
	imx8m_clock_set_target_val(IMX8M_DRAM_APB_CLK_ROOT,
				   IMX8M_CCM_TARGET_ROOTn_ENABLE |
				   IMX8M_CCM_TARGET_ROOTn_MUX(4) |
				   IMX8M_CCM_TARGET_ROOTn_POST_DIV(4 - 1));

	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_DDR1);

	/*
	 * The gate is not exported to clk tree, so configure them here.
	 * According to ANAMIX SPEC
	 * sys pll1 fixed at 800MHz
	 * sys pll2 fixed at 1GHz
	 * Here we only enable the outputs.
	 */
	val = readl(ana + IMX8MM_CCM_ANALOG_SYS_PLL1_GEN_CTRL);
	val |= INTPLL_CLKE_MASK | INTPLL_DIV2_CLKE_MASK |
		INTPLL_DIV3_CLKE_MASK | INTPLL_DIV4_CLKE_MASK |
		INTPLL_DIV5_CLKE_MASK | INTPLL_DIV6_CLKE_MASK |
		INTPLL_DIV8_CLKE_MASK | INTPLL_DIV10_CLKE_MASK |
		INTPLL_DIV20_CLKE_MASK;
	writel(val, ana + IMX8MM_CCM_ANALOG_SYS_PLL1_GEN_CTRL);

	val = readl(ana + IMX8MM_CCM_ANALOG_SYS_PLL2_GEN_CTRL);
	val |= INTPLL_CLKE_MASK | INTPLL_DIV2_CLKE_MASK |
		INTPLL_DIV3_CLKE_MASK | INTPLL_DIV4_CLKE_MASK |
		INTPLL_DIV5_CLKE_MASK | INTPLL_DIV6_CLKE_MASK |
		INTPLL_DIV8_CLKE_MASK | INTPLL_DIV10_CLKE_MASK |
		INTPLL_DIV20_CLKE_MASK;
	writel(val, ana + IMX8MM_CCM_ANALOG_SYS_PLL2_GEN_CTRL);

	if (cpu_type != IMX_CPU_IMX8MP) {
		/* 8MP ROM already set GIC to 400Mhz, system_pll1_800m with div = 2 */
		/* For everything else, config GIC to sys_pll2_100m */
		imx8m_ccgr_clock_disable(IMX8M_CCM_CCGR_GIC);
		imx8m_clock_set_target_val(IMX8M_GIC_CLK_ROOT,
					   IMX8M_CCM_TARGET_ROOTn_ENABLE |
					   IMX8M_CCM_TARGET_ROOTn_MUX(3));
		imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_GIC);
	}

	if (cpu_type == IMX_CPU_IMX8MN || cpu_type == IMX_CPU_IMX8MP)
		pll3_freq = 600000000UL;
	else
		pll3_freq = 750000000UL;

	/* Configure SYS_PLL3 */
	clk_pll1416x_early_set_rate(ana + IMX8MM_CCM_ANALOG_SYS_PLL3_GEN_CTRL,
				    pll3_freq, 25000000UL);

	if (cpu_type == IMX_CPU_IMX8MP) {
		/* 8MP ROM already set NOC to 800Mhz, only need to configure NOC_IO clk to 600Mhz */
		imx8m_clock_set_target_val(IMX8M_NOC_IO_CLK_ROOT,
					   IMX8M_CCM_TARGET_ROOTn_ENABLE |
					   IMX8M_CCM_TARGET_ROOTn_MUX(2));
	}

	clrsetbits_le32(ccm + IMX8M_CCM_TARGET_ROOTn(IMX8M_ARM_A53_CLK_ROOT),
			IMX8M_CCM_TARGET_ROOTn_MUX(7),
			IMX8M_CCM_TARGET_ROOTn_MUX(2));

	/* Configure ARM PLL to 1.2GHz */
	clk_pll1416x_early_set_rate(ana + IMX8MM_CCM_ANALOG_ARM_PLL_GEN_CTRL,
				    1200000000UL, 25000000UL);

	clrsetbits_le32(ana + IMX8MM_CCM_ANALOG_ARM_PLL_GEN_CTRL, 0,
			INTPLL_CLKE_MASK);

	clrsetbits_le32(ccm + IMX8M_CCM_TARGET_ROOTn(IMX8M_ARM_A53_CLK_ROOT),
			IMX8M_CCM_TARGET_ROOTn_MUX(7),
			IMX8M_CCM_TARGET_ROOTn_MUX(1));

	/* Configure DIV to 1.2GHz */
	clrsetbits_le32(ccm + IMX8M_CCM_TARGET_ROOTn(IMX8M_ARM_A53_CLK_ROOT),
			CCM_TARGET_ROOT0_DIV,
			FIELD_PREP(CCM_TARGET_ROOT0_DIV, 0));
}

void imx8mm_early_clock_init(void)
{
	__imx8m_early_clock_init(IMX_CPU_IMX8MM);
}

void imx8mn_early_clock_init(void)
{
	__imx8m_early_clock_init(IMX_CPU_IMX8MN);
}

void imx8mp_early_clock_init(void)
{
	__imx8m_early_clock_init(IMX_CPU_IMX8MP);
}


#define KEEP_ALIVE			0x18
#define VER_L				0x1c
#define VER_H				0x20
#define VER_LIB_L_ADDR			0x24
#define VER_LIB_H_ADDR			0x28
#define FW_ALIVE_TIMEOUT_US		100000

static int imx8mq_report_hdmi_firmware(void)
{
	void __iomem *hdmi = IOMEM(MX8MQ_HDMI_CTRL_BASE_ADDR);
	u16 ver_lib, ver;
	u32 reg;
	int ret;

	if (!cpu_is_mx8mq())
		return 0;

	/* check the keep alive register to make sure fw working */
	ret = readl_poll_timeout(hdmi + KEEP_ALIVE,
				 reg, reg, FW_ALIVE_TIMEOUT_US);
	if (ret < 0) {
		pr_info("HDP firmware is not running\n");
		return 0;
	}

	ver = readl(hdmi + VER_H) & 0xff;
	ver <<= 8;
	ver |= readl(hdmi + VER_L) & 0xff;

	ver_lib = readl(hdmi + VER_LIB_H_ADDR) & 0xff;
	ver_lib <<= 8;
	ver_lib |= readl(hdmi + VER_LIB_L_ADDR) & 0xff;

	pr_info("HDP firmware ver: %d ver_lib: %d\n", ver, ver_lib);

	return 0;
}
console_initcall(imx8mq_report_hdmi_firmware);

void imx8m_early_setup_uart_clock(void)
{
	imx8m_ccgr_clock_disable(IMX8M_CCM_CCGR_UART1);
	imx8m_ccgr_clock_disable(IMX8M_CCM_CCGR_UART2);
	imx8m_ccgr_clock_disable(IMX8M_CCM_CCGR_UART3);
	imx8m_ccgr_clock_disable(IMX8M_CCM_CCGR_UART4);

        imx8m_clock_set_target_val(IMX8M_UART1_CLK_ROOT,
				   IMX8M_CCM_TARGET_ROOTn_ENABLE |
				   IMX8M_UART1_CLK_ROOT__25M_REF_CLK);
        imx8m_clock_set_target_val(IMX8M_UART2_CLK_ROOT,
				   IMX8M_CCM_TARGET_ROOTn_ENABLE |
				   IMX8M_UART1_CLK_ROOT__25M_REF_CLK);
        imx8m_clock_set_target_val(IMX8M_UART3_CLK_ROOT,
				   IMX8M_CCM_TARGET_ROOTn_ENABLE |
				   IMX8M_UART1_CLK_ROOT__25M_REF_CLK);
        imx8m_clock_set_target_val(IMX8M_UART4_CLK_ROOT,
				   IMX8M_CCM_TARGET_ROOTn_ENABLE |
				   IMX8M_UART1_CLK_ROOT__25M_REF_CLK);

	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_UART1);
	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_UART2);
	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_UART3);
	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_UART4);
}
