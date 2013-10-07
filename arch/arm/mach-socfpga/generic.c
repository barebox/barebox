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

static int socfpga_init(void)
{
	uint32_t val;

	/* Clearing emac0 PHY interface select to 0 */
	val = readl(CONFIG_SYSMGR_EMAC_CTRL);
	val &= ~(SYSMGR_EMACGRP_CTRL_PHYSEL_MASK << SYSMGR_EMACGRP_CTRL_PHYSEL1_LSB);
	val |= SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RGMII << SYSMGR_EMACGRP_CTRL_PHYSEL1_LSB;
	writel(val, CONFIG_SYSMGR_EMAC_CTRL);

	writel(SYSMGR_SDMMC_CTRL_DRVSEL(3) | SYSMGR_SDMMC_CTRL_SMPLSEL(0),
		SYSMGR_SDMMCGRP_CTRL_REG);

	nic301_slave_ns();

	socfpga_detect_sdram();

	return 0;
}
core_initcall(socfpga_init);

#if defined(CONFIG_DEFAULT_ENVIRONMENT)
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
		printf("no %s. using default env\n", diskdev);
		goto out_free;
	}

	mkdir("/boot", 0666);
	ret = mount(partname, "fat", "/boot");
	if (ret) {
		printf("failed to mount %s\n", diskdev);
		goto out_free;
	}

	default_environment_path = "/boot/barebox.env";

out_free:
	free(partname);
	return 0;
}
late_initcall(socfpga_env_init);
#endif
