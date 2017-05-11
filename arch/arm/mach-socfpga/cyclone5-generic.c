#include <common.h>
#include <malloc.h>
#include <envfs.h>
#include <init.h>
#include <io.h>
#include <fs.h>
#include <mci.h>
#include <linux/clkdev.h>
#include <linux/clk.h>
#include <linux/stat.h>
#include <linux/sizes.h>
#include <asm/memory.h>
#include <mach/cyclone5-system-manager.h>
#include <mach/cyclone5-reset-manager.h>
#include <mach/cyclone5-regs.h>
#include <mach/generic.h>
#include <mach/nic301.h>
#include <platform_data/dw_mmc.h>
#include <platform_data/serial-ns16550.h>
#include <platform_data/cadence_qspi.h>

#define SYSMGR_SDMMCGRP_CTRL_REG		(CYCLONE5_SYSMGR_ADDRESS + 0x108)
#define SYSMGR_SDMMC_CTRL_SMPLSEL(smplsel)	(((smplsel) & 0x7) << 3)
#define SYSMGR_SDMMC_CTRL_DRVSEL(drvsel)	((drvsel) & 0x7)

enum socfpga_clks {
	timer, mmc, qspi_clk, uart, clk_max
};

static struct clk *clks[clk_max];

#if defined(CONFIG_MCI_DW)
static struct dw_mmc_platform_data mmc_pdata = {
	.bus_width_caps = MMC_CAP_4_BIT_DATA,
	.ciu_div = 3,
};

void socfpga_cyclone5_mmc_init(void)
{
	clks[mmc] = clk_fixed("mmc", 400000000);
	clkdev_add_physbase(clks[mmc], CYCLONE5_SDMMC_ADDRESS, NULL);
	add_generic_device("dw_mmc", 0, NULL, CYCLONE5_SDMMC_ADDRESS, SZ_4K,
			IORESOURCE_MEM, &mmc_pdata);
}
#else
void socfpga_cyclone5_mmc_init(void)
{
	pr_debug("%s: MMC support not compiled in!\n", __func__);

	return;
}
#endif

#if defined(CONFIG_SPI_CADENCE_QUADSPI)
static struct cadence_qspi_platform_data qspi_pdata = {
	.is_decoded_cs = 0,
	.fifo_depth = 128,
};

static void add_cadence_qspi_device(int id, resource_size_t ctrl,
				    resource_size_t data, void *pdata)
{
	struct device_d *dev;
	struct resource *res;

	res = xzalloc(sizeof(struct resource) * 2);
	res[0].start = ctrl;
	res[0].end = ctrl + 0x100 - 1;
	res[0].flags = IORESOURCE_MEM;
	res[1].start = data;
	res[1].end = data + 0x100 - 1;
	res[1].flags = IORESOURCE_MEM;

	dev = add_generic_device_res("cadence_qspi", id, res, 2, pdata);

	dev_dbg(dev, "added resource\n");
}

void socfpga_cyclone5_qspi_init(void)
{
	clks[qspi_clk] = clk_fixed("qspi_clk", 370000000);
	clkdev_add_physbase(clks[qspi_clk], CYCLONE5_QSPI_CTRL_ADDRESS, NULL);
	clkdev_add_physbase(clks[qspi_clk], CYCLONE5_QSPI_DATA_ADDRESS, NULL);
	add_cadence_qspi_device(0, CYCLONE5_QSPI_CTRL_ADDRESS,
				CYCLONE5_QSPI_DATA_ADDRESS, &qspi_pdata);
}
#else
void socfpga_cyclone5_qspi_init(void)
{
	pr_debug("%s: QSPI support not compiled in!\n", __func__);

	return;
}
#endif

static struct NS16550_plat uart_pdata = {
	.clock = 100000000,
	.shift = 2,
};

void socfpga_cyclone5_uart_init(void)
{
	struct device_d *dev;

	clks[uart] = clk_fixed("uart", 100000000);
	clkdev_add_physbase(clks[uart], CYCLONE5_UART0_ADDRESS, NULL);
	clkdev_add_physbase(clks[uart], CYCLONE5_UART1_ADDRESS, NULL);
	dev = add_ns16550_device(0, 0xffc02000, 1024, IORESOURCE_MEM |
			IORESOURCE_MEM_8BIT, &uart_pdata);

	dev_dbg(dev, "initialized\n");
}

void socfpga_cyclone5_timer_init(void)
{
	struct device_d *dev;

	clks[timer] = clk_fixed("timer", 200000000);
	clkdev_add_physbase(clks[timer], CYCLONE5_SMP_TWD_ADDRESS, NULL);
	dev = add_generic_device("smp_twd", 0, NULL, CYCLONE5_SMP_TWD_ADDRESS, 0x100,
				 IORESOURCE_MEM, NULL);

	dev_dbg(dev, "added smp_twd\n");
}

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
