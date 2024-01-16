// SPDX-License-Identifier: GPL-2.0-or-later

#include <init.h>
#include <common.h>
#include <asm/optee.h>
#include <linux/sizes.h>
#include <io.h>
#include <pm_domain.h>
#include <asm/syscounter.h>
#include <asm/system.h>
#include <asm-generic/memory_layout.h>
#include <mach/imx/generic.h>
#include <mach/imx/revision.h>
#include <mach/imx/imx8mq.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mach/imx/reset-reason.h>
#include <mach/imx/ocotp.h>
#include <mach/imx/imx8mp-regs.h>
#include <mach/imx/imx8mq-regs.h>
#include <mach/imx/scratch.h>
#include <mach/imx/tzasc.h>
#include <soc/imx8m/clk-early.h>
#include <tee/optee.h>

#include <linux/iopoll.h>
#include <linux/arm-smccc.h>

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

u64 imx8m_uid(void)
{
	return imx_ocotp_read_uid(IOMEM(MX8M_OCOTP_BASE_ADDR));
}

static int imx8m_init(const char *cputypestr)
{
	void __iomem *src = IOMEM(MX8M_SRC_BASE_ADDR);

	genpd_activate();

	/*
	 * Reset reasons seem to be identical to that of i.MX7
	 */
	imx_set_reset_reason(src + IMX7_SRC_SRSR, imx7_reset_reasons);
	pr_info("%s unique ID: %llx\n", cputypestr, imx8m_uid());

	if (IS_ENABLED(CONFIG_PBL_OPTEE) && tzc380_is_enabled()) {
		static struct of_optee_fixup_data optee_fixup_data = {
			.shm_size = OPTEE_SHM_SIZE,
			.method = "smc",
		};

		optee_set_membase(imx_scratch_get_optee_hdr());
		of_optee_fixup(of_get_root_node(), &optee_fixup_data);
		of_register_fixup(of_optee_fixup, &optee_fixup_data);
	}

	return 0;
}

int imx8mm_init(void)
{
	void __iomem *anatop = IOMEM(MX8M_ANATOP_BASE_ADDR);
	uint32_t type = FIELD_GET(DIGPROG_MAJOR,
				  readl(anatop + MX8MM_ANATOP_DIGPROG));
	const char *cputypestr;

	imx8mm_boot_save_loc();

	switch (type) {
	case IMX8M_CPUTYPE_IMX8MM:
		cputypestr = "i.MX8MM";
		break;
	default:
		cputypestr = "unknown i.MX8M";
		break;
	};

	imx_set_silicon_revision(cputypestr, imx8mm_cpu_revision());

	return imx8m_init(cputypestr);
}

int imx8mn_init(void)
{
	void __iomem *anatop = IOMEM(MX8M_ANATOP_BASE_ADDR);
	uint32_t type = FIELD_GET(DIGPROG_MAJOR,
				  readl(anatop + MX8MN_ANATOP_DIGPROG));
	const char *cputypestr;

	imx8mn_boot_save_loc();

	switch (type) {
	case IMX8M_CPUTYPE_IMX8MN:
		cputypestr = "i.MX8MN";
		break;
	default:
		cputypestr = "unknown i.MX8M";
		break;
	};

	imx_set_silicon_revision(cputypestr, imx8mn_cpu_revision());

	return imx8m_init(cputypestr);
}

int imx8mp_init(void)
{
	void __iomem *anatop = IOMEM(MX8MP_ANATOP_BASE_ADDR);
	uint32_t type = FIELD_GET(DIGPROG_MAJOR,
				  readl(anatop + MX8MP_ANATOP_DIGPROG));
	const char *cputypestr;

	imx8mp_boot_save_loc();

	switch (type) {
	case IMX8M_CPUTYPE_IMX8MP:
		cputypestr = "i.MX8MP";
		break;
	default:
		cputypestr = "unknown i.MX8M";
		break;
	};

	imx_set_silicon_revision(cputypestr, imx8mp_cpu_revision());

	return imx8m_init(cputypestr);
}

int imx8mq_init(void)
{
	void __iomem *anatop = IOMEM(MX8M_ANATOP_BASE_ADDR);
	uint32_t type = FIELD_GET(DIGPROG_MAJOR,
				  readl(anatop + MX8MQ_ANATOP_DIGPROG));
	const char *cputypestr;

	imx8mq_boot_save_loc();

	switch (type) {
	case IMX8M_CPUTYPE_IMX8MQ:
		cputypestr = "i.MX8MQ";
		break;
	default:
		cputypestr = "unknown i.MX8M";
		break;
	};

	imx_set_silicon_revision(cputypestr, imx8mq_cpu_revision());

	return imx8m_init(cputypestr);
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

static void __imx8m_early_clock_init(unsigned long pll3_freq) /* and later */
{
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

	/* config GIC to sys_pll2_100m */
	imx8m_ccgr_clock_disable(IMX8M_CCM_CCGR_GIC);
	imx8m_clock_set_target_val(IMX8M_GIC_CLK_ROOT,
				   IMX8M_CCM_TARGET_ROOTn_ENABLE |
				   IMX8M_CCM_TARGET_ROOTn_MUX(3));
	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_GIC);

	/* Configure SYS_PLL3 */
	clk_pll1416x_early_set_rate(ana + IMX8MM_CCM_ANALOG_SYS_PLL3_GEN_CTRL,
				    pll3_freq, 25000000UL);

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
	__imx8m_early_clock_init(750000000UL);
}

void imx8mn_early_clock_init(void)
{
	__imx8m_early_clock_init(600000000UL);
}

void imx8mp_early_clock_init(void)
{
	__imx8m_early_clock_init(750000000UL);
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
