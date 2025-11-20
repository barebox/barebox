// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2014 Lucas Stach, Pengutronix

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/errata.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <mach/imx/errata.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx7-regs.h>
#include <mach/imx/imx8mq-regs.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mach/imx/imx9-regs.h>
#include <mach/imx/trdc.h>
#include <io.h>
#include <asm/cache.h>
#include <asm/syscounter.h>
#include <asm/system.h>

static inline void imx_cpu_timer_init(void __iomem *syscnt)
{
	set_cntfrq(syscnt_get_cntfrq(syscnt));
	syscnt_enable(syscnt);
}

#ifdef CONFIG_CPU_32
void imx5_cpu_lowlevel_init(void)
{
	arm_cpu_lowlevel_init();

	enable_arm_errata_709718_war();
	enable_arm_errata_cortexa8_enable_ibe();
}

void imx6_cpu_lowlevel_init(void)
{
	arm_cpu_lowlevel_init();

	arm_early_mmu_cache_invalidate();

	/* apply necessary workarounds for Cortex A9 r2p10 */
	enable_arm_errata_742230_war();
	enable_arm_errata_743622_war();
	enable_arm_errata_751472_war();
	enable_arm_errata_761320_war();
	enable_arm_errata_794072_war();
	enable_arm_errata_845369_war();
}

void imx6ul_cpu_lowlevel_init(void)
{
	cortex_a7_lowlevel_init();
	arm_cpu_lowlevel_init();
}

void imx7_cpu_lowlevel_init(void)
{
	cortex_a7_lowlevel_init();
	arm_cpu_lowlevel_init();
	imx_cpu_timer_init(IOMEM(MX7_SYSCNT_CTRL_BASE_ADDR));
}

void vf610_cpu_lowlevel_init(void)
{
	arm_cpu_lowlevel_init();
}
#else
static void imx8m_cpu_lowlevel_init(void)
{
	arm_cpu_lowlevel_init();

	if (current_el() == 3)
		imx_cpu_timer_init(IOMEM(MX8M_SYSCNT_CTRL_BASE_ADDR));
}

void imx8mm_cpu_lowlevel_init(void)
{
	/* ungate system counter */
	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_SCTR);

	imx8m_cpu_lowlevel_init();

	erratum_050350_imx8m();
}

void imx8mn_cpu_lowlevel_init(void)
	__alias(imx8mm_cpu_lowlevel_init);

void imx8mp_cpu_lowlevel_init(void)
{
	/* ungate system counter */
	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_SCTR);

	imx8m_cpu_lowlevel_init();
}

void imx8mq_cpu_lowlevel_init(void)
{
	imx8m_cpu_lowlevel_init();

	erratum_050350_imx8m();
}

#define CCM_AUTHEN_TZ_NS	BIT(9)

#define OSCPLLa_AUTHEN(n)		(0x5030  + (n) * 0x40) /* 0..18 */
#define CLOCK_ROOT_AUTHEN(n)		(0x30  + (n) * 0x80) /* 0..94 */
#define LPCGa_AUTHEN(n)			(0x8030 + (n) * 0x40) /* 0..126 */
#define GPR_SHARED0_AUTHEN(n)		(0x4810 + (n) * 0x10) /* 0..3 */
#define SET 4

#define SRC_SP_ISO_CTRL			0x10c

#define MIX_PD_MEDIAMIX				1
#define MIX_PD_MLMIX				2
#define ANOMIX_LP_HANDSHAKE			0x110
#define SRC_MIX_MEDIA				8
#define SRC_MEM_MEDIA				8
#define SRC_MIX_ML				5
#define SRC_MEM_ML				4
#define SRC_MIX_SLICE_FUNC_STAT_PSW_STAT	BIT(0)
#define SRC_MIX_SLICE_FUNC_STAT_RST_STAT	BIT(2)
#define SRC_MIX_SLICE_FUNC_STAT_ISO_STAT	BIT(4)
#define SRC_MIX_SLICE_FUNC_STAT_MEM_STAT	BIT(12)
#define SRC_AUTHEN_CTRL				0x4
#define SRC_MEM_CTRL				0x4
#define SRC_PSW_ACK_CTRL0			0x80
#define SRC_GLOBAL_SCR				0x10
#define SRC_SLICE_SW_CTRL			0x20
#define SRC_FUNC_STAT				0xb4

static void imx93_mix_power_init(int pd)
{
	void __iomem *anomix = IOMEM(MX9_ANOMIX_BASE_ADDR);
	void __iomem *global_regs = IOMEM(MX9_SRC_BASE_ADDR);
	void __iomem *mix_regs, *mem_regs;
	u32 scr, val, mix_id, mem_id;

	scr = 0;
	mix_id = 0;
	mem_id = 0;

	switch (pd) {
	case MIX_PD_MEDIAMIX:
		mix_id = SRC_MIX_MEDIA;
		mem_id = SRC_MEM_MEDIA;
		scr = BIT(5);

		/* Enable ELE handshake */
		setbits_le32(anomix + ANOMIX_LP_HANDSHAKE, BIT(13));
		break;
	case MIX_PD_MLMIX:
		mix_id = SRC_MIX_ML;
		mem_id = SRC_MEM_ML;
		scr = BIT(4);
		break;
	}

	mix_regs = IOMEM(MX9_SRC_BASE_ADDR + 0x400 * (mix_id + 1));
	mem_regs = IOMEM(MX9_SRC_BASE_ADDR + 0x3800 + 0x400 * mem_id);

	/* Allow NS to set it */
	setbits_le32(mix_regs + SRC_AUTHEN_CTRL, BIT(9));

	clrsetbits_le32(mix_regs + SRC_PSW_ACK_CTRL0, BIT(28), BIT(29));

	/* mix reset will be held until boot core write this bit to 1 */
	setbits_le32(global_regs + SRC_GLOBAL_SCR, scr);

	/* Enable mem in Low power auto sequence */
	setbits_le32(mem_regs + SRC_MEM_CTRL, BIT(2));

	/* Set the power down state */
	val = readl(mix_regs + SRC_FUNC_STAT);
	if (val & SRC_MIX_SLICE_FUNC_STAT_PSW_STAT) {
		/* The mix is default power off, power down it to make PDN_SFT bit
		 *  aligned with FUNC STAT
		 */
		setbits_le32(mix_regs + SRC_SLICE_SW_CTRL, BIT(31));
		val = readl(mix_regs + SRC_FUNC_STAT);

		/* Since PSW_STAT is 1, can't be used for power off status (SW_CTRL BIT31 set)) */
		/* Check the MEM STAT change to ensure SSAR is completed */
		while (!(val & SRC_MIX_SLICE_FUNC_STAT_MEM_STAT))
			val = readl(mix_regs + SRC_FUNC_STAT);

		/* wait few ipg clock cycles to ensure FSM done and power off status is correct */
		/* About 5 cycles at 24Mhz, 1us is enough  */
		udelay(1);
	} else {
		/*  The mix is default power on, Do mix power cycle */
		setbits_le32(mix_regs + SRC_SLICE_SW_CTRL, BIT(31));
		val = readl(mix_regs + SRC_FUNC_STAT);
		while (!(val & SRC_MIX_SLICE_FUNC_STAT_PSW_STAT))
			val = readl(mix_regs + SRC_FUNC_STAT);
	}

	/* power on */
	clrbits_le32(mix_regs + SRC_SLICE_SW_CTRL, BIT(31));
	val = readl(mix_regs + SRC_FUNC_STAT);
	while (val & SRC_MIX_SLICE_FUNC_STAT_ISO_STAT)
		val = readl(mix_regs + SRC_FUNC_STAT);
}

void imx93_cpu_lowlevel_init(void)
{
	void __iomem *ccm = IOMEM(MX9_CCM_BASE_ADDR);
	void __iomem *src = IOMEM(MX9_SRC_BASE_ADDR);
	int i;

	arm_cpu_lowlevel_init();

	if (current_el() != 3)
		return;

	imx9_trdc_init();

	imx_cpu_timer_init(IOMEM(MX9_SYSCNT_CTRL_BASE_ADDR));

	for (i = 0; i <= 18; i++)
		writel(CCM_AUTHEN_TZ_NS, ccm + OSCPLLa_AUTHEN(i) + SET);
	for (i = 0; i <= 94; i++)
		writel(CCM_AUTHEN_TZ_NS, ccm + CLOCK_ROOT_AUTHEN(i) + SET);
	for (i = 0; i <= 126 ; i++)
		writel(CCM_AUTHEN_TZ_NS, ccm + LPCGa_AUTHEN(i) + SET);
	for (i = 0; i <= 3 ; i++)
		writel(CCM_AUTHEN_TZ_NS, ccm + GPR_SHARED0_AUTHEN(i) + SET);

	imx93_mix_power_init(MIX_PD_MEDIAMIX);
	imx93_mix_power_init(MIX_PD_MLMIX);

	/* clear isolation for usbphy, dsi, csi*/
	writel(0x0, src + SRC_SP_ISO_CTRL);

}

#endif
