// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2014 Lucas Stach, Pengutronix

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/errata.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <mach/generic.h>
#include <mach/imx7-regs.h>
#include <mach/imx8mq-regs.h>
#include <mach/imx8m-ccm-regs.h>
#include <common.h>
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

void imx8mp_cpu_lowlevel_init(void)
{
	imx8m_cpu_lowlevel_init();
}

void imx8mq_cpu_lowlevel_init(void)
{
	imx8m_cpu_lowlevel_init();
}
#endif
