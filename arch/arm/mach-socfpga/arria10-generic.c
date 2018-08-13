#include <common.h>
#include <io.h>
#include <init.h>
#include <restart.h>
#include <mach/generic.h>
#include <mach/arria10-reset-manager.h>
#include <mach/arria10-system-manager.h>
#include <mach/arria10-regs.h>

/* Some initialization for the EMAC */
static void arria10_init_emac(void)
{
	uint32_t rst, val;

	/* No need for this without network support, e.g. xloader build */
	if (!IS_ENABLED(CONFIG_NET))
		return;

	rst = readl(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST);
	rst |= ARRIA10_RSTMGR_PER0MODRST_EMAC0 |
	       ARRIA10_RSTMGR_PER0MODRST_EMAC1 |
	       ARRIA10_RSTMGR_PER0MODRST_EMAC2;

	writel(rst, ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST);
	val = readl(ARRIA10_SYSMGR_EMAC0);
	val &= ~(ARRIA10_SYSMGR_EMACGRP_CTRL_PHYSEL_MASK);
	val |= ARRIA10_SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RGMII;
	writel(val, ARRIA10_SYSMGR_EMAC0);

	val = readl(ARRIA10_SYSMGR_EMAC1);
	val &= ~(ARRIA10_SYSMGR_EMACGRP_CTRL_PHYSEL_MASK);
	val |= ARRIA10_SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RGMII;
	writel(val, ARRIA10_SYSMGR_EMAC1);

	val = readl(ARRIA10_SYSMGR_EMAC2);
	val &= ~(ARRIA10_SYSMGR_EMACGRP_CTRL_PHYSEL_MASK);
	val |= ARRIA10_SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RGMII;
	writel(val, ARRIA10_SYSMGR_EMAC2);

	rst = readl(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST);
	rst &= ~(ARRIA10_RSTMGR_PER0MODRST_EMAC0 |
		 ARRIA10_RSTMGR_PER0MODRST_EMAC1 |
		 ARRIA10_RSTMGR_PER0MODRST_EMAC2);
	writel(rst, ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST);
}

/* Write the reset manager register to cause reset */
static void __noreturn arria10_restart_soc(struct restart_handler *rst)
{
	/* request a warm reset */
	writel(ARRIA10_RSTMGR_CTL_SWWARMRSTREQ,
	       ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_CTRL);
	/*
	 * infinite loop here as watchdog will trigger and reset
	 * the processor
	 */
	hang();
}

static int arria10_generic_init(void)
{
	barebox_set_model("SoCFPGA Arria10");

	pr_debug("Setting SDMMC phase shifts for Arria10\n");
	writel(ARRIA10_SYSMGR_SDMMC_DRVSEL(3) |
	       ARRIA10_SYSMGR_SDMMC_SMPLSEL(2),
	       ARRIA10_SYSMGR_SDMMC);

	pr_debug("Initialize EMACs\n");
	arria10_init_emac();

	pr_debug("Register restart handler\n");
	restart_handler_register_fn(arria10_restart_soc);

	return 0;
}
postcore_initcall(arria10_generic_init);
