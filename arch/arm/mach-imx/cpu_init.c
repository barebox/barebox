// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2014 Lucas Stach, Pengutronix

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/errata.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx7-regs.h>
#include <mach/imx/imx8mq-regs.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mach/imx/imx9-regs.h>
#include <mach/imx/trdc.h>
#include <io.h>
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
}

void imx8mn_cpu_lowlevel_init(void)
	__alias(imx8mm_cpu_lowlevel_init);

void imx8mp_cpu_lowlevel_init(void)
	__alias(imx8mm_cpu_lowlevel_init);

void imx8mq_cpu_lowlevel_init(void)
{
	imx8m_cpu_lowlevel_init();
}

#define CCM_AUTHEN_TZ_NS	BIT(9)

#define OSCPLLa_AUTHEN(n)		(0x5030  + (n) * 0x40) /* 0..18 */
#define CLOCK_ROOT_AUTHEN(n)		(0x30  + (n) * 0x80) /* 0..94 */
#define LPCGa_AUTHEN(n)			(0x8030 + (n) * 0x40) /* 0..126 */
#define GPR_SHARED0_AUTHEN(n)		(0x4810 + (n) * 0x10) /* 0..3 */
#define SET 4

#define SRC_SP_ISO_CTRL			0x10c

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

	/* clear isolation for usbphy, dsi, csi*/
	writel(0x0, src + SRC_SP_ISO_CTRL);

}

#endif
