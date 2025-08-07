// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <io.h>
#include <debug_ll.h>
#include <asm/barebox-arm-head.h>
#include <asm/cache.h>
#include <asm/errata.h>
#include <mach/socfpga/init.h>
#include <mach/socfpga/agilex5-clk.h>
#include <mach/socfpga/mailbox_s10.h>
#include <mach/socfpga/soc64-regs.h>
#include <mach/socfpga/soc64-reset-manager.h>
#include <mach/socfpga/soc64-system-manager.h>

#ifdef CONFIG_CPU_32
void arria10_cpu_lowlevel_init(void)
{
	arm_early_mmu_cache_invalidate();

	/* apply necessary workarounds for Cortex A9 r4p1 */
	enable_arm_errata_794072_war();
	enable_arm_errata_845369_war();
}
#else
void socfpga_agilex5_cpu_lowlevel_init(void)
{
	int enable = 0x3;
	int loadval = ~0;
	int val;

	arm_cpu_lowlevel_init();

	agilex5_sysmgr_pinmux_init();

	/* Disable all watchdogs */
	writel(SYSMGR_WDDBG_PAUSE_ALL_CPU, SOCFPGA_SYSMGR_ADDRESS +
			SYSMGR_SOC64_WDDBG);
	/* Let everything out of reset but the watchdogs */
	val = 0xf;
	writel(val, SOCFPGA_RSTMGR_ADDRESS + RSTMGR_SOC64_PER1MODRST);

	/* de-assert resets */
	writel(0, SOCFPGA_RSTMGR_ADDRESS + RSTMGR_SOC64_BRGMODRST);

	/* configure DFI_SEL for SDMMC */
	writel(SYSMGR_SOC64_COMBOPHY_DFISEL_SDMMC,
	       SOCFPGA_SYSMGR_ADDRESS + SYSMGR_SOC64_COMBOPHY_DFISEL);

	writel(enable, SOCFPGA_GTIMER_SEC_ADDRESS);
	asm volatile("msr cntp_ctl_el0, %0" : : "r" (enable));
	asm volatile("msr cntp_tval_el0, %0" : : "r" (loadval));

	/* configure default base clkmgr clock - 200MHz */
	val = readl(SOCFPGA_CLKMGR_ADDRESS + CLKMGR_MAINPLL_NOCDIV);
	val &= 0xfffcffff | (CLKMGR_NOCDIV_SOFTPHY_DIV_ONE << CLKMGR_NOCDIV_SOFTPHY_OFFSET);
	writel(val, SOCFPGA_CLKMGR_ADDRESS + CLKMGR_MAINPLL_NOCDIV);
}
#endif
