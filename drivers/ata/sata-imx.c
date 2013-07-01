#include <common.h>
#include <ata_drive.h>
#include <io.h>
#include <clock.h>
#include <disks.h>
#include <driver.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <malloc.h>
#include <mach/imx53-regs.h>
#include <mach/imx6-regs.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include "ahci.h"

#define HOST_TIMER1MS 0xe0 /* Timer 1-ms */

struct imx_ahci {
	struct ahci_device ahci;
	struct clk *clk;
};

struct imx_sata_data {
	int (*init)(struct imx_ahci *imx_ahci);
};

static int imx6_sata_init(struct imx_ahci *imx_ahci)
{
	u32 val;
	void __iomem *base = (void *)MX6_IOMUXC_BASE_ADDR;

	val = 0x11 << IMX6Q_GPR13_SATA_PHY_2_OFF | IMX6Q_GPR13_SATA_PHY_4_9_16 |
		IMX6Q_GPR13_SATA_SPEED_MASK | 0x3 << IMX6Q_GPR13_SATA_PHY_6_OFF |
		IMX6Q_GPR13_SATA_PHY_7_SATA2I | IMX6Q_GPR13_SATA_PHY_8_3_0_DB;

	writel(val, base + IOMUXC_GPR13);
	writel(val | 2, base + IOMUXC_GPR13);

	return 0;
}

static int imx53_sata_init(struct imx_ahci *imx_ahci)
{
	u32 val;
	void __iomem *base = (void *)MX53_IIM_BASE_ADDR;

	/*
	 * The clock for the external interface can be set to use internal clock
	 * if fuse bank 4, row 3, bit 2 is set.
	 * This is an undocumented feature and it was confirmed by Freescale's support:
	 * Fuses (but not pins) may be used to configure SATA clocks.
	 * Particularly the i.MX53 Fuse_Map contains the next information
	 * about configuring SATA clocks :  SATA_ALT_REF_CLK[1:0] (offset 0x180C)
	 * '00' - 100MHz (External)
	 * '01' - 50MHz (External)
	 * '10' - 120MHz, internal (USB PHY)
	 * '11' - Reserved
	 */
	val = readl(base + 0x180c);
	val &= (0x3 << 1);
	val |= (0x1 << 1);
	writel(val, base + 0x180c);

	return 0;
}

static int imx_sata_init_1ms(struct imx_ahci *imx_ahci)
{
	void __iomem *base = imx_ahci->ahci.mmio_base;
	u32 val;

	val = readl(base + HOST_PORTS_IMPL);
	writel(val | 1, base + HOST_PORTS_IMPL);

	val = clk_get_rate(imx_ahci->clk) / 1000;

	writel(val, base + HOST_TIMER1MS);

	return 0;
}

static int imx_sata_probe(struct device_d *dev)
{
	struct imx_ahci *imx_ahci;
	struct imx_sata_data *data;
	int ret;

	ret = dev_get_drvdata(dev, (unsigned long *)&data);
	if (ret)
		return ret;

	imx_ahci = xzalloc(sizeof(*imx_ahci));

	imx_ahci->clk = clk_get(dev, NULL);
	if (IS_ERR(imx_ahci->clk)) {
		ret = PTR_ERR(imx_ahci->clk);
		goto err_free;
	}

	imx_ahci->ahci.mmio_base = dev_request_mem_region(dev, 0);
	if (!imx_ahci->ahci.mmio_base)
		return -ENODEV;

	data->init(imx_ahci);

	imx_sata_init_1ms(imx_ahci);

	imx_ahci->ahci.dev = dev;
	dev->priv = &imx_ahci->ahci;
	dev->info = ahci_info,

	ret = ahci_add_host(&imx_ahci->ahci);
	if (ret)
		goto err_free;

	return 0;

err_free:
	free(imx_ahci);

	return ret;
}

struct imx_sata_data data_imx6 = {
	.init = imx6_sata_init,
};

struct imx_sata_data data_imx53 = {
	.init = imx53_sata_init,
};

static struct platform_device_id imx_sata_ids[] = {
	{
		.name = "imx6-sata",
		.driver_data = (unsigned long)&data_imx6,
	}, {
		.name = "imx53-sata",
		.driver_data = (unsigned long)&data_imx53,
	}, {
		/* sentinel */
	},
};

static __maybe_unused struct of_device_id imx_sata_dt_ids[] = {
	{
		.compatible = "fsl,imx6q-ahci",
		.data = (unsigned long)&data_imx6,
	}, {
		.compatible = "fsl,imx53-ahci",
		.data = (unsigned long)&data_imx53,
	}, {
		/* sentinel */
	}
};

static struct driver_d imx_sata_driver = {
	.name   = "imx-sata",
	.probe  = imx_sata_probe,
	.id_table = imx_sata_ids,
	.of_compatible = DRV_OF_COMPAT(imx_sata_dt_ids),
};
device_platform_driver(imx_sata_driver);
