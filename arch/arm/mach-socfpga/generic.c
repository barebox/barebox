#include <common.h>
#include <malloc.h>
#include <envfs.h>
#include <init.h>
#include <io.h>
#include <fs.h>
#include <net/designware.h>
#include <linux/clkdev.h>
#include <linux/clk.h>
#include <linux/stat.h>
#include <asm/memory.h>
#include <mach/system-manager.h>
#include <mach/reset-manager.h>
#include <mach/socfpga-regs.h>
#include <mach/nic301.h>

#define SYSMGR_SDMMCGRP_CTRL_REG	(CYCLONE5_SYSMGR_ADDRESS + 0x108)
#define SYSMGR_SDMMC_CTRL_SMPLSEL(smplsel)	(((smplsel) & 0x7) << 3)
#define SYSMGR_SDMMC_CTRL_DRVSEL(drvsel)	((drvsel) & 0x7)

static int socfpga_detect_sdram(void)
{
	void __iomem *base = (void *)CYCLONE5_SDR_ADDRESS;
	uint32_t dramaddrw, ctrlwidth, memsize;
	int colbits, rowbits, bankbits;
	int width_bytes;

	dramaddrw = readl(base + 0x5000 + 0x2c);

	colbits = dramaddrw & 0x1f;
	rowbits = (dramaddrw >> 5) & 0x1f;
	bankbits = (dramaddrw >> 10) & 0x7;

	ctrlwidth = readl(base + 0x5000 + 0x60);

	switch (ctrlwidth & 0x3) {
	default:
	case 0:
		width_bytes = 1;
		break;
	case 1:
		width_bytes = 2;
		break;
	case 2:
		width_bytes = 4;
		break;
	}

	memsize = (1 << colbits) * (1 << rowbits) * (1 << bankbits) * width_bytes;

	pr_debug("%s: colbits: %d rowbits: %d bankbits: %d width: %d => memsize: 0x%08x\n",
			__func__, colbits, rowbits, bankbits, width_bytes, memsize);

	arm_add_mem_device("ram0", 0x0, memsize);

	return 0;
}

/* Some initialization for the EMAC */
static void socfpga_init_emac(void)
{
	uint32_t rst, val;

	/* No need for this without network support, e.g. xloader build */
	if (!IS_ENABLED(CONFIG_NET))
		return;

	/* According to Cyclone V datasheet, 17-60 "EMAC HPS Interface
	 * Initialization", changing PHYSEL should be done with EMAC in reset
	 * via permodrst.  */

	/* Everything, except L4WD0/1, is out of reset via socfpga_lowlevel_init() */
	rst = readl(CYCLONE5_RSTMGR_ADDRESS + RESET_MGR_PER_MOD_RESET_OFS);
	rst |= RSTMGR_PERMODRST_EMAC0 | RSTMGR_PERMODRST_EMAC1;
	writel(rst, CYCLONE5_RSTMGR_ADDRESS + RESET_MGR_PER_MOD_RESET_OFS);

	/* Set emac0/1 PHY interface select to RGMII.  We could read phy-mode
	 * from the device tree, if it was desired to support interfaces other
	 * than RGMII. */
	val = readl(CONFIG_SYSMGR_EMAC_CTRL);
	val &= ~(SYSMGR_EMACGRP_CTRL_PHYSEL_MASK << SYSMGR_EMACGRP_CTRL_PHYSEL0_LSB);
	val &= ~(SYSMGR_EMACGRP_CTRL_PHYSEL_MASK << SYSMGR_EMACGRP_CTRL_PHYSEL1_LSB);
	val |= SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RGMII << SYSMGR_EMACGRP_CTRL_PHYSEL0_LSB;
	val |= SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RGMII << SYSMGR_EMACGRP_CTRL_PHYSEL1_LSB;
	writel(val, CONFIG_SYSMGR_EMAC_CTRL);

	/* Take emac0 and emac1 out of reset */
	rst &= ~(RSTMGR_PERMODRST_EMAC0 | RSTMGR_PERMODRST_EMAC1);
	writel(rst, CYCLONE5_RSTMGR_ADDRESS + RESET_MGR_PER_MOD_RESET_OFS);
}

static int socfpga_init(void)
{
	socfpga_init_emac();

	writel(SYSMGR_SDMMC_CTRL_DRVSEL(3) | SYSMGR_SDMMC_CTRL_SMPLSEL(0),
		SYSMGR_SDMMCGRP_CTRL_REG);

	nic301_slave_ns();

	socfpga_detect_sdram();

	return 0;
}
core_initcall(socfpga_init);

#if defined(CONFIG_ENV_HANDLING)
#define ENV_PATH "/boot/barebox.env"
static int socfpga_env_init(void)
{
	struct stat s;
	char *diskdev, *partname;
	int ret;

	diskdev = "mmc0";

	device_detect_by_name(diskdev);

	partname = asprintf("/dev/%s.1", diskdev);

	ret = stat(partname, &s);

	if (ret) {
		pr_err("Failed to load environment: no device '%s'\n", diskdev);
		goto out_free;
	}

	mkdir("/boot", 0666);
	ret = mount(partname, "fat", "/boot", NULL);
	if (ret) {
		pr_err("Failed to load environment: mount %s failed (%d)\n", partname, ret);
		goto out_free;
	}

	pr_debug("Loading default env from %s on device %s\n", ENV_PATH, diskdev);
	default_environment_path_set(ENV_PATH);

out_free:
	free(partname);
	return 0;
}
late_initcall(socfpga_env_init);
#endif
